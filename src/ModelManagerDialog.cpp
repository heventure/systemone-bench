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
        table_->setItem(i,2,new QTableWidgetItem(m.format));
        table_->setItem(i,3,new QTableWidgetItem(sc>=70?"Recommended":sc>=30?"Compatible":"Not recommended"));
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
    status_->setText(QString("%1 — %2").arg(m.name,ModelCatalog::reason(m,hw_)));
}
void ModelManagerDialog::downloadSelected(){
    int r=table_->currentRow(); if(r<0)return; const auto m=models_[r];
    if(m.url.isEmpty()){ status_->setText("This model needs a multi-file semantic adapter; direct download is not enabled yet."); return; }
    QDir().mkpath(ModelCatalog::cacheDir()); const QString path=ModelCatalog::cacheDir()+"/"+m.fileName;
    download_->setEnabled(false); status_->setText("Downloading "+m.name+"…");
    auto*nam=new QNetworkAccessManager(this); auto*reply=nam->get(QNetworkRequest(QUrl(m.url)));
    connect(reply,&QNetworkReply::downloadProgress,this,[this,m](qint64 a,qint64 n){
      status_->setText(n>0?QString("Downloading %1… %2%").arg(m.name).arg(a*100/n):"Downloading "+m.name+"…");
    });
    connect(reply,&QNetworkReply::finished,this,[=]{
      download_->setEnabled(true);
      if(reply->error()!=QNetworkReply::NoError){status_->setText("Download failed: "+reply->errorString());reply->deleteLater();return;}
      const QByteArray data=reply->readAll();
      if(!m.sha256.isEmpty() && QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex()!=m.sha256.toLatin1()){
        status_->setText("SHA-256 verification failed; file was not saved.");reply->deleteLater();return;
      }
      QSaveFile f(path); if(!f.open(QIODevice::WriteOnly)||f.write(data)!=data.size()||!f.commit()){
        status_->setText("Could not save model.");reply->deleteLater();return;
      }
      status_->setText("Ready: "+path);
      QString backend=m.runtimes.isEmpty()?QString():m.runtimes.first();
      emit modelReady(path,backend); reply->deleteLater();
    });
}
