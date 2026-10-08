#include "RuntimeProbe.h"
#include <QSysInfo>
#include <QProcess>
#ifdef S1B_WITH_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif
#ifdef S1B_WITH_OPENVINO
#include <openvino/openvino.hpp>
#endif

QString RuntimeProbe::report(){
    QString s="OS: "+QSysInfo::prettyProductName()+"\nCPU arch: "+QSysInfo::currentCpuArchitecture()+"\n";
#ifdef S1B_WITH_ONNXRUNTIME
    s+="ONNX Runtime: bundled "+QString::fromUtf8(OrtGetApiBase()->GetVersionString())+"\nONNX Runtime providers: CPU\n";
#endif
#ifdef Q_OS_WIN
    s+="Platform: Windows\n";
#ifdef S1B_WITH_OPENVINO
    s+="OpenVINO: compiled in\n";
    try {
        ov::Core core;
        QStringList ds;
        for(const auto& d:core.get_available_devices()) ds << QString::fromStdString(d);
        s+="OpenVINO devices: "+ds.join(", ")+"\n";
    } catch(const std::exception& e) {
        s+="OpenVINO probe error: "+QString::fromUtf8(e.what())+"\n";
    }
#else
    s+="OpenVINO: not included in this build\n";
#endif
    QProcess p;
    p.start("powershell",{"-NoProfile","-Command",
        "Get-CimInstance Win32_Processor | Select-Object -ExpandProperty Name; "
        "Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name"});
    if(p.waitForFinished(3000)) s+="Windows devices:\n"+QString::fromLocal8Bit(p.readAllStandardOutput());
#elif defined(Q_OS_MACOS)
    s+="Platform: macOS\nCore ML: available\nMetal: available\n";
    QProcess p;
    p.start("system_profiler",{"SPHardwareDataType","SPDisplaysDataType"});
    if(p.waitForFinished(5000)) s+=QString::fromLocal8Bit(p.readAllStandardOutput());
#endif
    return s;
}
