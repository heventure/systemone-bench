#include "OpenVINOBackend.h"
#include <openvino/openvino.hpp>
#include <QElapsedTimer>
#include <cstring>

QStringList OpenVINOBackend::devices() const {
    QStringList r;
    try {
        ov::Core core;
        for (const auto& d : core.get_available_devices()) r << QString::fromStdString(d);
    } catch (...) {}
    return r;
}
QStringList OpenVINOBackend::modelFilters() const {
    return {"OpenVINO / ONNX models (*.xml *.onnx)"};
}

LocalRunResult OpenVINOBackend::run(const QString& modelPath, const QString& device, int warmup, int runs) {
    LocalRunResult rr;
    try {
        QElapsedTimer load; load.start();
        ov::Core core;
        auto model = core.read_model(modelPath.toStdString());
        auto compiled = core.compile_model(model, device.toStdString());
        auto request = compiled.create_infer_request();

        std::vector<ov::Tensor> tensors;
        const auto inputs = compiled.inputs();
        tensors.reserve(inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            const auto& port = inputs[i];
            if (port.get_partial_shape().is_dynamic()) {
                rr.error = QString("Input %1 has a dynamic shape. v0.2 zero-input mode requires fixed input shapes.").arg(i);
                return rr;
            }
            ov::Tensor t(port.get_element_type(), port.get_shape());
            std::memset(t.data(), 0, t.get_byte_size());
            request.set_input_tensor(i, t);
            tensors.push_back(t);
        }
        rr.loadMs = load.nsecsElapsed() / 1e6;

        auto once = [&]() {
            BenchSample s;
            QElapsedTimer t; t.start();
            request.infer();
            s.ms = t.nsecsElapsed() / 1e6;
            s.ok = true;
            return s;
        };
        for (int i = 0; i < warmup; ++i) once();
        QList<BenchSample> samples;
        samples.reserve(runs);
        for (int i = 0; i < runs; ++i) samples << once();

        rr.summary = Benchmark::summarize(samples);
        rr.ok = true;
        rr.details = QString("Backend: OpenVINO\nDevice: %1\nInputs: %2").arg(device).arg(inputs.size());
    } catch (const std::exception& e) {
        rr.error = QString::fromUtf8(e.what());
    }
    return rr;
}
