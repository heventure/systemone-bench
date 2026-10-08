#pragma once
#include "LocalBackend.h"

class OnnxRuntimeBackend final : public LocalBackend {
public:
    QString name() const override { return "ONNX Runtime"; }
    QStringList devices() const override;
    QStringList modelFilters() const override { return {"ONNX models (*.onnx)"}; }
    LocalRunResult run(const QString& modelPath, const QString& device, int warmup, int runs) override;
};
