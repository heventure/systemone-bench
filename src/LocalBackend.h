#pragma once
#include "Benchmark.h"
#include <QString>
#include <QStringList>
#include <memory>

struct LocalRunResult {
    bool ok = false;
    QString error;
    QString details;
    double loadMs = 0;
    BenchSummary summary;
};

class LocalBackend {
public:
    virtual ~LocalBackend() = default;
    virtual QString name() const = 0;
    virtual QStringList devices() const = 0;
    virtual QStringList modelFilters() const = 0;
    virtual LocalRunResult run(const QString& modelPath, const QString& device,
                               int warmup, int runs) = 0;
};

QStringList availableLocalBackends();
std::unique_ptr<LocalBackend> createLocalBackend(const QString& name);
