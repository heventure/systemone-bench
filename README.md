# SystemOne Bench

Cross-platform benchmark workbench for **non-generative System-1 / decision models**.

## v0.1
- Qt 6 native desktop UI on Windows and macOS.
- Arbitrary HTTP payloads for `/v1/systemone` and `/v1/decisions`.
- Warm-up + measured runs with mean/P50/P95/P99/min/max.
- Hardware/runtime probe.
- GitHub Actions builds downloadable Windows x64 and macOS arm64 artifacts.

The benchmark intentionally measures **request/forward latency**, not token/s.

## Planned local backends
- Windows: OpenVINO CPU/GPU/**Intel NPU**, ONNX Runtime, CUDA.
- macOS: Core ML CPU/GPU/**ANE**, Metal.
- Model-family adapters for Decision/Decider/Laya/OneJev-style typed heads.
- Text, vision and GUI-agent suites; calibration/ECE/Brier and probability drift.

## Build locally
Qt 6.5+ and CMake 3.24+:
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build --config Release
```

You do not need a local compiler to use releases: GitHub Actions builds both platforms.
