#ifndef UUIDGENERATOR_MAINWINDOW_H
#define UUIDGENERATOR_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

private slots:
  void on_pushButton_clicked();
  void on_timer_timeout();

  void on_checkBox_toggled(bool checked);

private:
  Ui::MainWindow* ui;
  QTimer* timer;
};

#endif
