#pragma once
#include "LocalBackend.h"

class OpenVINOBackend final : public LocalBackend {
public:
    QString name() const override { return "OpenVINO"; }
    QStringList devices() const override;
    QStringList modelFilters() const override;
    LocalRunResult run(const QString& modelPath, const QString& device,
                       int warmup, int runs) override;
};
