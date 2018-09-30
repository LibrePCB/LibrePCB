#include "mainwindow.h"

#include "polygonsimplifier.h"
#include "ui_mainwindow.h"

#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/eagleimport/converterdb.h>
#include <librepcb/eagleimport/deviceconverter.h>
#include <librepcb/eagleimport/devicesetconverter.h>
#include <librepcb/eagleimport/packageconverter.h>
#include <librepcb/eagleimport/symbolconverter.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/sym/symbol.h>
#include <parseagle/library.h>

#include <QtCore>
#include <QtWidgets>

namespace librepcb {
using namespace library;

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  QSettings s;
  restoreGeometry(s.value("mainwindow/geometry").toByteArray());
  restoreState(s.value("mainwindow/state").toByteArray());
  mlastInputDirectory = s.value("mainwindow/last_input_directory").toString();
  ui->edtDuplicateFolders->setText(
      s.value("mainwindow/last_library_directory").toString());
  ui->input->addItems(s.value("mainwindow/input").toStringList());
  ui->output->setText(s.value("mainwindow/output").toString());
  ui->uuidList->setText(s.value("mainwindow/uuid_list").toString());

  reset();
}

MainWindow::~MainWindow() {
  QStringList inputList;
  for (int i = 0; i < ui->input->count(); i++)
    inputList.append(ui->input->item(i)->text());

  QSettings s;
  s.setValue("mainwindow/geometry", saveGeometry());
  s.setValue("mainwindow/state", saveState());
  s.setValue("mainwindow/last_input_directory", mlastInputDirectory);
  s.setValue("mainwindow/last_library_directory",
             ui->edtDuplicateFolders->text());
  s.setValue("mainwindow/input", QVariant::fromValue(inputList));
  s.setValue("mainwindow/output", ui->output->text());
  s.setValue("mainwindow/uuid_list", ui->uuidList->text());

  delete ui;
}

void MainWindow::reset() {
  mAbortConversion        = false;
  mReadedElementsCount    = 0;
  mConvertedElementsCount = 0;

  ui->errors->clear();
  ui->pbarElements->setValue(0);
  ui->pbarElements->setMaximum(0);
  ui->pbarFiles->setValue(0);
  ui->pbarFiles->setMaximum(ui->input->count());
  ui->lblConvertedElements->setText("0 of 0");
}

void MainWindow::addError(const QString& msg, const FilePath& inputFile,
                          int inputLine) {
  ui->errors->addItem(
      QString("%1 (%2:%3)").arg(msg).arg(inputFile.toNative()).arg(inputLine));
}

void MainWindow::convertAllFiles(ConvertFileType_t type) {
  reset();

  // create output directory
  FilePath outputDir(ui->output->text());
  try {
    FileUtils::makePath(outputDir);  // can throw
  } catch (const Exception& e) {
    addError("Fatal Error: " % e.getMsg());
  }

  eagleimport::ConverterDb db(FilePath(ui->uuidList->text()));

  for (int i = 0; i < ui->input->count(); i++) {
    FilePath filepath(ui->input->item(i)->text());
    if (!filepath.isExistingFile()) {
      addError("File not found: " % filepath.toNative());
      continue;
    }

    convertFile(type, db, filepath);
    ui->pbarFiles->setValue(i + 1);

    if (mAbortConversion) break;
  }
}

void MainWindow::convertFile(ConvertFileType_t         type,
                             eagleimport::ConverterDb& db,
                             const FilePath&           filepath) {
  try {
    parseagle::Library library(filepath.toStr());
    db.setCurrentLibraryFilePath(filepath);

    switch (type) {
      case ConvertFileType_t::Symbols_to_Symbols:
        ui->pbarElements->setValue(0);
        ui->pbarElements->setMaximum(library.getSymbols().count());
        foreach (const parseagle::Symbol& symbol, library.getSymbols()) {
          bool success = convertSymbol(db, symbol);
          mReadedElementsCount++;
          if (success) mConvertedElementsCount++;
          ui->pbarElements->setValue(ui->pbarElements->value() + 1);
          ui->lblConvertedElements->setText(QString("%1 of %2")
                                                .arg(mConvertedElementsCount)
                                                .arg(mReadedElementsCount));
        }
        break;
      case ConvertFileType_t::Packages_to_PackagesAndDevices:
        ui->pbarElements->setValue(0);
        ui->pbarElements->setMaximum(library.getPackages().count());
        foreach (const parseagle::Package& package, library.getPackages()) {
          bool success = convertPackage(db, package);
          mReadedElementsCount++;
          if (success) mConvertedElementsCount++;
          ui->pbarElements->setValue(ui->pbarElements->value() + 1);
          ui->lblConvertedElements->setText(QString("%1 of %2")
                                                .arg(mConvertedElementsCount)
                                                .arg(mReadedElementsCount));
        }
        break;
      case ConvertFileType_t::Devices_to_Components:
        ui->pbarElements->setValue(0);
        ui->pbarElements->setMaximum(library.getDeviceSets().count());
        foreach (const parseagle::DeviceSet& deviceSet,
                 library.getDeviceSets()) {
          bool success = convertDevice(db, deviceSet);
          mReadedElementsCount++;
          if (success) mConvertedElementsCount++;
          ui->pbarElements->setValue(ui->pbarElements->value() + 1);
          ui->lblConvertedElements->setText(QString("%1 of %2")
                                                .arg(mConvertedElementsCount)
                                                .arg(mReadedElementsCount));
        }
        break;
      default:
        throw Exception(__FILE__, __LINE__);
    }
  } catch (const std::exception& e) {
    addError(e.what());
    return;
  }
}

bool MainWindow::convertSymbol(eagleimport::ConverterDb& db,
                               const parseagle::Symbol&  symbol) {
  try {
    // create symbol
    eagleimport::SymbolConverter converter(symbol, db);
    std::unique_ptr<Symbol>      newSymbol = converter.generate();

    // convert line rects to polygon rects
    PolygonSimplifier<Symbol> polygonSimplifier(*newSymbol);
    polygonSimplifier.convertLineRectsToPolygonRects(false, true);

    // save symbol to file
    newSymbol->saveIntoParentDirectory(
        FilePath(QString("%1/sym").arg(ui->output->text())));
  } catch (const std::exception& e) {
    addError(e.what());
    return false;
  }

  return true;
}

bool MainWindow::convertPackage(eagleimport::ConverterDb& db,
                                const parseagle::Package& package) {
  try {
    // create package
    eagleimport::PackageConverter converter(package, db);
    std::unique_ptr<Package>      newPackage = converter.generate();

    // convert line rects to polygon rects
    Q_ASSERT(newPackage->getFootprints().count() == 1);
    PolygonSimplifier<Footprint> polygonSimplifier(
        *newPackage->getFootprints().first());
    polygonSimplifier.convertLineRectsToPolygonRects(false, true);

    // save package to file
    newPackage->saveIntoParentDirectory(
        FilePath(QString("%1/pkg").arg(ui->output->text())));
  } catch (const std::exception& e) {
    addError(e.what());
    return false;
  }

  return true;
}

bool MainWindow::convertDevice(eagleimport::ConverterDb&   db,
                               const parseagle::DeviceSet& deviceSet) {
  try {
    // abort if device name ends with "-US" or "-US_"
    if (deviceSet.getName().endsWith("-US")) return false;
    if (deviceSet.getName().endsWith("-US_")) return false;

    // create component
    eagleimport::DeviceSetConverter converter(deviceSet, db);
    std::unique_ptr<Component>      newComponent = converter.generate();

    // create devices
    foreach (const parseagle::Device& device, deviceSet.getDevices()) {
      if (device.getPackage().isNull()) continue;

      eagleimport::DeviceConverter devConverter(deviceSet, device, db);
      std::unique_ptr<Device>      newDevice = devConverter.generate();

      // save device
      newDevice->saveIntoParentDirectory(
          FilePath(QString("%1/dev").arg(ui->output->text())));
    }

    // save component to file
    newComponent->saveIntoParentDirectory(
        FilePath(QString("%1/cmp").arg(ui->output->text())));
  } catch (const std::exception& e) {
    addError(e.what());
    return false;
  }

  return true;
}

void MainWindow::on_inputBtn_clicked() {
  ui->input->addItems(QFileDialog::getOpenFileNames(
      this, "Select Eagle Library Files", mlastInputDirectory, "*.lbr"));
  ui->pbarFiles->setMaximum(ui->input->count());

  if (ui->input->count() > 0)
    mlastInputDirectory = QFileInfo(ui->input->item(0)->text()).absolutePath();
}

void MainWindow::on_outputBtn_clicked() {
  ui->output->setText(QFileDialog::getExistingDirectory(
      this, "Select Output Directory", ui->output->text()));
}

void MainWindow::on_btnAbort_clicked() {
  mAbortConversion = true;
}

void MainWindow::on_btnConvertSymbols_clicked() {
  convertAllFiles(ConvertFileType_t::Symbols_to_Symbols);
}

void MainWindow::on_btnConvertDevices_clicked() {
  convertAllFiles(ConvertFileType_t::Devices_to_Components);
}

void MainWindow::on_pushButton_2_clicked() {
  convertAllFiles(ConvertFileType_t::Packages_to_PackagesAndDevices);
}

void MainWindow::on_btnPathsFromIni_clicked() {
  FilePath inputDir(QFileDialog::getExistingDirectory(
      this, "Select Input Folder", mlastInputDirectory));
  if (!inputDir.isExistingDir()) return;

  QSettings outputSettings(ui->uuidList->text(), QSettings::IniFormat);

  foreach (QString key, outputSettings.allKeys()) {
    key.remove(0, key.indexOf("/") + 1);
    key.remove(key.indexOf(".lbr") + 4, key.length() - key.indexOf(".lbr") - 4);
    QString filepath = inputDir.getPathTo(key).toNative();

    bool exists = false;
    for (int i = 0; i < ui->input->count(); i++) {
      FilePath fp(ui->input->item(i)->text());
      if (fp.toNative() == filepath) exists = true;
    }
    if (!exists) ui->input->addItem(filepath);
  }
}

void MainWindow::on_toolButton_clicked() {
  ui->input->clear();
}

void MainWindow::on_toolButton_2_clicked() {
  qDeleteAll(ui->input->selectedItems());
}

void MainWindow::on_toolButton_3_clicked() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Library Folder", ui->edtDuplicateFolders->text());
  if (dir.isEmpty()) return;
  ui->edtDuplicateFolders->setText(dir);
}

void MainWindow::on_toolButton_4_clicked() {
  if (ui->edtDuplicateFolders->text().isEmpty()) return;
  if (ui->output->text().isEmpty()) return;

  QDir libDir(ui->edtDuplicateFolders->text());
  QDir outDir(ui->output->text());

  uint count = 0;

  foreach (const QString& subdir1,
           libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    if (subdir1 == "EagleImport") continue;
    if (subdir1 == "Staging_Area") continue;
    QDir repoDir = libDir.absoluteFilePath(subdir1);
    foreach (const QString& subdir2,
             repoDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
      QDir repoSubDir = repoDir.absoluteFilePath(subdir2);
      foreach (const QString& elementDir,
               repoSubDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        tl::optional<Uuid> elementUuid = Uuid::tryFromString(elementDir);
        if (!elementUuid) continue;

        QDir outDirElement =
            outDir.absoluteFilePath(subdir2 % "/" % elementDir);
        if (outDirElement.exists()) {
          qDebug() << outDirElement.absolutePath();
          if (outDirElement.removeRecursively())
            count++;
          else
            qDebug() << "Failed!";
        }
      }
    }
  }

  QMessageBox::information(this, "Duplicates Removed",
                           QString("%1 duplicates removed.").arg(count));
}

void MainWindow::on_uuidListBtn_clicked() {
  QString file = QFileDialog::getSaveFileName(this, "Select UUID List File",
                                              ui->uuidList->text(), "*.ini");
  if (file.isEmpty()) return;
  ui->uuidList->setText(file);
}

}  // namespace librepcb
