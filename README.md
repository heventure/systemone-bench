# SystemOne Bench

Cross-platform benchmark workbench for **non-generative System-1 / decision models**.

## v0.2

### Backends
- **HTTP API** — arbitrary JSON requests to endpoints such as `/v1/systemone` and `/v1/decisions`.
- **Core ML (macOS)** — local `.mlmodel`, `.mlpackage`, or `.mlmodelc` inference.
  - CPU only
  - CPU + GPU
  - CPU + Neural Engine
  - All / Core ML automatic placement
- **OpenVINO (Windows)** — local OpenVINO IR (`.xml`) and ONNX (`.onnx`) inference.
  - Enumerates the actual OpenVINO devices available on the machine, e.g. CPU / GPU / NPU.

### Metrics
The benchmark measures request/forward latency, not token generation speed:
- model compile/load time
- warm-up runs
- measured runs
- mean
- P50 / P95 / P99
- min / max

## Local zero-input benchmark mode

v0.2 automatically creates zero-valued inputs from the model's declared fixed input shapes. This isolates the inference runtime/device path from tokenizer, image preprocessing and application logic.

Dynamic-shape models and model-specific semantic inputs are the next adapter layer. Planned suites will add:
- text decision cases
- vision / GUI-agent cases
- accuracy
- probability drift
- Brier score
- ECE calibration
- preprocessing and end-to-end latency

## Windows NPU

The GitHub Actions Windows artifact is built with OpenVINO and bundles its runtime/plugins. On a supported Intel Core Ultra system with the correct NPU driver, **Probe hardware** and the OpenVINO Device selector should show `NPU`.

## macOS ANE

Core ML exposes compute-unit policies rather than a promise that every operation executes exclusively on the Neural Engine. SystemOne Bench therefore records the **requested Core ML compute units**. A future instrumentation layer will report execution placement when available.

## Build locally

Qt 6.5+, CMake 3.26+.

```sh
cmake -S . -B build
cmake --build build --config Release
```

On Windows, install OpenVINO and provide `OpenVINO_DIR` for the local OpenVINO backend.

GitHub Actions builds downloadable Windows x64 and macOS arm64 artifacts automatically.
