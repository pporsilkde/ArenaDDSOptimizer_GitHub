#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaType>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStatusBar>
#include <QTableWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>

namespace
{
QString native(const QString& p) { return QDir::toNativeSeparators(p); }

bool samePath(const QString& a, const QString& b)
{
    const QString aa = QDir::cleanPath(QFileInfo(a).absoluteFilePath());
    const QString bb = QDir::cleanPath(QFileInfo(b).absoluteFilePath());
#ifdef Q_OS_WIN
    return aa.compare(bb, Qt::CaseInsensitive) == 0;
#else
    return aa == bb;
#endif
}

bool isInsideTree(const QString& path, const QString& root)
{
    if (root.trimmed().isEmpty())
        return false;

    const QString cleanPath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    QString cleanRoot = QDir::cleanPath(QFileInfo(root).absoluteFilePath());
    if (!cleanRoot.endsWith(QDir::separator()))
        cleanRoot += QDir::separator();

#ifdef Q_OS_WIN
    return cleanPath.startsWith(cleanRoot, Qt::CaseInsensitive);
#else
    return cleanPath.startsWith(cleanRoot);
#endif
}

constexpr int SortRole = Qt::UserRole + 1;
constexpr int JobIndexRole = Qt::UserRole + 2;

class SortableTableWidgetItem final : public QTableWidgetItem
{
public:
    explicit SortableTableWidgetItem(const QString& text = {}, const QVariant& sortKey = {})
        : QTableWidgetItem(text)
    {
        if (sortKey.isValid())
            setData(SortRole, sortKey);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const QVariant a = data(SortRole);
        const QVariant b = other.data(SortRole);
        if (a.isValid() && b.isValid())
        {
            const int aType = a.userType();
            const int bType = b.userType();
            const bool aNumber = aType == QMetaType::Int || aType == QMetaType::UInt ||
                                 aType == QMetaType::LongLong || aType == QMetaType::ULongLong ||
                                 aType == QMetaType::Double;
            const bool bNumber = bType == QMetaType::Int || bType == QMetaType::UInt ||
                                 bType == QMetaType::LongLong || bType == QMetaType::ULongLong ||
                                 bType == QMetaType::Double;
            if (aNumber && bNumber)
                return a.toDouble() < b.toDouble();
            return QString::localeAwareCompare(a.toString(), b.toString()) < 0;
        }
        return QTableWidgetItem::operator<(other);
    }
};
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_profiles(builtInProfiles())
    , m_process(new QProcess(this))
{
    setWindowTitle(QStringLiteral("Arena DDS Optimizer"));
    resize(1180, 720);
    buildUi();
    loadSettings();

    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &MainWindow::processFinished);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::processError);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::buildUi()
{
    auto* root = new QWidget(this);
    auto* main = new QVBoxLayout(root);

    auto* paths = new QGroupBox(QStringLiteral("Источник и инструмент"), root);
    auto* form = new QFormLayout(paths);

    auto makePathRow = [paths](QLineEdit*& edit, const QString& buttonText, auto slot) {
        auto* w = new QWidget(paths);
        auto* l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        edit = new QLineEdit(w);
        auto* b = new QPushButton(buttonText, w);
        l->addWidget(edit, 1);
        l->addWidget(b);
        QObject::connect(b, &QPushButton::clicked, slot);
        return w;
    };

    form->addRow(QStringLiteral("Папка с DDS:"), makePathRow(m_sourceEdit, QStringLiteral("Обзор…"), [this]{ browseSource(); }));
    form->addRow(QStringLiteral("Выходная папка:"), makePathRow(m_outputEdit, QStringLiteral("Обзор…"), [this]{ browseOutput(); }));
    form->addRow(QStringLiteral("texconv.exe (встроенный):"), makePathRow(m_toolEdit, QStringLiteral("Обзор…"), [this]{ browseTool(); }));
    m_toolEdit->setPlaceholderText(QStringLiteral("В portable-сборке находится рядом с ArenaDDSOptimizer.exe"));
    m_toolEdit->setToolTip(QStringLiteral("Windows release включает DirectXTex texconv.exe. Ручной путь нужен только для собственной/отладочной сборки."));
    main->addWidget(paths);

    auto* options = new QGroupBox(QStringLiteral("Профиль оптимизации"), root);
    auto* optionsLayout = new QHBoxLayout(options);
    m_profileCombo = new QComboBox(options);
    for (const auto& p : m_profiles)
        m_profileCombo->addItem(p.displayName, p.id);
    m_compressionCombo = new QComboBox(options);
    m_compressionCombo->addItem(QStringLiteral("Доп. компрессия: обычная"), 0);
    m_compressionCombo->addItem(QStringLiteral("Доп. компрессия: сильная"), 1);
    m_compressionCombo->addItem(QStringLiteral("Доп. компрессия: максимальная"), 2);
    m_compressionCombo->setToolTip(QStringLiteral(
        "BC1/BC3 имеют фиксированный размер блока. Дополнительное уменьшение размера достигается "
        "контролируемым снижением разрешения крупных текстур: сильная = 1/2 лимита профиля, "
        "максимальная = 1/4 (не ниже 1024). Полные mipmaps сохраняются."));
    m_recursiveCheck = new QCheckBox(QStringLiteral("Все подпапки (рекурсивно)"), options);
    m_recursiveCheck->setToolTip(QStringLiteral("Сканировать выбранную папку и все вложенные подпапки на любой глубине."));
    m_recursiveCheck->setChecked(true);
    m_backupCheck = new QCheckBox(QStringLiteral("Резервная копия при замене"), options);
    m_backupCheck->setChecked(true);
    m_forceCheck = new QCheckBox(QStringLiteral("Перекодировать даже оптимальные"), options);
    m_dryRunCheck = new QCheckBox(QStringLiteral("Только анализ"), options);
    optionsLayout->addWidget(m_profileCombo, 1);
    optionsLayout->addWidget(m_compressionCombo);
    optionsLayout->addWidget(m_recursiveCheck);
    optionsLayout->addWidget(m_backupCheck);
    optionsLayout->addWidget(m_forceCheck);
    optionsLayout->addWidget(m_dryRunCheck);
    main->addWidget(options);

    auto* actions = new QHBoxLayout;
    m_scanButton = new QPushButton(QStringLiteral("Сканировать"), root);
    m_optimizeButton = new QPushButton(QStringLiteral("Оптимизировать"), root);
    m_cancelButton = new QPushButton(QStringLiteral("Отмена"), root);
    m_cancelButton->setEnabled(false);
    m_summaryLabel = new QLabel(QStringLiteral("Файлы ещё не просканированы"), root);
    actions->addWidget(m_scanButton);
    actions->addWidget(m_optimizeButton);
    actions->addWidget(m_cancelButton);
    actions->addSpacing(12);
    actions->addWidget(m_summaryLabel, 1);
    main->addLayout(actions);

    m_table = new QTableWidget(root);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Файл"), QStringLiteral("Размер"), QStringLiteral("Формат"), QStringLiteral("Mip"),
        QStringLiteral("На диске"), QStringLiteral("Цель"), QStringLiteral("План"), QStringLiteral("Статус")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionsClickable(true);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setToolTip(QStringLiteral("Нажмите заголовок столбца для сортировки; повторный клик меняет направление."));
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c <= 6; ++c)
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    main->addWidget(m_table, 1);

    m_progress = new QProgressBar(root);
    m_progress->setRange(0, 1);
    m_progress->setValue(0);
    main->addWidget(m_progress);

    setCentralWidget(root);
    statusBar()->showMessage(QStringLiteral("Готово"));

    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::scanTextures);
    connect(m_optimizeButton, &QPushButton::clicked, this, &MainWindow::optimizeTextures);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelOptimization);
    connect(m_profileCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::profileChanged);
    connect(m_compressionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]{ refreshPlans(); });
    connect(m_forceCheck, &QCheckBox::toggled, this, [this]{ refreshPlans(); });
}

void MainWindow::loadSettings()
{
    QSettings s(QStringLiteral("ArenaMP"), QStringLiteral("ArenaDDSOptimizer"));
    restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());
    m_sourceEdit->setText(s.value(QStringLiteral("source")).toString());
    m_outputEdit->setText(s.value(QStringLiteral("output")).toString());
    const QString savedTexconv = s.value(QStringLiteral("texconv")).toString();
    const QString bundledTexconv = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("texconv.exe"));
#ifdef Q_OS_WIN
    // Portable Windows releases ship texconv.exe next to ArenaDDSOptimizer.exe.
    // Prefer the bundled, version-pinned tool so the user does not need winget or PATH setup.
    if (QFileInfo::exists(bundledTexconv))
        m_toolEdit->setText(bundledTexconv);
    else
        m_toolEdit->setText(savedTexconv);
#else
    m_toolEdit->setText(savedTexconv);
#endif
    m_recursiveCheck->setChecked(s.value(QStringLiteral("recursive"), true).toBool());
    m_backupCheck->setChecked(s.value(QStringLiteral("backup"), true).toBool());
    m_forceCheck->setChecked(s.value(QStringLiteral("force"), false).toBool());
    const int savedCompression = s.value(QStringLiteral("compressionLevel"), 0).toInt();
    const int compressionIndex = m_compressionCombo->findData(savedCompression);
    if (compressionIndex >= 0)
        m_compressionCombo->setCurrentIndex(compressionIndex);
    const QString profile = s.value(QStringLiteral("profile"), QStringLiteral("safe")).toString();
    const int idx = m_profileCombo->findData(profile);
    if (idx >= 0)
        m_profileCombo->setCurrentIndex(idx);

#ifdef Q_OS_WIN
    if (m_toolEdit->text().isEmpty())
    {
        const QString found = QStandardPaths::findExecutable(QStringLiteral("texconv.exe"));
        if (!found.isEmpty())
            m_toolEdit->setText(found);
    }
#endif
}

void MainWindow::saveSettings()
{
    QSettings s(QStringLiteral("ArenaMP"), QStringLiteral("ArenaDDSOptimizer"));
    s.setValue(QStringLiteral("geometry"), saveGeometry());
    s.setValue(QStringLiteral("source"), m_sourceEdit->text());
    s.setValue(QStringLiteral("output"), m_outputEdit->text());
    s.setValue(QStringLiteral("texconv"), m_toolEdit->text());
    s.setValue(QStringLiteral("recursive"), m_recursiveCheck->isChecked());
    s.setValue(QStringLiteral("backup"), m_backupCheck->isChecked());
    s.setValue(QStringLiteral("force"), m_forceCheck->isChecked());
    s.setValue(QStringLiteral("compressionLevel"), compressionLevel());
    s.setValue(QStringLiteral("profile"), m_profileCombo->currentData());
}

void MainWindow::browseSource()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Папка с DDS"), m_sourceEdit->text());
    if (!dir.isEmpty())
    {
        m_sourceEdit->setText(dir);
        if (m_outputEdit->text().isEmpty())
            m_outputEdit->setText(dir);
    }
}

void MainWindow::browseOutput()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Выходная папка"), m_outputEdit->text());
    if (!dir.isEmpty())
        m_outputEdit->setText(dir);
}

void MainWindow::browseTool()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Выберите texconv"), m_toolEdit->text(),
#ifdef Q_OS_WIN
        QStringLiteral("texconv (texconv.exe);;Все файлы (*.*)"));
#else
        QStringLiteral("texconv (texconv);;Все файлы (*)"));
#endif
    if (!path.isEmpty())
        m_toolEdit->setText(path);
}

OptimizerProfile MainWindow::currentProfile() const
{
    const QString id = m_profileCombo->currentData().toString();
    for (const auto& p : m_profiles)
        if (p.id == id)
            return p;
    return m_profiles.first();
}

int MainWindow::compressionLevel() const
{
    return m_compressionCombo ? m_compressionCombo->currentData().toInt() : 0;
}

int MainWindow::rowForJob(int jobIndex) const
{
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const QTableWidgetItem* item = m_table->item(row, 0);
        if (item && item->data(JobIndexRole).toInt() == jobIndex)
            return row;
    }
    return -1;
}

void MainWindow::scanTextures()
{
    const QString root = QDir::cleanPath(m_sourceEdit->text().trimmed());
    if (root.isEmpty() || !QFileInfo(root).isDir())
    {
        QMessageBox::warning(this, QStringLiteral("Arena DDS Optimizer"), QStringLiteral("Выберите существующую папку с текстурами."));
        return;
    }

    saveSettings();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_jobs.clear();
    m_table->setRowCount(0);

    const QDirIterator::IteratorFlags flags = m_recursiveCheck->isChecked()
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;
    QDirIterator it(root, QStringList() << QStringLiteral("*.dds") << QStringLiteral("*.DDS"), QDir::Files, flags);
    QDir base(root);

    QString outputRoot = QDir::cleanPath(m_outputEdit->text().trimmed());
    if (outputRoot.isEmpty() || samePath(outputRoot, root) || !isInsideTree(outputRoot, root))
        outputRoot.clear();

    while (it.hasNext())
    {
        const QString path = it.next();
        const QString rel = QDir::fromNativeSeparators(base.relativeFilePath(path));

        // Never scan our own backup tree or a dedicated output tree located inside the source tree.
        if (rel.startsWith(QStringLiteral("_ArenaDDS_Backup/"), Qt::CaseInsensitive))
            continue;
        if (!outputRoot.isEmpty() && isInsideTree(path, outputRoot))
            continue;

        TextureJob job;
        job.inputPath = path;
        job.relativePath = rel;
        job.info = readDdsInfo(path);
        job.plan = buildPlan(path, job.info, currentProfile(), m_forceCheck->isChecked(), compressionLevel());
        m_jobs.push_back(job);
    }

    const bool sortingWasEnabled = m_table->isSortingEnabled();
    m_table->setSortingEnabled(false);
    m_table->setRowCount(m_jobs.size());
    for (int jobIndex = 0; jobIndex < m_jobs.size(); ++jobIndex)
    {
        const auto& job = m_jobs[jobIndex];
        const int r = jobIndex;
        auto set = [this, r, jobIndex](int c, const QString& text, const QVariant& sortKey = QVariant()) {
            auto* item = new SortableTableWidgetItem(text, sortKey);
            item->setData(JobIndexRole, jobIndex);
            m_table->setItem(r, c, item);
        };
        set(0, native(job.relativePath), job.relativePath.toLower());
        const qulonglong area = job.info.valid ? qulonglong(job.info.width) * qulonglong(job.info.height) : 0;
        set(1, job.info.valid ? QStringLiteral("%1×%2").arg(job.info.width).arg(job.info.height) : QStringLiteral("—"), area);
        set(2, job.info.format, job.info.format.toLower());
        set(3, job.info.valid ? QString::number(job.info.mipCount) : QStringLiteral("—"), job.info.valid ? job.info.mipCount : -1);
        set(4, humanSize(job.info.fileSize), qulonglong(job.info.fileSize));
        const qulonglong targetArea = job.plan.process ? qulonglong(job.plan.targetWidth) * qulonglong(job.plan.targetHeight) : 0;
        set(5, job.plan.process ? QStringLiteral("%1×%2 %3").arg(job.plan.targetWidth).arg(job.plan.targetHeight).arg(job.plan.outputFormat) : QStringLiteral("—"), targetArea);
        set(6, job.plan.reason, job.plan.reason.toLower());
        const QString status = job.plan.risky ? QStringLiteral("Проверить вручную") : (job.plan.process ? QStringLiteral("Готов к обработке") : QStringLiteral("Пропуск"));
        set(7, status, status.toLower());
    }
    m_table->setSortingEnabled(sortingWasEnabled);

    QApplication::restoreOverrideCursor();
    updateSummary();
    statusBar()->showMessage(QStringLiteral("Сканирование завершено"), 4000);
}

void MainWindow::refreshPlans()
{
    if (m_jobs.isEmpty())
        return;
    const bool sortingWasEnabled = m_table->isSortingEnabled();
    m_table->setSortingEnabled(false);
    for (int jobIndex = 0; jobIndex < m_jobs.size(); ++jobIndex)
    {
        auto& job = m_jobs[jobIndex];
        job.plan = buildPlan(job.inputPath, job.info, currentProfile(), m_forceCheck->isChecked(), compressionLevel());
        const int r = rowForJob(jobIndex);
        if (r < 0)
            continue;
        const QString target = job.plan.process ? QStringLiteral("%1×%2 %3").arg(job.plan.targetWidth).arg(job.plan.targetHeight).arg(job.plan.outputFormat) : QStringLiteral("—");
        m_table->item(r, 5)->setText(target);
        m_table->item(r, 5)->setData(SortRole, job.plan.process ? QVariant::fromValue(qulonglong(job.plan.targetWidth) * qulonglong(job.plan.targetHeight)) : QVariant::fromValue(qulonglong(0)));
        m_table->item(r, 6)->setText(job.plan.reason);
        m_table->item(r, 6)->setData(SortRole, job.plan.reason.toLower());
        const QString status = job.plan.risky ? QStringLiteral("Проверить вручную") : (job.plan.process ? QStringLiteral("Готов к обработке") : QStringLiteral("Пропуск"));
        m_table->item(r, 7)->setText(status);
        m_table->item(r, 7)->setData(SortRole, status.toLower());
    }
    m_table->setSortingEnabled(sortingWasEnabled);
    updateSummary();
}

void MainWindow::profileChanged(int)
{
    refreshPlans();
}

bool MainWindow::validateTexconv(QString& error) const
{
    const QString tool = m_toolEdit->text().trimmed();
    if (tool.isEmpty())
    {
        error = QStringLiteral("texconv.exe не найден. В официальной portable-сборке он должен находиться рядом с ArenaDDSOptimizer.exe. Для собственной сборки укажите путь вручную.");
        return false;
    }
    if (!QFileInfo(tool).isExecutable() && !QFileInfo(tool).isFile())
    {
        error = QStringLiteral("texconv не найден: %1").arg(native(tool));
        return false;
    }
    return true;
}

void MainWindow::optimizeTextures()
{
    if (m_jobs.isEmpty())
        scanTextures();
    if (m_jobs.isEmpty())
        return;

    if (m_dryRunCheck->isChecked())
    {
        QMessageBox::information(this, QStringLiteral("Arena DDS Optimizer"), QStringLiteral("Включён режим «Только анализ». Файлы не изменены."));
        return;
    }

    if (compressionLevel() > 0)
    {
        const QString mode = compressionLevel() == 1 ? QStringLiteral("сильная") : QStringLiteral("максимальная");
        const auto answer = QMessageBox::warning(this, QStringLiteral("Arena DDS Optimizer"),
            QStringLiteral("Выбрана %1 дополнительная компрессия.\n\n"
                           "Для уменьшения размера слишком крупные текстуры могут быть уменьшены по разрешению. "
                           "BC1/DXT1 и BC3/DXT5, а также полный mip-chain сохраняются.\n\nПродолжить?").arg(mode),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    QString error;
    if (!validateTexconv(error))
    {
        QMessageBox::warning(this, QStringLiteral("Arena DDS Optimizer"), error);
        return;
    }

    QString outputRoot = m_outputEdit->text().trimmed();
    if (outputRoot.isEmpty())
        outputRoot = m_sourceEdit->text().trimmed();
    if (!QDir().mkpath(outputRoot))
    {
        QMessageBox::warning(this, QStringLiteral("Arena DDS Optimizer"), QStringLiteral("Не удалось создать выходную папку."));
        return;
    }

    if (samePath(outputRoot, m_sourceEdit->text().trimmed()) && !m_backupCheck->isChecked())
    {
        const auto answer = QMessageBox::warning(this, QStringLiteral("Arena DDS Optimizer"),
            QStringLiteral("Вы выбрали замену DDS на месте и отключили резервную копию.\n\nПродолжить без возможности автоматического отката?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    m_queue.clear();
    for (int i = 0; i < m_jobs.size(); ++i)
        if (m_jobs[i].plan.process && !m_jobs[i].plan.risky)
            m_queue.push_back(i);

    if (m_queue.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Arena DDS Optimizer"), QStringLiteral("Для выбранного профиля нет файлов, требующих обработки."));
        return;
    }

    m_queuePos = 0;
    m_successCount = 0;
    m_failCount = 0;
    m_cancelRequested = false;
    m_sessionBackupRoot.clear();
    if (m_backupCheck->isChecked())
    {
        const QString source = QDir::cleanPath(m_sourceEdit->text().trimmed());
        const QString output = QDir::cleanPath(outputRoot);
        if (samePath(source, output))
            m_sessionBackupRoot = QDir(source).filePath(QStringLiteral("_ArenaDDS_Backup/%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    }

    m_progress->setRange(0, m_queue.size());
    m_progress->setValue(0);
    setBusy(true);
    startNextJob();
}

void MainWindow::startNextJob()
{
    if (m_cancelRequested || m_queuePos >= m_queue.size())
    {
        setBusy(false);
        m_progress->setValue(qMin(m_queuePos, m_queue.size()));
        statusBar()->showMessage(QStringLiteral("Готово: %1 успешно, %2 ошибок%3")
                                 .arg(m_successCount).arg(m_failCount)
                                 .arg(m_cancelRequested ? QStringLiteral(", отменено") : QString()), 10000);
        updateSummary();
        return;
    }

    m_currentJobIndex = m_queue[m_queuePos];
    const TextureJob& job = m_jobs[m_currentJobIndex];
    const int row = rowForJob(m_currentJobIndex);
    QTableWidgetItem* fileItem = row >= 0 ? m_table->item(row, 0) : nullptr;
    if (row >= 0)
    {
        m_table->item(row, 7)->setText(QStringLiteral("Обработка…"));
        m_table->item(row, 7)->setData(SortRole, QStringLiteral("обработка"));
    }
    if (fileItem)
        m_table->scrollToItem(fileItem);

    const QString tempBase = QDir(QDir::tempPath()).filePath(QStringLiteral("ArenaDDSOptimizer_%1_%2")
        .arg(QCoreApplication::applicationPid()).arg(m_currentJobIndex));
    QDir(tempBase).removeRecursively();
    QDir().mkpath(tempBase);
    m_currentTempDir = tempBase;

    const OptimizerProfile profile = currentProfile();
    QStringList args;
    args << QStringLiteral("-nologo") << QStringLiteral("-y")
         << QStringLiteral("-f") << job.plan.outputFormat
         << QStringLiteral("-m") << (profile.generateMipmaps ? QStringLiteral("0") : QStringLiteral("1"))
         << QStringLiteral("-if") << QStringLiteral("FANT");

    // DirectXTex BC1-BC3 compression is fixed-rate. Dithering can improve visual
    // quality at the same size after aggressive downscaling. BC7's `x` flag
    // enables its maximum compression search mode (quality/encode effort, not bytes).
    if (compressionLevel() > 0 &&
        (job.plan.outputFormat == QLatin1String("DXT1") || job.plan.outputFormat == QLatin1String("DXT5")))
        args << QStringLiteral("-bc") << QStringLiteral("d");
    else if (compressionLevel() == 2 && job.plan.outputFormat == QLatin1String("BC7_UNORM"))
        args << QStringLiteral("-bc") << QStringLiteral("x");

    if (job.plan.targetWidth != job.info.width)
        args << QStringLiteral("-w") << QString::number(job.plan.targetWidth);
    if (job.plan.targetHeight != job.info.height)
        args << QStringLiteral("-h") << QString::number(job.plan.targetHeight);

    args << QStringLiteral("-o") << tempBase << job.inputPath;

    statusBar()->showMessage(QStringLiteral("%1 / %2: %3").arg(m_queuePos + 1).arg(m_queue.size()).arg(native(job.relativePath)));
    m_process->setProgram(m_toolEdit->text().trimmed());
    m_process->setArguments(args);
    m_process->start();
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        return;
    TextureJob& job = m_jobs[m_currentJobIndex];
    const bool sortingWasEnabled = m_table->isSortingEnabled();
    m_table->setSortingEnabled(false);
    const int row = rowForJob(m_currentJobIndex);

    if (m_cancelRequested)
    {
        if (row >= 0) { m_table->item(row, 7)->setText(QStringLiteral("Отменено")); m_table->item(row, 7)->setData(SortRole, QStringLiteral("отменено")); }
    }
    else if (exitStatus != QProcess::NormalExit || exitCode != 0)
    {
        QString err = QString::fromLocal8Bit(m_process->readAllStandardError()).trimmed();
        const QString out = QString::fromLocal8Bit(m_process->readAllStandardOutput()).trimmed();
        if (!out.isEmpty())
            err += (err.isEmpty() ? QString() : QStringLiteral("\n")) + out;
        if (row >= 0) { m_table->item(row, 7)->setText(QStringLiteral("Ошибка texconv")); m_table->item(row, 7)->setData(SortRole, QStringLiteral("ошибка texconv")); }
        if (row >= 0) m_table->item(row, 7)->setToolTip(err);
        ++m_failCount;
    }
    else
    {
        const QString generated = findGeneratedFile(m_currentTempDir, job.inputPath);
        QString error;
        if (generated.isEmpty() || !commitOutput(job, generated, error))
        {
            if (row >= 0) { m_table->item(row, 7)->setText(QStringLiteral("Ошибка записи")); m_table->item(row, 7)->setData(SortRole, QStringLiteral("ошибка записи")); }
            if (row >= 0) m_table->item(row, 7)->setToolTip(error.isEmpty() ? QStringLiteral("texconv не создал ожидаемый DDS") : error);
            ++m_failCount;
        }
        else
        {
            const DdsInfo after = readDdsInfo(outputPathFor(job));
            if (row >= 0) { m_table->item(row, 7)->setText(QStringLiteral("Готово")); m_table->item(row, 7)->setData(SortRole, QStringLiteral("готово")); }
            if (after.valid && row >= 0)
            {
                m_table->item(row, 1)->setText(QStringLiteral("%1×%2").arg(after.width).arg(after.height));
                m_table->item(row, 1)->setData(SortRole, QVariant::fromValue(qulonglong(after.width) * qulonglong(after.height)));
                m_table->item(row, 2)->setText(after.format);
                m_table->item(row, 2)->setData(SortRole, after.format.toLower());
                m_table->item(row, 3)->setText(QString::number(after.mipCount));
                m_table->item(row, 3)->setData(SortRole, after.mipCount);
                m_table->item(row, 4)->setText(humanSize(after.fileSize));
                m_table->item(row, 4)->setData(SortRole, QVariant::fromValue(qulonglong(after.fileSize)));
                m_table->item(row, 7)->setToolTip(QStringLiteral("%1×%2, %3, mip %4, %5")
                    .arg(after.width).arg(after.height).arg(after.format).arg(after.mipCount).arg(humanSize(after.fileSize)));
            }
            ++m_successCount;
        }
    }

    m_table->setSortingEnabled(sortingWasEnabled);
    QDir(m_currentTempDir).removeRecursively();
    ++m_queuePos;
    m_progress->setValue(m_queuePos);
    m_currentJobIndex = -1;
    startNextJob();
}

void MainWindow::processError(QProcess::ProcessError errorCode)
{
    if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        return;

    TextureJob& job = m_jobs[m_currentJobIndex];
    Q_UNUSED(job);
    const bool sortingWasEnabled = m_table->isSortingEnabled();
    m_table->setSortingEnabled(false);
    const int row = rowForJob(m_currentJobIndex);
    if (row >= 0)
        m_table->item(row, 7)->setToolTip(m_process->errorString());

    if (errorCode == QProcess::FailedToStart)
    {
        if (row >= 0)
            { m_table->item(row, 7)->setText(QStringLiteral("texconv не запущен")); m_table->item(row, 7)->setData(SortRole, QStringLiteral("texconv не запущен")); }
        ++m_failCount;
        QDir(m_currentTempDir).removeRecursively();
        ++m_queuePos;
        m_progress->setValue(m_queuePos);
        m_currentJobIndex = -1;
        m_table->setSortingEnabled(sortingWasEnabled);
        startNextJob();
        return;
    }
    m_table->setSortingEnabled(sortingWasEnabled);
}

QString MainWindow::findGeneratedFile(const QString& tempDir, const QString& sourcePath) const
{
    const QString base = QFileInfo(sourcePath).completeBaseName();
    QDir dir(tempDir);
    const QFileInfoList files = dir.entryInfoList(QDir::Files);
    for (const QFileInfo& fi : files)
        if (fi.suffix().compare(QStringLiteral("dds"), Qt::CaseInsensitive) == 0 &&
            fi.completeBaseName().compare(base, Qt::CaseInsensitive) == 0)
            return fi.absoluteFilePath();
    return {};
}

QString MainWindow::outputPathFor(const TextureJob& job) const
{
    QString root = m_outputEdit->text().trimmed();
    if (root.isEmpty())
        root = m_sourceEdit->text().trimmed();
    return QDir(root).filePath(job.relativePath);
}

bool MainWindow::commitOutput(const TextureJob& job, const QString& generatedFile, QString& error)
{
    const QString target = outputPathFor(job);
    QDir().mkpath(QFileInfo(target).absolutePath());

    if (!m_sessionBackupRoot.isEmpty() && samePath(target, job.inputPath) && QFileInfo::exists(job.inputPath))
    {
        const QString backup = QDir(m_sessionBackupRoot).filePath(job.relativePath);
        QDir().mkpath(QFileInfo(backup).absolutePath());
        if (!QFile::copy(job.inputPath, backup))
        {
            error = QStringLiteral("Не удалось создать резервную копию: %1").arg(native(backup));
            return false;
        }
    }

    QFile src(generatedFile);
    if (!src.open(QIODevice::ReadOnly))
    {
        error = QStringLiteral("Не удалось открыть результат texconv");
        return false;
    }

    QSaveFile dst(target);
    if (!dst.open(QIODevice::WriteOnly))
    {
        error = QStringLiteral("Не удалось открыть файл назначения: %1").arg(native(target));
        return false;
    }

    QByteArray buffer;
    buffer.resize(1024 * 1024);
    while (!src.atEnd())
    {
        const qint64 n = src.read(buffer.data(), buffer.size());
        if (n < 0 || dst.write(buffer.constData(), n) != n)
        {
            dst.cancelWriting();
            error = QStringLiteral("Ошибка записи DDS");
            return false;
        }
    }
    if (!dst.commit())
    {
        error = QStringLiteral("Не удалось атомарно заменить DDS");
        return false;
    }
    return true;
}

void MainWindow::cancelOptimization()
{
    m_cancelRequested = true;
    if (m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

void MainWindow::setBusy(bool busy)
{
    m_scanButton->setEnabled(!busy);
    m_optimizeButton->setEnabled(!busy);
    m_cancelButton->setEnabled(busy);
    m_sourceEdit->setEnabled(!busy);
    m_outputEdit->setEnabled(!busy);
    m_toolEdit->setEnabled(!busy);
    m_profileCombo->setEnabled(!busy);
    m_compressionCombo->setEnabled(!busy);
}

QString MainWindow::humanSize(quint64 bytes) const
{
    static const char* units[] = { "B", "KB", "MB", "GB" };
    double value = double(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 3)
    {
        value /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', unit == 0 ? 0 : 1).arg(QString::fromLatin1(units[unit]));
}

void MainWindow::updateSummary()
{
    quint64 totalBytes = 0;
    int ready = 0;
    int risky = 0;
    for (const auto& j : m_jobs)
    {
        totalBytes += j.info.fileSize;
        if (j.plan.risky)
            ++risky;
        else if (j.plan.process)
            ++ready;
    }
    m_summaryLabel->setText(QStringLiteral("DDS: %1 • %2 • к обработке: %3 • вручную: %4")
                            .arg(m_jobs.size()).arg(humanSize(totalBytes)).arg(ready).arg(risky));
}
