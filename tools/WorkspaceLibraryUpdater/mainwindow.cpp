#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/elements.h>

using namespace library;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    ui->libDirs->addItems(s.value("mainwindow/lib_dirs").toStringList());

    ui->cbx_cmpcat->setChecked(s.value("mainwindow/cbx_cmpcat", true).toBool());
    ui->cbx_pkgcat->setChecked(s.value("mainwindow/cbx_pkgcat", true).toBool());
    ui->cbx_sym->setChecked(s.value("mainwindow/cbx_sym", true).toBool());
    ui->cbx_fpt->setChecked(s.value("mainwindow/cbx_fpt", true).toBool());
    ui->cbx_3dmdl->setChecked(s.value("mainwindow/cbx_3dmdl", true).toBool());
    ui->cbx_pkg->setChecked(s.value("mainwindow/cbx_pkg", true).toBool());
    ui->cbx_gencmp->setChecked(s.value("mainwindow/cbx_gencmp", true).toBool());
    ui->cbx_cmp->setChecked(s.value("mainwindow/cbx_cmp", true).toBool());

    if (ui->libDirs->count() > 0) lastDir = ui->libDirs->item(ui->libDirs->count()-1)->text();
}

MainWindow::~MainWindow()
{
    QStringList libDirList;
    for (int i = 0; i < ui->libDirs->count(); i++)
        libDirList.append(ui->libDirs->item(i)->text());

    QSettings s;
    s.setValue("mainwindow/geometry", saveGeometry());
    s.setValue("mainwindow/state", saveState());
    s.setValue("mainwindow/lib_dirs", QVariant::fromValue(libDirList));

    s.setValue("mainwindow/cbx_cmpcat", ui->cbx_cmpcat->isChecked());
    s.setValue("mainwindow/cbx_pkgcat", ui->cbx_pkgcat->isChecked());
    s.setValue("mainwindow/cbx_sym", ui->cbx_sym->isChecked());
    s.setValue("mainwindow/cbx_fpt", ui->cbx_fpt->isChecked());
    s.setValue("mainwindow/cbx_3dmdl", ui->cbx_3dmdl->isChecked());
    s.setValue("mainwindow/cbx_pkg", ui->cbx_pkg->isChecked());
    s.setValue("mainwindow/cbx_gencmp", ui->cbx_gencmp->isChecked());
    s.setValue("mainwindow/cbx_cmp", ui->cbx_cmp->isChecked());

    delete ui;
}

void MainWindow::on_addDirectoryBtn_clicked()
{
    lastDir = QFileDialog::getExistingDirectory(this, "Select Directory", lastDir);
    if (lastDir.isEmpty()) return;
    ui->libDirs->addItem(lastDir);
}

void MainWindow::on_removeDirectoryBtn_clicked()
{
    qDeleteAll(ui->libDirs->selectedItems());
}

void MainWindow::on_clrLibraryBtn_clicked()
{
    ui->libDirs->clear();
}

void MainWindow::on_updateBtn_clicked()
{
    if (ui->libDirs->count() == 0) return;
    ui->log->clear();

    int elementCount = 0;
    int ignoreCount = 0;
    int errorCount = 0;
    for (int i = 0; i < ui->libDirs->count(); i++)
    {
        QString dirStr = ui->libDirs->item(i)->text();
        QStringList filter;
        if (ui->cbx_cmpcat->isChecked())    filter.append("*.cmpcat");
        if (ui->cbx_pkgcat->isChecked())    filter.append("*.pkgcat");
        if (ui->cbx_sym->isChecked())       filter.append("*.sym");
        if (ui->cbx_fpt->isChecked())       filter.append("*.fpt");
        if (ui->cbx_3dmdl->isChecked())     filter.append("*.3dmdl");
        if (ui->cbx_pkg->isChecked())       filter.append("*.pkg");
        if (ui->cbx_gencmp->isChecked())    filter.append("*.gencmp");
        if (ui->cbx_cmp->isChecked())       filter.append("*.cmp");

        // search library elements
        QDirIterator it(dirStr, filter, QDir::Dirs, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            FilePath dirFilePath(it.next());
            if (dirFilePath.getBasename() == "{00000000-0000-4001-8000-000000000000}")
            {
                // ignore demo files as they contain documentation which would be removed
                ignoreCount++;
                continue;
            }
            try
            {
                if (dirFilePath.getSuffix() == "cmpcat")
                {
                    library::ComponentCategory elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "pkgcat")
                {
                    library::PackageCategory elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "sym")
                {
                    library::Symbol elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "fpt")
                {
                    library::Footprint elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "3dmdl")
                {
                    library::Model3D elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "pkg")
                {
                    library::Package elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "gencmp")
                {
                    library::GenericComponent elem(dirFilePath);
                    elem.save();
                }
                else if (dirFilePath.getSuffix() == "cmp")
                {
                    library::Component elem(dirFilePath);
                    elem.save();
                }
                else
                {
                    Q_ASSERT(false);
                }
                ui->log->addItem(dirFilePath.toNative());
                elementCount++;
            }
            catch (Exception& e)
            {
                ui->log->addItem("ERROR: " % e.getUserMsg());
                errorCount++;
            }
        }
    }

    ui->log->addItem(QString("FINISHED: %1 updated, %2 ignored, %3 errors")
                     .arg(elementCount).arg(ignoreCount).arg(errorCount));
    ui->log->setCurrentRow(ui->log->count()-1);
}
