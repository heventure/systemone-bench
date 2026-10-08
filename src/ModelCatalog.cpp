#include "ModelCatalog.h"
#include "LocalBackend.h"
#include <QSysInfo>
#include <QStorageInfo>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_MACOS
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

HardwareProfile ModelCatalog::probe(){
    HardwareProfile h;
    h.os = QSysInfo::productType();
    h.arch = QSysInfo::currentCpuArchitecture();
    h.runtimes = availableLocalBackends();
    for(const auto& n:h.runtimes)
        if(auto b=createLocalBackend(n)) h.devices.append(b->devices());
#ifdef Q_OS_WIN
    MEMORYSTATUSEX m{}; m.dwLength=sizeof(m);
    if(GlobalMemoryStatusEx(&m)) h.memoryBytes=m.ullTotalPhys;
#elif defined(Q_OS_MACOS)
    uint64_t mem=0; size_t len=sizeof(mem);
    if(sysctlbyname("hw.memsize",&mem,&len,nullptr,0)==0) h.memoryBytes=mem;
#endif
    return h;
}

QList<CatalogModel> ModelCatalog::models(){
    // Curated manifest v1. URLs are deliberately explicit and can later move to a remote signed manifest.
    return {
      {"decima-small","Decima Small","Decima","typed decision","ONNX",
       "https://huggingface.co/amyrmahdy/decima-small/resolve/main/model.onnx","decima-small.onnx","",0,2,
       {"OpenVINO"},{"CPU","NPU"},"decima","True System-1 model; semantic benchmark requires the Decima adapter.",false},
      {"s1-auto-router","S1 LLM Auto Router","System1 Models","routing decision","ONNX",
       "https://huggingface.co/system1models/s1-llm-auto-router/resolve/main/model.onnx","s1-llm-auto-router.onnx","",0,1,
       {"OpenVINO"},{"CPU","NPU"},"s1-router","Small multi-head System-1 router; adapter required for semantic inputs.",false},
      {"mobilenet-smoke","MobileNetV3 Small (runtime smoke test)","MobileNetV3","vision smoke test","ONNX",
       "https://huggingface.co/pyronear/mobilenet_v3_small/resolve/main/model.onnx","mobilenet-v3-small.onnx",
       "8fd451f919499e30e879eda19bfd2b249ceec77e4f1c02f7b022730e365ae897",6070000,1,
       {"OpenVINO"},{"CPU","GPU","NPU"},"raw-fixed","Not a System-1 model. Included only to validate accelerator/runtime execution.",true}
    };
}

int ModelCatalog::score(const CatalogModel& m,const HardwareProfile& h){
    int s=0;
    for(const auto&r:m.runtimes) if(h.runtimes.contains(r,Qt::CaseInsensitive)) s+=40;
    for(const auto&d:m.devices) for(const auto&hd:h.devices) if(hd.contains(d,Qt::CaseInsensitive)){s+=15;break;}
    const quint64 gb=h.memoryBytes/(1024ull*1024ull*1024ull);
    if(!m.minMemoryGB || gb>=quint64(m.minMemoryGB)) s+=20; else s-=100;
    if(m.adapter=="raw-fixed") s+=10;
    if(m.smokeTest) s-=5; // Prefer real System-1 when a semantic adapter exists.
    return s;
}
QString ModelCatalog::reason(const CatalogModel&m,const HardwareProfile&h){
    QStringList x;
    for(const auto&r:m.runtimes) if(h.runtimes.contains(r,Qt::CaseInsensitive)) x<<"runtime "+r+" available";
    for(const auto&d:m.devices) for(const auto&hd:h.devices) if(hd.contains(d,Qt::CaseInsensitive)){x<<"device "+hd+" matches";break;}
    if(m.adapter!="raw-fixed") x<<"semantic adapter: "+m.adapter;
    if(m.smokeTest) x<<"runtime smoke test only";
    return x.isEmpty() ? "No direct local runtime match" : x.join("; ");
}
QString ModelCatalog::cacheDir(){
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/models";
}
