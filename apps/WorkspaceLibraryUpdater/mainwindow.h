#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <librepcb/common/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

namespace Ui {
class MainWindow;
}

namespace librepcb {

class DomElement;
class TransactionalFileSystem;

namespace library {
class Library;
}

}  // namespace librepcb

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

private slots:

  void on_addDirectoryBtn_clicked();
  void on_removeDirectoryBtn_clicked();
  void on_clrLibraryBtn_clicked();
  void on_updateBtn_clicked();

private:
  template <typename ElementType>
  void updateElements(std::shared_ptr<librepcb::TransactionalFileSystem> fs,
                      const librepcb::library::Library& lib) noexcept;

  // Attributes
  Ui::MainWindow* ui;
  QString         lastDir;
  int             elementCount;
  int             ignoreCount;
  int             errorCount;
};

#endif  // MAINWINDOW_H
