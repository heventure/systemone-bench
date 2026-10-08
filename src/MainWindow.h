#pragma once
#include <QMainWindow>
#include <memory>

class QLineEdit;
class QSpinBox;
class QPlainTextEdit;
class QPushButton;
class QComboBox;
class QLabel;

class MainWindow: public QMainWindow {
    Q_OBJECT
    QComboBox *backend_, *device_;
    QLineEdit *model_, *url_;
    QSpinBox *warm_, *runs_;
    QPlainTextEdit *request_, *out_;
    QPushButton *run_, *browse_;
    QLabel *modelLabel_, *endpointLabel_, *requestLabel_;
public:
    MainWindow();
private slots:
    void runBench();
    void probe();
    void browseModel();
    void backendChanged();
    void openModelManager();
};
