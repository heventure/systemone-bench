#include "MainWindow.h"
#include "Benchmark.h"
#include "RuntimeProbe.h"
#include <QtWidgets>
#include <QJsonDocument>
MainWindow::MainWindow(){
 setWindowTitle("SystemOne Bench"); resize(980,720);
 auto*c=new QWidget;auto*v=new QVBoxLayout(c);auto*top=new QHBoxLayout;
 url_=new QLineEdit("http://127.0.0.1:8080/v1/systemone");warm_=new QSpinBox;runs_=new QSpinBox;
 warm_->setRange(0,10000);warm_->setValue(10);runs_->setRange(1,100000);runs_->setValue(100);
 run_=new QPushButton("Run benchmark");auto*probeBtn=new QPushButton("Probe hardware");
 top->addWidget(new QLabel("Endpoint"));top->addWidget(url_,1);top->addWidget(new QLabel("Warmup"));top->addWidget(warm_);
 top->addWidget(new QLabel("Runs"));top->addWidget(runs_);top->addWidget(run_);top->addWidget(probeBtn);v->addLayout(top);
 request_=new QPlainTextEdit(R"({"state":"A dialog is open and the task is not complete.","questions":{"next_action":{"type":"choice","options":["click","type","scroll","wait","escalate_system2"]},"done":{"type":"noul","statement":"The task is complete."}}})");
 out_=new QPlainTextEdit;out_->setReadOnly(true);v->addWidget(new QLabel("Decision request JSON"));v->addWidget(request_,1);
 v->addWidget(new QLabel("Results"));v->addWidget(out_,1);setCentralWidget(c);
 connect(run_,&QPushButton::clicked,this,&MainWindow::runBench);connect(probeBtn,&QPushButton::clicked,this,&MainWindow::probe);
}
void MainWindow::probe(){out_->appendPlainText("\n=== Hardware / runtime ===\n"+RuntimeProbe::report());}
void MainWindow::runBench(){
 QJsonParseError e;auto d=QJsonDocument::fromJson(request_->toPlainText().toUtf8(),&e);
 if(e.error!=QJsonParseError::NoError||!d.isObject()){QMessageBox::warning(this,"Invalid JSON",e.errorString());return;}
 run_->setEnabled(false);auto*b=new Benchmark(this);
 connect(b,&Benchmark::progress,this,[this](int a,int n){statusBar()->showMessage(QString("%1 / %2").arg(a).arg(n));});
 connect(b,&Benchmark::finished,this,[this,b](auto xs,BenchSummary s){
  out_->appendPlainText(QString("\nOK %1/%2\nMean %3 ms\nP50 %4 ms\nP95 %5 ms\nP99 %6 ms\nMin %7 ms\nMax %8 ms")
   .arg(s.ok).arg(s.total).arg(s.mean,0,'f',3).arg(s.p50,0,'f',3).arg(s.p95,0,'f',3).arg(s.p99,0,'f',3).arg(s.min,0,'f',3).arg(s.max,0,'f',3));
  run_->setEnabled(true);b->deleteLater();});
 b->run(QUrl(url_->text()),d.object(),warm_->value(),runs_->value());
}
