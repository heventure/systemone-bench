#pragma once
#include <QString>
#include <QStringList>
#include <QList>
#include <QtGlobal>

struct HardwareProfile {
    QString os;
    QString arch;
    QStringList runtimes;
    QStringList devices;
    quint64 memoryBytes = 0;
};

struct ModelArtifact {
    QString id;
    QString runtime;
    QString format;
    QString url;
    QString fileName;
    QString sha256;
    qint64 sizeBytes = 0;
    bool benchmarkReady = false;
    QString note;
};

struct CatalogModel {
    QString id;
    QString name;
    QString family;
    QString task;
    int minMemoryGB = 0;
    QStringList devices;
    QString adapter;
    QString note;
    bool adapterAvailable = false;
    bool smokeTest = false;
    QList<ModelArtifact> artifacts;
};

class ModelCatalog {
public:
    static HardwareProfile probe();
    static QList<CatalogModel> models();
    static QList<ModelArtifact> compatibleArtifacts(const CatalogModel&, const HardwareProfile&);
    static int score(const CatalogModel&, const HardwareProfile&);
    static QString status(const CatalogModel&, const HardwareProfile&);
    static QString reason(const CatalogModel&, const HardwareProfile&);
    static QString cacheDir();
};
