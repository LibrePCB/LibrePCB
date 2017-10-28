#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/elements.h>

using namespace librepcb;
using namespace librepcb::library;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    ui->libDirs->addItems(s.value("mainwindow/lib_dirs").toStringList());

    ui->cbx_lplib->setChecked(s.value("mainwindow/cbx_lplib", true).toBool());
    ui->cbx_cmpcat->setChecked(s.value("mainwindow/cbx_cmpcat", true).toBool());
    ui->cbx_pkgcat->setChecked(s.value("mainwindow/cbx_pkgcat", true).toBool());
    ui->cbx_sym->setChecked(s.value("mainwindow/cbx_sym", true).toBool());
    ui->cbx_pkg->setChecked(s.value("mainwindow/cbx_pkg", true).toBool());
    ui->cbx_cmp->setChecked(s.value("mainwindow/cbx_cmp", true).toBool());
    ui->cbx_dev->setChecked(s.value("mainwindow/cbx_dev", true).toBool());

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

    s.setValue("mainwindow/cbx_lplib", ui->cbx_lplib->isChecked());
    s.setValue("mainwindow/cbx_cmpcat", ui->cbx_cmpcat->isChecked());
    s.setValue("mainwindow/cbx_pkgcat", ui->cbx_pkgcat->isChecked());
    s.setValue("mainwindow/cbx_sym", ui->cbx_sym->isChecked());
    s.setValue("mainwindow/cbx_pkg", ui->cbx_pkg->isChecked());
    s.setValue("mainwindow/cbx_cmp", ui->cbx_cmp->isChecked());
    s.setValue("mainwindow/cbx_dev", ui->cbx_dev->isChecked());

    delete ui;
}

void MainWindow::on_addDirectoryBtn_clicked()
{
    QFileDialog dialog(this, "Select Directories", lastDir);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly |
                      QFileDialog::HideNameFilterDetails | QFileDialog::DontUseNativeDialog);
    QListView* l = dialog.findChild<QListView*>("listView");
    if (l) l->setSelectionMode(QAbstractItemView::MultiSelection);
    QTreeView* t = dialog.findChild<QTreeView*>();
    if (t) t->setSelectionMode(QAbstractItemView::MultiSelection);
    if (dialog.exec()) {
        ui->libDirs->addItems(dialog.selectedFiles());
    }
    lastDir = dialog.directory().absolutePath();
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

    elementCount = 0;
    ignoreCount = 0;
    errorCount = 0;
    for (int i = 0; i < ui->libDirs->count(); i++)
    {
        QString dirStr = ui->libDirs->item(i)->text();

        try
        {
            library::Library lib(FilePath(dirStr), false);

            if (ui->cbx_cmpcat->isChecked()) updateElements<ComponentCategory>(lib);
            if (ui->cbx_pkgcat->isChecked()) updateElements<PackageCategory>(lib);
            if (ui->cbx_sym->isChecked()) updateElements<Symbol>(lib);
            if (ui->cbx_pkg->isChecked()) updateElements<Package>(lib);
            if (ui->cbx_cmp->isChecked()) updateElements<Component>(lib);
            if (ui->cbx_dev->isChecked()) updateElements<Device>(lib);

            if (ui->cbx_lplib->isChecked()) {
                lib.save();
                elementCount++;
            }
        }
        catch (Exception& e)
        {
            ui->log->addItem("ERROR: " % e.getMsg());
            errorCount++;
        }
    }

    ui->log->addItem(QString("FINISHED: %1 updated, %2 ignored, %3 errors")
                     .arg(elementCount).arg(ignoreCount).arg(errorCount));
    ui->log->setCurrentRow(ui->log->count()-1);
}

template <typename ElementType>
void MainWindow::updateElements(const library::Library& lib) noexcept
{
    foreach (const FilePath& fp, lib.searchForElements<ElementType>()) {
        if (fp.getBasename() == "00000000-0000-4001-8000-000000000000") {
            // ignore demo files as they contain documentation which would be removed
            ignoreCount++;
            continue;
        }
        try {
            ElementType element(fp, false);
            element.save();
            ui->log->addItem(fp.toNative());
            elementCount++;
        } catch (const Exception& e) {
            ui->log->addItem("ERROR: " % e.getMsg());
            errorCount++;
        }
    }
}
