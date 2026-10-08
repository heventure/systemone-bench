#include "LocalBackend.h"
#ifdef S1B_WITH_ONNXRUNTIME
#include "OnnxRuntimeBackend.h"
#endif

#ifdef Q_OS_MACOS
#include "CoreMLBackend.h"
#endif
#ifdef S1B_WITH_OPENVINO
#include "OpenVINOBackend.h"
#endif

QStringList availableLocalBackends() {
    QStringList r;
#ifdef S1B_WITH_ONNXRUNTIME
    r << "ONNX Runtime";
#endif
#ifdef Q_OS_MACOS
    r << "Core ML";
#endif
#ifdef S1B_WITH_OPENVINO
    r << "OpenVINO";
#endif
    return r;
}

std::unique_ptr<LocalBackend> createLocalBackend(const QString& name) {
#ifdef S1B_WITH_ONNXRUNTIME
    if (name == "ONNX Runtime") return std::make_unique<OnnxRuntimeBackend>();
#endif
#ifdef Q_OS_MACOS
    if (name == "Core ML") return std::make_unique<CoreMLBackend>();
#endif
#ifdef S1B_WITH_OPENVINO
    if (name == "OpenVINO") return std::make_unique<OpenVINOBackend>();
#endif
    return {};
}
