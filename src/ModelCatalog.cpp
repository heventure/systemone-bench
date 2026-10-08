#include "ModelCatalog.h"
#include "LocalBackend.h"
#include <QSysInfo>
#include <QStandardPaths>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_MACOS
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

HardwareProfile ModelCatalog::probe(){
    HardwareProfile h;
    h.os=QSysInfo::productType(); h.arch=QSysInfo::currentCpuArchitecture();
    h.runtimes=availableLocalBackends();
    for(const auto& n:h.runtimes) if(auto b=createLocalBackend(n)) h.devices.append(b->devices());
#ifdef Q_OS_WIN
    MEMORYSTATUSEX m{}; m.dwLength=sizeof(m); if(GlobalMemoryStatusEx(&m)) h.memoryBytes=m.ullTotalPhys;
#elif defined(Q_OS_MACOS)
    uint64_t mem=0; size_t len=sizeof(mem); if(sysctlbyname("hw.memsize",&mem,&len,nullptr,0)==0) h.memoryBytes=mem;
#endif
    return h;
}

QList<CatalogModel> ModelCatalog::models(){
    return {
      {"s1-auto-router","S1 LLM Auto Router","System1 Models","fixed-schema routing decision",1,
       {"CPU","NPU"},"s1-router",
       "True seven-head System-1 router. Semantic adapter is the next implementation target.",false,false,
       {{"onnx-int8","OpenVINO","ONNX",
         "https://huggingface.co/system1models/s1-llm-auto-router/resolve/main/model.gq.onnx",
         "model.gq.onnx","",183000000,false,
         "Requires tokenizer/tokenizer.json and meta.json; artifact is downloadable but not benchmark-ready until the adapter bundle lands."}}},
      {"decima-small","Decima Small 1.1","Decima","typed decision",2,
       {"CPU","NPU"},"decima",
       "122M typed System-1 model. Choice, verify, score and rank require the Decima semantic adapter.",false,false,
       {{"onnx-int8","OpenVINO","ONNX","",
         "onnx/int8/model.onnx","",127000000,false,
         "Preview only: multi-file snapshot and Decima adapter required."}}},
      {"mobilenet-smoke","MobileNetV3 Small (runtime smoke test)","MobileNetV3","vision smoke test",1,
       {"CPU","GPU","NPU"},"raw-fixed",
       "Not a System-1 model; only validates OpenVINO accelerator/runtime execution.",true,true,
       {{"onnx","OpenVINO","ONNX",
         "https://huggingface.co/pyronear/mobilenet_v3_small/resolve/main/model.onnx",
         "mobilenet-v3-small.onnx",
         "8fd451f919499e30e879eda19bfd2b249ceec77e4f1c02f7b022730e365ae897",6070000,true,
         "Fixed-shape zero-input runtime smoke test."}}}
    };
}

QList<ModelArtifact> ModelCatalog::compatibleArtifacts(const CatalogModel&m,const HardwareProfile&h){
    QList<ModelArtifact> out;
    for(const auto&a:m.artifacts)
        if(h.runtimes.contains(a.runtime,Qt::CaseInsensitive)) out<<a;
    return out;
}
int ModelCatalog::score(const CatalogModel&m,const HardwareProfile&h){
    const quint64 gb=h.memoryBytes/(1024ull*1024ull*1024ull);
    if(m.minMemoryGB && gb<quint64(m.minMemoryGB)) return -100;
    const auto arts=compatibleArtifacts(m,h);
    if(arts.isEmpty()) return 0;
    bool ready=false; for(const auto&a:arts) ready|=a.benchmarkReady;
    int s=20;
    for(const auto&d:m.devices) for(const auto&hd:h.devices)
        if(hd.contains(d,Qt::CaseInsensitive)){s+=10;break;}
    if(m.adapterAvailable && ready) s+=60;
    else if(ready) s+=35;
    else s+=5;
    if(m.smokeTest) s-=10;
    return s;
}
QString ModelCatalog::status(const CatalogModel&m,const HardwareProfile&h){
    const int s=score(m,h);
    if(s<0) return "Insufficient memory";
    const auto arts=compatibleArtifacts(m,h);
    if(arts.isEmpty()) return "No compatible runtime";
    bool ready=false; for(const auto&a:arts) ready|=a.benchmarkReady;
    if(m.adapterAvailable && ready && !m.smokeTest) return "Recommended";
    if(ready && m.smokeTest) return "Runtime smoke test";
    return "Preview / adapter required";
}
QString ModelCatalog::reason(const CatalogModel&m,const HardwareProfile&h){
    QStringList x;
    const auto arts=compatibleArtifacts(m,h);
    for(const auto&a:arts) x<<QString("%1/%2").arg(a.runtime,a.format);
    for(const auto&d:m.devices) for(const auto&hd:h.devices)
        if(hd.contains(d,Qt::CaseInsensitive)){x<<"device "+hd+" matches";break;}
    if(!m.adapterAvailable && m.adapter!="raw-fixed") x<<"adapter "+m.adapter+" not installed";
    bool ready=false; for(const auto&a:arts) ready|=a.benchmarkReady;
    if(!ready && !arts.isEmpty()) x<<"artifact is not benchmark-ready";
    if(m.smokeTest) x<<"runtime smoke test only";
    return x.isEmpty()?"No runnable artifact for this machine":x.join("; ");
}
QString ModelCatalog::cacheDir(){
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/models";
}
