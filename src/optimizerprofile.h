#pragma once

#include "ddsinfo.h"

#include <QString>
#include <QVector>

struct OptimizerProfile
{
    QString id;
    QString displayName;
    int maxDimension = 4096;
    bool generateMipmaps = true;
    bool compatibilityFormatsOnly = true;
    bool convertDxt3ToDxt5 = true;
    bool allowBc7 = false;
    int jpegLikeQualityHint = 0; // reserved for future backends
};

struct OptimizationPlan
{
    bool process = false;
    bool risky = false;
    QString outputFormat;
    int targetWidth = 0;
    int targetHeight = 0;
    QString reason;
};

QVector<OptimizerProfile> builtInProfiles();
OptimizationPlan buildPlan(const QString& path, const DdsInfo& info, const OptimizerProfile& profile,
                           bool forceReencode, int compressionLevel = 0);
