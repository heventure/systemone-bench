#pragma once
#include <QObject>
#include <QJsonObject>
#include <QList>
struct BenchSample { double ms=0; int status=0; bool ok=false; QByteArray body; };
struct BenchSummary { int total=0, ok=0; double mean=0,p50=0,p95=0,p99=0,min=0,max=0; };
class Benchmark : public QObject {
 Q_OBJECT
public:
 using QObject::QObject;
 void run(const QUrl&, const QJsonObject&, int warmup, int runs);
 static BenchSummary summarize(const QList<BenchSample>&);
signals:
 void finished(QList<BenchSample>, BenchSummary);
 void progress(int,int);
};
Q_DECLARE_METATYPE(BenchSummary)
