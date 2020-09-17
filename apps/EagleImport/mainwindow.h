#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

namespace Ui {
class MainWindow;
}

namespace parseagle {
class Symbol;
class Package;
class DeviceSet;
}  // namespace parseagle

namespace librepcb {

namespace eagleimport {
class ConverterDb;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

private slots:
  void on_inputBtn_clicked();
  void on_outputBtn_clicked();
  void on_btnAbort_clicked();
  void on_btnConvertSymbols_clicked();
  void on_btnConvertDevices_clicked();
  void on_pushButton_2_clicked();
  void on_btnPathsFromIni_clicked();
  void on_toolButton_clicked();
  void on_toolButton_2_clicked();
  void on_toolButton_3_clicked();
  void on_toolButton_4_clicked();
  void on_uuidListBtn_clicked();

private:
  enum class ConvertFileType_t {
    Symbols_to_Symbols,
    Packages_to_PackagesAndDevices,
    Devices_to_Components
  };

  void reset();
  void addError(const QString& msg,
                const librepcb::FilePath& inputFile = librepcb::FilePath(),
                int inputLine = 0);
  void convertAllFiles(ConvertFileType_t type);
  void convertFile(ConvertFileType_t type, eagleimport::ConverterDb& db,
                   const librepcb::FilePath& filepath);
  bool convertSymbol(eagleimport::ConverterDb& db,
                     const parseagle::Symbol& symbol);
  bool convertPackage(eagleimport::ConverterDb& db,
                      const parseagle::Package& package);
  bool convertDevice(eagleimport::ConverterDb& db,
                     const parseagle::DeviceSet& deviceSet);

  // Attributes
  Ui::MainWindow* ui;
  bool mAbortConversion;
  QString mlastInputDirectory;
  int mReadedElementsCount;
  int mConvertedElementsCount;
};

}  // namespace librepcb

#endif  // MAINWINDOW_H
