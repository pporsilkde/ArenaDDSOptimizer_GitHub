#include "optimizerprofile.h"

#include <QFileInfo>
#include <QtMath>

namespace
{
bool looksLikeNormalMap(const QString& path)
{
    const QString n = QFileInfo(path).completeBaseName().toLower();
    return n.endsWith(QStringLiteral("_n")) ||
           n.endsWith(QStringLiteral("_nm")) ||
           n.endsWith(QStringLiteral("_normal")) ||
           n.contains(QStringLiteral("normal")) ||
           n.contains(QStringLiteral("normals"));
}

bool looksLikeAlphaTexture(const QString& path)
{
    const QString p = path.toLower();
    const QString n = QFileInfo(path).completeBaseName().toLower();
    return p.contains(QStringLiteral("menu")) || p.contains(QStringLiteral("icons")) ||
           p.contains(QStringLiteral("gui")) || p.contains(QStringLiteral("hud")) ||
           n.contains(QStringLiteral("alpha")) || n.endsWith(QStringLiteral("_a"));
}

int roundedBlockDimension(int value)
{
    return qMax(4, ((value + 3) / 4) * 4);
}

bool isLegacySafe(const QString& fmt)
{
    return fmt.startsWith(QStringLiteral("DXT1")) || fmt.startsWith(QStringLiteral("DXT5"));
}
}

QVector<OptimizerProfile> builtInProfiles()
{
    return {
        { QStringLiteral("safe"), QStringLiteral("Windows + Android — безопасный"), 4096, true, true, true, false, 0 },
        { QStringLiteral("android"), QStringLiteral("Android / ng-gl4es — производительность"), 2048, true, true, true, false, 0 },
        { QStringLiteral("windows"), QStringLiteral("Windows — качество"), 8192, true, true, true, false, 0 },
        { QStringLiteral("windows_bc7"), QStringLiteral("Windows — BC7 (расширенный)"), 8192, true, false, true, true, 0 }
    };
}

OptimizationPlan buildPlan(const QString& path, const DdsInfo& info, const OptimizerProfile& profile, bool forceReencode)
{
    OptimizationPlan plan;
    if (!info.valid)
    {
        plan.reason = QStringLiteral("Пропуск: некорректный DDS");
        return plan;
    }

    plan.targetWidth = info.width;
    plan.targetHeight = info.height;

    const int largest = qMax(info.width, info.height);
    if (largest > profile.maxDimension)
    {
        const double scale = double(profile.maxDimension) / double(largest);
        plan.targetWidth = roundedBlockDimension(qRound(info.width * scale));
        plan.targetHeight = roundedBlockDimension(qRound(info.height * scale));
    }

    const bool normal = looksLikeNormalMap(path);
    const bool alphaHint = info.hasAlphaCapability || looksLikeAlphaTexture(path) || normal;

    if (profile.allowBc7)
    {
        plan.outputFormat = QStringLiteral("BC7_UNORM");
    }
    else
    {
        if (info.format.startsWith(QStringLiteral("BC4")) || info.format.startsWith(QStringLiteral("BC5")) ||
            info.format.startsWith(QStringLiteral("BC6")) || info.format.startsWith(QStringLiteral("BC7")))
        {
            // Modern single/two-channel and HDR formats may have shader-specific semantics.
            // Do not silently reinterpret them in the compatibility profile.
            plan.risky = true;
            plan.reason = QStringLiteral("Современный BC-формат: автоматическая конверсия отключена");
            return plan;
        }
        plan.outputFormat = alphaHint ? QStringLiteral("DXT5") : QStringLiteral("DXT1");
    }

    const bool resize = plan.targetWidth != info.width || plan.targetHeight != info.height;
    const bool needsMips = profile.generateMipmaps && info.mipCount <= 1;
    const bool formatChange = profile.allowBc7 ||
        (plan.outputFormat == QLatin1String("DXT1") && !info.format.startsWith(QStringLiteral("DXT1"))) ||
        (plan.outputFormat == QLatin1String("DXT5") && !info.format.startsWith(QStringLiteral("DXT5")));

    plan.process = forceReencode || resize || needsMips || formatChange;
    if (!plan.process && isLegacySafe(info.format))
    {
        plan.reason = QStringLiteral("Уже оптимально для выбранного профиля");
        return plan;
    }

    QStringList why;
    if (resize)
        why << QStringLiteral("%1x%2 → %3x%4").arg(info.width).arg(info.height).arg(plan.targetWidth).arg(plan.targetHeight);
    if (needsMips)
        why << QStringLiteral("добавить mipmaps");
    if (formatChange || forceReencode)
        why << QStringLiteral("→ %1").arg(plan.outputFormat);
    if (normal)
        why << QStringLiteral("normal-map: совместимый BC3/DXT5");
    plan.reason = why.isEmpty() ? QStringLiteral("переупаковка") : why.join(QStringLiteral(", "));
    return plan;
}
