#include "ModelManagerDialog.h"
#include <QtWidgets>
#include <QtNetwork>
#include <QCryptographicHash>
#include <QSaveFile>
#include <algorithm>

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
}
void ModelManagerDialog::downloadSelected(){
    int r=table_->currentRow(); if(r<0)return; const auto m=models_[r];
    const auto compatible=ModelCatalog::compatibleArtifacts(m,hw_);
    if(compatible.isEmpty()){status_->setText("No artifact is compatible with a runtime in this build.");return;}
    const ModelArtifact* chosen=nullptr;
    for(const auto&a:compatible) if(a.benchmarkReady && !a.url.isEmpty()){chosen=&a;break;}
    if(!chosen) for(const auto&a:compatible) if(!a.url.isEmpty()){chosen=&a;break;}
    if(!chosen){status_->setText("This model is catalog preview only: a complete downloadable artifact/adapter is not available yet.");return;}
    const auto a=*chosen;
    QDir dir(ModelCatalog::cacheDir()); dir.mkpath(m.id);
    const QString path=dir.filePath(m.id+"/"+a.fileName);
    QDir().mkpath(QFileInfo(path).absolutePath());
    download_->setEnabled(false); status_->setText("Downloading "+m.name+" ("+a.runtime+"/"+a.format+")…");
    auto*nam=new QNetworkAccessManager(this); auto*reply=nam->get(QNetworkRequest(QUrl(a.url)));
    connect(reply,&QNetworkReply::downloadProgress,this,[this,m](qint64 got,qint64 total){
      status_->setText(total>0?QString("Downloading %1… %2%").arg(m.name).arg(got*100/total):"Downloading "+m.name+"…");
    });
    connect(reply,&QNetworkReply::finished,this,[=]{
      download_->setEnabled(true);
      if(reply->error()!=QNetworkReply::NoError){status_->setText("Download failed: "+reply->errorString());reply->deleteLater();return;}
      const QByteArray data=reply->readAll();
      if(!a.sha256.isEmpty() && QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex()!=a.sha256.toLatin1()){
        status_->setText("SHA-256 verification failed; file was not saved.");reply->deleteLater();return;
      }
      QSaveFile file(path); if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){
        status_->setText("Could not save model.");reply->deleteLater();return;
      }
      if(!a.benchmarkReady){
        status_->setText("Downloaded, but this artifact still requires the "+m.adapter+" semantic adapter before benchmarking.");
        reply->deleteLater(); return;
      }
      status_->setText("Ready: "+path);
      emit modelReady(path,a.runtime); reply->deleteLater();
    });
}
