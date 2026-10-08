#include "Benchmark.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QElapsedTimer>
#include <algorithm>
static BenchSample once(QNetworkAccessManager& n,const QUrl& u,const QByteArray& body){
 QNetworkRequest r(u); r.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
 QElapsedTimer t; t.start(); auto *reply=n.post(r,body); QEventLoop loop;
 QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit); loop.exec();
 BenchSample s; s.ms=t.nsecsElapsed()/1e6; s.status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
 s.ok=reply->error()==QNetworkReply::NoError && s.status>=200 && s.status<300; s.body=reply->readAll(); reply->deleteLater(); return s;
}
static double pct(QList<double> v,double p){ if(v.isEmpty())return 0; std::sort(v.begin(),v.end()); return v[(v.size()-1)*p]; }
BenchSummary Benchmark::summarize(const QList<BenchSample>& xs){ BenchSummary s; s.total=xs.size(); QList<double> v;
 for(auto&x:xs)if(x.ok){s.ok++;v<<x.ms;} if(v.isEmpty())return s;
 s.min=*std::min_element(v.begin(),v.end()); s.max=*std::max_element(v.begin(),v.end());
 for(double x:v)s.mean+=x; s.mean/=v.size(); s.p50=pct(v,.5);s.p95=pct(v,.95);s.p99=pct(v,.99);return s; }
void Benchmark::run(const QUrl& u,const QJsonObject& obj,int warmup,int runs){
 QNetworkAccessManager n; auto b=QJsonDocument(obj).toJson(QJsonDocument::Compact);
 for(int i=0;i<warmup;i++)once(n,u,b); QList<BenchSample> xs;
 for(int i=0;i<runs;i++){xs<<once(n,u,b);emit progress(i+1,runs);} emit finished(xs,summarize(xs));
}
