#include "RuntimeProbe.h"
#include <QSysInfo>
#include <QProcess>
QString RuntimeProbe::report(){
 QString s="OS: "+QSysInfo::prettyProductName()+"\nCPU arch: "+QSysInfo::currentCpuArchitecture()+"\n";
#ifdef Q_OS_WIN
 s+="Platform: Windows\nRecommended local accelerators: OpenVINO CPU/GPU/NPU; ONNX Runtime; CUDA adapter.\n";
 QProcess p; p.start("powershell",{"-NoProfile","-Command","Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name"});
 if(p.waitForFinished(3000)) s+="Display/accelerators:\n"+QString::fromLocal8Bit(p.readAllStandardOutput());
#elif defined(Q_OS_MACOS)
 s+="Platform: macOS\nRecommended local accelerators: Core ML CPU/GPU/ANE; Metal adapters.\n";
 QProcess p; p.start("system_profiler",{"SPHardwareDataType","SPDisplaysDataType"});
 if(p.waitForFinished(5000)) s+=QString::fromLocal8Bit(p.readAllStandardOutput());
#endif
 return s;
}
