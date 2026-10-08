#pragma once
#include <QDialog>
#include "ModelCatalog.h"
class QTableWidget; class QLabel; class QPushButton; class QNetworkReply;
class ModelManagerDialog: public QDialog {
    Q_OBJECT
    HardwareProfile hw_;
    QList<CatalogModel> models_;
    QTableWidget* table_;
    QLabel* status_;
    QPushButton* download_;
public:
    explicit ModelManagerDialog(QWidget* parent=nullptr);
signals:
    void modelReady(QString path, QString preferredBackend);
private slots:
    void downloadSelected();
    void selectionChanged();
};
