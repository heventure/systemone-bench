#include "OnnxRuntimeBackend.h"
#include <onnxruntime_cxx_api.h>
#include <QElapsedTimer>
#include <QFileInfo>
#include <numeric>

QStringList OnnxRuntimeBackend::devices() const {
    // Keep the first bundled implementation deliberately portable. Provider-specific
    // acceleration can be added without changing the backend contract.
    return {"CPU"};
}

static size_t elementSize(ONNXTensorElementDataType t) {
    switch(t){
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: return 4;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: return 8;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: return 8;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: return 4;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: return 1;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8: return 1;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL: return 1;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16: return 2;
        default: return 0;
    }
}

LocalRunResult OnnxRuntimeBackend::run(const QString& modelPath,const QString& device,int warmup,int runs){
    LocalRunResult rr;
    try {
        QElapsedTimer load; load.start();
        static Ort::Env env(ORT_LOGGING_LEVEL_WARNING,"systemone-bench");
        Ort::SessionOptions options;
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef Q_OS_WIN
        std::wstring wp=modelPath.toStdWString();
        Ort::Session session(env,wp.c_str(),options);
#else
        QByteArray p=QFile::encodeName(modelPath);
        Ort::Session session(env,p.constData(),options);
#endif
        rr.loadMs=load.nsecsElapsed()/1e6;

        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<std::string> inputNamesStorage;
        std::vector<const char*> inputNames;
        std::vector<std::vector<unsigned char>> buffers;
        std::vector<Ort::Value> inputs;
        QStringList details;

        for(size_t i=0;i<session.GetInputCount();++i){
            auto n=session.GetInputNameAllocated(i,allocator);
            inputNamesStorage.emplace_back(n.get());
            auto info=session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
            auto shape=info.GetShape();
            for(auto d:shape) if(d<=0) throw std::runtime_error("Dynamic ONNX input shapes are not supported yet.");
            const auto type=info.GetElementType();
            const size_t es=elementSize(type);
            if(!es) throw std::runtime_error("Unsupported ONNX input tensor type.");
            size_t count=1; for(auto d:shape) count*=static_cast<size_t>(d);
            buffers.emplace_back(count*es,0);
            auto mem=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
            inputs.emplace_back(Ort::Value::CreateTensor(mem,buffers.back().data(),buffers.back().size(),shape.data(),shape.size(),type));
            details << QString::fromStdString(inputNamesStorage.back());
        }
        for(auto& n:inputNamesStorage) inputNames.push_back(n.c_str());

        std::vector<std::string> outputNamesStorage;
        std::vector<const char*> outputNames;
        for(size_t i=0;i<session.GetOutputCount();++i){ auto n=session.GetOutputNameAllocated(i,allocator); outputNamesStorage.emplace_back(n.get()); }
        for(auto& n:outputNamesStorage) outputNames.push_back(n.c_str());

        auto infer=[&](){ auto out=session.Run(Ort::RunOptions{nullptr},inputNames.data(),inputs.data(),inputs.size(),outputNames.data(),outputNames.size()); };
        for(int i=0;i<warmup;++i) infer();
        QList<BenchSample> samples;
        samples.reserve(runs);
        for(int i=0;i<runs;++i){ QElapsedTimer t;t.start(); infer(); BenchSample sample; sample.ms=t.nsecsElapsed()/1e6; sample.ok=true; sample.status=200; samples.push_back(sample); }
        rr.summary=Benchmark::summarize(samples);
        rr.ok=true;
        rr.details=QString("Backend: ONNX Runtime\nExecution provider: %1\nInputs: %2").arg(device,details.join(", "));
    } catch(const Ort::Exception& e){ rr.error=QString::fromUtf8(e.what()); }
      catch(const std::exception& e){ rr.error=QString::fromUtf8(e.what()); }
    return rr;
}
