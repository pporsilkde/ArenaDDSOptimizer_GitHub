#pragma once

#include "ddsinfo.h"
#include "optimizerprofile.h"

#include <QMainWindow>
#include <QProcess>
#include <QVector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QProgressBar;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void browseSource();
    void browseOutput();
    void browseTool();
    void scanTextures();
    void optimizeTextures();
    void cancelOptimization();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void profileChanged(int index);

private:
    struct TextureJob
    {
        QString inputPath;
        QString relativePath;
        DdsInfo info;
        OptimizationPlan plan;
        int row = -1;
    };

    void buildUi();
    void loadSettings();
    void saveSettings();
    void refreshPlans();
    void startNextJob();
    bool commitOutput(const TextureJob& job, const QString& generatedFile, QString& error);
    QString findGeneratedFile(const QString& tempDir, const QString& sourcePath) const;
    QString outputPathFor(const TextureJob& job) const;
    QString humanSize(quint64 bytes) const;
    OptimizerProfile currentProfile() const;
    void setBusy(bool busy);
    void updateSummary();
    bool validateTexconv(QString& error) const;

    QLineEdit* m_sourceEdit = nullptr;
    QLineEdit* m_outputEdit = nullptr;
    QLineEdit* m_toolEdit = nullptr;
    QComboBox* m_profileCombo = nullptr;
    QCheckBox* m_recursiveCheck = nullptr;
    QCheckBox* m_backupCheck = nullptr;
    QCheckBox* m_forceCheck = nullptr;
    QCheckBox* m_dryRunCheck = nullptr;
    QPushButton* m_scanButton = nullptr;
    QPushButton* m_optimizeButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QProgressBar* m_progress = nullptr;

    QVector<OptimizerProfile> m_profiles;
    QVector<TextureJob> m_jobs;
    QVector<int> m_queue;
    int m_queuePos = 0;
    int m_successCount = 0;
    int m_failCount = 0;
    bool m_cancelRequested = false;
    QString m_sessionBackupRoot;
    QString m_currentTempDir;
    int m_currentJobIndex = -1;
    QProcess* m_process = nullptr;
};
