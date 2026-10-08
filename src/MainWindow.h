#pragma once
#include <QMainWindow>
class QLineEdit; class QSpinBox; class QPlainTextEdit; class QPushButton; class QComboBox;
class MainWindow: public QMainWindow {
 Q_OBJECT
 QLineEdit *url_; QSpinBox *warm_,*runs_; QPlainTextEdit *request_,*out_; QPushButton *run_;
public: MainWindow();
private slots: void runBench(); void probe();
};
