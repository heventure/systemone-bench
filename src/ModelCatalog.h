#pragma once
#include <QString>
#include <QStringList>
#include <QList>

struct HardwareProfile {
    QString os;
    QString arch;
    QStringList runtimes;
    QStringList devices;
    quint64 memoryBytes = 0;
};

struct CatalogModel {
    QString id;
    QString name;
    QString family;
    QString task;
    QString format;
    QString url;
    QString fileName;
    QString sha256;
    qint64 sizeBytes = 0;
    int minMemoryGB = 0;
    QStringList runtimes;
    QStringList devices;
    QString adapter;
    QString note;
    bool smokeTest = false;
};

class ModelCatalog {
public:
    static HardwareProfile probe();
    static QList<CatalogModel> models();
    static int score(const CatalogModel&, const HardwareProfile&);
    static QString reason(const CatalogModel&, const HardwareProfile&);
    static QString cacheDir();
};
