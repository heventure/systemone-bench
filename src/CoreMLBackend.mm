#include "CoreMLBackend.h"
#import <CoreML/CoreML.h>
#import <CoreVideo/CoreVideo.h>
#include <QElapsedTimer>
#include <QFileInfo>

static NSString* ns(const QString& s) {
    QByteArray u = s.toUtf8();
    return [NSString stringWithUTF8String:u.constData()];
}
static QString qs(NSError* e) {
    return e ? QString::fromUtf8(e.localizedDescription.UTF8String) : QString();
}
static MLComputeUnits computeUnits(const QString& d) {
    if (d == "CPU only") return MLComputeUnitsCPUOnly;
    if (d == "CPU + GPU") return MLComputeUnitsCPUAndGPU;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
    if (d == "CPU + Neural Engine") return MLComputeUnitsCPUAndNeuralEngine;
#endif
    return MLComputeUnitsAll;
}

QStringList CoreMLBackend::devices() const {
    QStringList r{"CPU only", "CPU + GPU"};
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
    if (@available(macOS 13.0, *)) r << "CPU + Neural Engine";
#endif
    r << "All (Core ML chooses)";
    return r;
}
QStringList CoreMLBackend::modelFilters() const {
    return {"Core ML models (*.mlmodelc *.mlpackage *.mlmodel)"};
}

static id<MLFeatureProvider> zeroProvider(MLModel* model, QString& errorText) {
    NSMutableDictionary<NSString*, MLFeatureValue*>* values = [NSMutableDictionary dictionary];
    NSDictionary<NSString*, MLFeatureDescription*>* inputs = model.modelDescription.inputDescriptionsByName;

    for (NSString* key in inputs) {
        MLFeatureDescription* d = inputs[key];
        NSError* err = nil;
        switch (d.type) {
            case MLFeatureTypeMultiArray: {
                MLMultiArrayConstraint* c = d.multiArrayConstraint;
                if (!c || !c.shape) { errorText = "Dynamic/unknown Core ML multi-array shape is not supported by zero-input mode."; return nil; }
                MLMultiArray* a = [[MLMultiArray alloc] initWithShape:c.shape dataType:c.dataType error:&err];
                if (!a || err) { errorText = qs(err); return nil; }
                for (NSInteger i = 0; i < a.count; ++i) a[i] = @0;
                values[key] = [MLFeatureValue featureValueWithMultiArray:a];
                break;
            }
            case MLFeatureTypeDouble:
                values[key] = [MLFeatureValue featureValueWithDouble:0.0];
                break;
            case MLFeatureTypeInt64:
                values[key] = [MLFeatureValue featureValueWithInt64:0];
                break;
            case MLFeatureTypeString:
                values[key] = [MLFeatureValue featureValueWithString:@""];
                break;
            case MLFeatureTypeImage: {
                MLImageConstraint* c = d.imageConstraint;
                if (!c || c.pixelsWide <= 0 || c.pixelsHigh <= 0) {
                    errorText = "Dynamic/unknown Core ML image size is not supported by zero-input mode."; return nil;
                }
                CVPixelBufferRef pb = nullptr;
                CVReturn cr = CVPixelBufferCreate(kCFAllocatorDefault, c.pixelsWide, c.pixelsHigh,
                                                  kCVPixelFormatType_32BGRA, nullptr, &pb);
                if (cr != kCVReturnSuccess || !pb) { errorText = "Failed to allocate Core ML image input."; return nil; }
                CVPixelBufferLockBaseAddress(pb, 0);
                memset(CVPixelBufferGetBaseAddress(pb), 0, CVPixelBufferGetDataSize(pb));
                CVPixelBufferUnlockBaseAddress(pb, 0);
                values[key] = [MLFeatureValue featureValueWithPixelBuffer:pb];
                CVPixelBufferRelease(pb);
                break;
            }
            default:
                errorText = QString("Unsupported Core ML input feature type for '%1'.").arg(QString::fromUtf8(key.UTF8String));
                return nil;
        }
    }

    NSError* err = nil;
    MLDictionaryFeatureProvider* p = [[MLDictionaryFeatureProvider alloc] initWithDictionary:values error:&err];
    if (!p || err) { errorText = qs(err); return nil; }
    return p;
}

LocalRunResult CoreMLBackend::run(const QString& modelPath, const QString& device, int warmup, int runs) {
    LocalRunResult rr;
    @autoreleasepool {
        NSError* err = nil;
        NSURL* source = [NSURL fileURLWithPath:ns(modelPath)];
        NSURL* compiledURL = source;
        const QString suffix = QFileInfo(modelPath).suffix().toLower();

        QElapsedTimer loadTimer; loadTimer.start();
        if (suffix != "mlmodelc") {
            compiledURL = [MLModel compileModelAtURL:source error:&err];
            if (!compiledURL || err) { rr.error = "Core ML compile failed: " + qs(err); return rr; }
        }

        MLModelConfiguration* cfg = [[MLModelConfiguration alloc] init];
        cfg.computeUnits = computeUnits(device);
        MLModel* model = [MLModel modelWithContentsOfURL:compiledURL configuration:cfg error:&err];
        if (!model || err) { rr.error = "Core ML load failed: " + qs(err); return rr; }
        rr.loadMs = loadTimer.nsecsElapsed() / 1e6;

        QString providerError;
        id<MLFeatureProvider> provider = zeroProvider(model, providerError);
        if (!provider) { rr.error = providerError; return rr; }

        auto inferOnce = [&]() -> BenchSample {
            BenchSample s;
            NSError* e = nil;
            QElapsedTimer t; t.start();
            id<MLFeatureProvider> out = [model predictionFromFeatures:provider error:&e];
            s.ms = t.nsecsElapsed() / 1e6;
            s.ok = out != nil && e == nil;
            if (!s.ok && e) s.body = QByteArray(e.localizedDescription.UTF8String);
            return s;
        };

        for (int i = 0; i < warmup; ++i) {
            auto s = inferOnce();
            if (!s.ok) { rr.error = "Core ML warmup failed: " + QString::fromUtf8(s.body); return rr; }
        }
        QList<BenchSample> samples;
        samples.reserve(runs);
        for (int i = 0; i < runs; ++i) {
            auto s = inferOnce();
            if (!s.ok) { rr.error = "Core ML inference failed: " + QString::fromUtf8(s.body); return rr; }
            samples << s;
        }
        rr.summary = Benchmark::summarize(samples);
        rr.ok = true;

        QStringList inputLines;
        for (NSString* key in model.modelDescription.inputDescriptionsByName) {
            MLFeatureDescription* d = model.modelDescription.inputDescriptionsByName[key];
            inputLines << QString("%1 (type %2)").arg(QString::fromUtf8(key.UTF8String)).arg((int)d.type);
        }
        rr.details = QString("Backend: Core ML\nRequested compute units: %1\nInputs: %2")
            .arg(device, inputLines.join(", "));
    }
    return rr;
}
