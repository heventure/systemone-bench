#include "MainWindow.h"
#include "Benchmark.h"
#include "LocalBackend.h"
#include "RuntimeProbe.h"
#include <QtWidgets>
#include <QJsonDocument>

static QString summaryText(const BenchSummary& s) {
    return QString("OK %1/%2\nMean %3 ms\nP50 %4 ms\nP95 %5 ms\nP99 %6 ms\nMin %7 ms\nMax %8 ms")
        .arg(s.ok).arg(s.total)
        .arg(s.mean,0,'f',3).arg(s.p50,0,'f',3).arg(s.p95,0,'f',3)
        .arg(s.p99,0,'f',3).arg(s.min,0,'f',3).arg(s.max,0,'f',3);
}

MainWindow::MainWindow(){
    setWindowTitle("SystemOne Bench v0.2"); resize(1080,760);
    auto*c=new QWidget; auto*v=new QVBoxLayout(c);

    auto*mode=new QGridLayout;
    backend_=new QComboBox;
    backend_->addItem("HTTP API");
    backend_->addItems(availableLocalBackends());
    device_=new QComboBox;
    model_=new QLineEdit;
    browse_=new QPushButton("Browse…");
    url_=new QLineEdit("http://127.0.0.1:8080/v1/systemone");
    warm_=new QSpinBox; runs_=new QSpinBox;
    warm_->setRange(0,10000); warm_->setValue(10);
    runs_->setRange(1,100000); runs_->setValue(100);
    run_=new QPushButton("Run benchmark");
    auto*probeBtn=new QPushButton("Probe hardware");

    mode->addWidget(new QLabel("Backend"),0,0); mode->addWidget(backend_,0,1);
    mode->addWidget(new QLabel("Device"),0,2); mode->addWidget(device_,0,3);
    modelLabel_=new QLabel("Model"); mode->addWidget(modelLabel_,1,0);
    mode->addWidget(model_,1,1,1,3); mode->addWidget(browse_,1,4);
    endpointLabel_=new QLabel("Endpoint"); mode->addWidget(endpointLabel_,2,0);
    mode->addWidget(url_,2,1,1,3);
    mode->addWidget(new QLabel("Warmup"),3,0); mode->addWidget(warm_,3,1);
    mode->addWidget(new QLabel("Runs"),3,2); mode->addWidget(runs_,3,3);
    mode->addWidget(run_,3,4); mode->addWidget(probeBtn,3,5);
    v->addLayout(mode);

    requestLabel_=new QLabel("Decision request JSON");
    request_=new QPlainTextEdit(R"({"state":"A dialog is open and the task is not complete.","questions":{"next_action":{"type":"choice","options":["click","type","scroll","wait","escalate_system2"]},"done":{"type":"noul","statement":"The task is complete."}}})");
    out_=new QPlainTextEdit; out_->setReadOnly(true);
    v->addWidget(requestLabel_); v->addWidget(request_,1);
    v->addWidget(new QLabel("Results")); v->addWidget(out_,1);
    setCentralWidget(c);

    connect(run_,&QPushButton::clicked,this,&MainWindow::runBench);
    connect(probeBtn,&QPushButton::clicked,this,&MainWindow::probe);
    connect(browse_,&QPushButton::clicked,this,&MainWindow::browseModel);
    connect(backend_,&QComboBox::currentTextChanged,this,&MainWindow::backendChanged);
    backendChanged();
}

void MainWindow::backendChanged(){
    const bool http = backend_->currentText()=="HTTP API";
    modelLabel_->setEnabled(!http); model_->setEnabled(!http); browse_->setEnabled(!http);
    endpointLabel_->setEnabled(http); url_->setEnabled(http);
    requestLabel_->setEnabled(http); request_->setEnabled(http);
    device_->clear();
    if(http) device_->addItem("Remote server");
    else if(auto b=createLocalBackend(backend_->currentText())) device_->addItems(b->devices());
}

void MainWindow::browseModel(){
    auto b=createLocalBackend(backend_->currentText());
    if(!b) return;
    const QString filter=b->modelFilters().join(";;");
    const QString p=QFileDialog::getOpenFileName(this,"Choose local model",QString(),filter+";;All files (*)");
    if(!p.isEmpty()) model_->setText(p);
}

void MainWindow::probe(){
    out_->appendPlainText("\n=== Hardware / runtime ===\n"+RuntimeProbe::report());
    for(const auto& n:availableLocalBackends()){
        if(auto b=createLocalBackend(n))
            out_->appendPlainText(QString("\n%1 devices: %2").arg(n,b->devices().join(", ")));
    }
}

void MainWindow::runBench(){
    if(backend_->currentText()!="HTTP API"){
        if(model_->text().isEmpty()){ QMessageBox::warning(this,"Model required","Choose a local model first."); return; }
        auto b=createLocalBackend(backend_->currentText());
        if(!b){ QMessageBox::warning(this,"Backend unavailable","This backend is not available in this build."); return; }
        run_->setEnabled(false);
        statusBar()->showMessage("Running local inference…");
        auto r=b->run(model_->text(),device_->currentText(),warm_->value(),runs_->value());
        if(r.ok){
            out_->appendPlainText(QString("\n=== Local benchmark ===\n%1\nModel load/compile: %2 ms\n%3")
                .arg(r.details).arg(r.loadMs,0,'f',3).arg(summaryText(r.summary)));
        } else {
            out_->appendPlainText("\nERROR: "+r.error);
        }
        run_->setEnabled(true); statusBar()->clearMessage();
        return;
    }

    QJsonParseError e; auto d=QJsonDocument::fromJson(request_->toPlainText().toUtf8(),&e);
    if(e.error!=QJsonParseError::NoError||!d.isObject()){
        QMessageBox::warning(this,"Invalid JSON",e.errorString()); return;
    }
    run_->setEnabled(false); auto*b=new Benchmark(this);
    connect(b,&Benchmark::progress,this,[this](int a,int n){statusBar()->showMessage(QString("%1 / %2").arg(a).arg(n));});
    connect(b,&Benchmark::finished,this,[this,b](auto, BenchSummary s){
        out_->appendPlainText("\n=== HTTP benchmark ===\n"+summaryText(s));
        run_->setEnabled(true); statusBar()->clearMessage(); b->deleteLater();
    });
    b->run(QUrl(url_->text()),d.object(),warm_->value(),runs_->value());
}
