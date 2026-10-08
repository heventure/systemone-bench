#include "ModelManagerDialog.h"
#include <QtWidgets>
#include <QtNetwork>
#include <QCryptographicHash>
#include <QSaveFile>
#include <algorithm>
#include <memory>
#include <functional>

ModelManagerDialog::ModelManagerDialog(QWidget* parent):QDialog(parent),hw_(ModelCatalog::probe()),models_(ModelCatalog::models()){
    setWindowTitle("Model catalog & recommendations"); resize(900,480);
    auto*v=new QVBoxLayout(this);
    const auto gb=hw_.memoryBytes/(1024.0*1024*1024);
    v->addWidget(new QLabel(QString("Detected: %1 / %2 / %3 GB RAM\nRuntimes: %4\nDevices: %5")
      .arg(hw_.os,hw_.arch).arg(gb,0,'f',1).arg(hw_.runtimes.join(", "),hw_.devices.join(", "))));
    table_=new QTableWidget(models_.size(),6); table_->setHorizontalHeaderLabels({"Model","Type","Format","Recommendation","Compatibility","Notes"});
    std::sort(models_.begin(),models_.end(),[&](auto&a,auto&b){return ModelCatalog::score(a,hw_)>ModelCatalog::score(b,hw_);});
    for(int i=0;i<models_.size();++i){
        const auto&m=models_[i]; const int sc=ModelCatalog::score(m,hw_);
        table_->setItem(i,0,new QTableWidgetItem(m.name));
        table_->setItem(i,1,new QTableWidgetItem(m.task));
        QStringList formats; for(const auto&a:m.artifacts) formats<<a.format;
        table_->setItem(i,2,new QTableWidgetItem(formats.join(" / ")));
        table_->setItem(i,3,new QTableWidgetItem(ModelCatalog::status(m,hw_)));
        table_->setItem(i,4,new QTableWidgetItem(ModelCatalog::reason(m,hw_)));
        table_->setItem(i,5,new QTableWidgetItem(m.note));
    }
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true); table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->addWidget(table_,1);
    auto*h=new QHBoxLayout; status_=new QLabel("Select a model."); download_=new QPushButton("Download selected");
    h->addWidget(status_,1);h->addWidget(download_);v->addLayout(h);
    connect(download_,&QPushButton::clicked,this,&ModelManagerDialog::downloadSelected);
    connect(table_,&QTableWidget::itemSelectionChanged,this,&ModelManagerDialog::selectionChanged);
    if(!models_.isEmpty()){table_->selectRow(0);selectionChanged();}
}
void ModelManagerDialog::selectionChanged(){
    int r=table_->currentRow(); if(r<0)return; const auto&m=models_[r];
    QString extra;
#ifdef Q_OS_MACOS
    for(const auto&a:m.artifacts) if(a.format.compare("ONNX",Qt::CaseInsensitive)==0){
        extra=" — ONNX cannot be sent directly to Core ML; a model-specific Core ML artifact/recipe is required for ANE."; break;
    }
#endif
    status_->setText(QString("%1 — %2%3").arg(m.name,ModelCatalog::reason(m,hw_),extra));
    bool ready=false; for(const auto&a:ModelCatalog::compatibleArtifacts(m,hw_)) ready|=a.benchmarkReady;
    download_->setText(ready ? "Download & Test" : "Download selected");
}
void ModelManagerDialog::downloadSelected(){
    int r=table_->currentRow(); if(r<0)return; const auto m=models_[r];
    const auto compatible=ModelCatalog::compatibleArtifacts(m,hw_);
    if(compatible.isEmpty()){status_->setText("No artifact is compatible with a runtime in this build.");return;}
    const ModelArtifact* chosen=nullptr;
    for(const auto&a:compatible) if(a.benchmarkReady && (!a.url.isEmpty() || !a.files.isEmpty())){chosen=&a;break;}
    if(!chosen) for(const auto&a:compatible) if(!a.url.isEmpty() || !a.files.isEmpty()){chosen=&a;break;}
    if(!chosen){status_->setText("This model is catalog preview only: a complete downloadable artifact/adapter is not available yet.");return;}
    const auto a=*chosen;

    QList<ModelFile> files=a.files;
    if(files.isEmpty()) files << ModelFile{a.url,a.fileName,a.sha256,a.sizeBytes};

    const QString modelDir=ModelCatalog::cacheDir()+"/"+m.id;
    const QString artifactRoot=a.files.isEmpty()?modelDir:modelDir+"/"+a.fileName;
    QDir().mkpath(artifactRoot);
    download_->setEnabled(false);

    auto* nam=new QNetworkAccessManager(this);
    auto index=std::make_shared<int>(0);
    auto next=std::make_shared<std::function<void()>>();
    *next=[=]() {
        if(*index>=files.size()){
            download_->setEnabled(true);
            const QString readyPath=a.files.isEmpty()?modelDir+"/"+a.fileName:artifactRoot;
            if(!a.benchmarkReady){
                status_->setText("Downloaded, but this artifact still requires the "+m.adapter+" semantic adapter before benchmarking.");
            }else{
                status_->setText("Ready: "+readyPath);
                emit modelReady(readyPath,a.runtime);
            }
            nam->deleteLater(); return;
        }
        const int i=(*index)++; const auto mf=files[i];
        const QString path=(a.files.isEmpty()?modelDir:artifactRoot)+"/"+mf.relativePath;
        QDir().mkpath(QFileInfo(path).absolutePath());
        status_->setText(QString("Downloading %1… file %2/%3").arg(m.name).arg(i+1).arg(files.size()));
        auto* reply=nam->get(QNetworkRequest(QUrl(mf.url)));
        connect(reply,&QNetworkReply::downloadProgress,this,[=](qint64 got,qint64 total){
            if(total>0) status_->setText(QString("Downloading %1… file %2/%3 — %4%")
                .arg(m.name).arg(i+1).arg(files.size()).arg(got*100/total));
        });
        connect(reply,&QNetworkReply::finished,this,[=](){
            if(reply->error()!=QNetworkReply::NoError){
                download_->setEnabled(true); status_->setText("Download failed: "+reply->errorString());
                reply->deleteLater(); nam->deleteLater(); return;
            }
            const QByteArray data=reply->readAll();
            if(!mf.sha256.isEmpty() && QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex()!=mf.sha256.toLatin1()){
                download_->setEnabled(true); status_->setText("SHA-256 verification failed for "+mf.relativePath);
                reply->deleteLater(); nam->deleteLater(); return;
            }
            QSaveFile file(path);
            if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){
                download_->setEnabled(true); status_->setText("Could not save "+mf.relativePath);
                reply->deleteLater(); nam->deleteLater(); return;
            }
            reply->deleteLater(); (*next)();
        });
    };
    (*next)();
}
