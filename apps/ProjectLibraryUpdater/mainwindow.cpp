#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>

using namespace librepcb;
using namespace librepcb::library;
using namespace librepcb::workspace;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    ui->workspacepath->setText(s.value("mainwindow/workspace_directory").toString());
    ui->projectfiles->addItems(s.value("mainwindow/projects").toStringList());
}

MainWindow::~MainWindow()
{
    QStringList projectList;
    for (int i = 0; i < ui->projectfiles->count(); i++)
        projectList.append(ui->projectfiles->item(i)->text());

    QSettings s;
    s.setValue("mainwindow/geometry", saveGeometry());
    s.setValue("mainwindow/state", saveState());
    s.setValue("mainwindow/workspace_directory", ui->workspacepath->text());
    s.setValue("mainwindow/projects", QVariant::fromValue(projectList));

    delete ui;
}

void MainWindow::on_libBtn_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Directory",
                                                    ui->workspacepath->text());
    if (dir.isEmpty()) return;
    ui->workspacepath->setText(dir);
}

void MainWindow::on_addProjectBtn_clicked()
{
    ui->projectfiles->addItems(QFileDialog::getOpenFileNames(this, "Select Project File",
                                                             QString(), "*.lpp"));
}

void MainWindow::on_removeProjectBtn_clicked()
{
    qDeleteAll(ui->projectfiles->selectedItems());
}

void MainWindow::on_clrProjectBtn_clicked()
{
    ui->projectfiles->clear();
}

void MainWindow::on_pushButton_2_clicked()
{
    if (ui->workspacepath->text().isEmpty()) return;
    if (ui->projectfiles->count() == 0) return;
    ui->log->clear();

    try
    {
        FilePath workspacePath(ui->workspacepath->text());
        Workspace workspace(workspacePath);

        for (int i = 0; i < ui->projectfiles->count(); i++)
        {
            // open the project file
            FilePath projectFilepath(ui->projectfiles->item(i)->text());
            FilePath coreDirPath = projectFilepath.getParentDir().getPathTo("core");
            SmartSExprFile boardsFile(coreDirPath.getPathTo("boards.lp"), false, true);
            SExpression boardsRoot = boardsFile.parseFileAndBuildDomTree();

            // remove the whole library directory
            FilePath libDir = projectFilepath.getParentDir().getPathTo("library");
            FileUtils::removeDirRecursively(libDir); // can throw

            // components & symbols
            SmartSExprFile circuitFile(projectFilepath.getParentDir().getPathTo("core/circuit.lp"), false, true);
            SExpression circuitRoot = circuitFile.parseFileAndBuildDomTree();
            foreach (const SExpression& node, circuitRoot.getChildren("component")) {
                Uuid compUuid = node.getValueByPath<Uuid>("lib_component", true);
                FilePath filepath = workspace.getLibraryDb().getLatestComponent(compUuid);
                if (!filepath.isExistingDir()) {
                    qDebug() << filepath.toStr();
                    throw RuntimeError(__FILE__, __LINE__,
                        QString("Missing component: %1").arg(compUuid.toStr()));
                }
                // open & copy component
                Component latestComp(filepath, true);
                FilePath dest = libDir.getPathTo("cmp").getPathTo(filepath.getFilename());
                if (!dest.isExistingDir()) FileUtils::copyDirRecursively(filepath, dest);
                ui->log->addItem(latestComp.getFilePath().toNative());

                // search all required symbols
                for (const ComponentSymbolVariant& symbvar : latestComp.getSymbolVariants()) {
                    foreach (const Uuid& symbolUuid, symbvar.getAllSymbolUuids()) {
                        FilePath filepath = workspace.getLibraryDb().getLatestSymbol(symbolUuid);
                        if (!filepath.isExistingDir()) {
                            qDebug() << filepath.toStr();
                            throw RuntimeError(__FILE__, __LINE__,
                                QString("Missing symbol: %1").arg(symbolUuid.toStr()));
                        }
                        // open & copy symbol
                        Symbol latestSymbol(filepath, true);
                        FilePath dest = libDir.getPathTo("sym").getPathTo(filepath.getFilename());
                        if (!dest.isExistingDir()) FileUtils::copyDirRecursively(filepath, dest);
                        ui->log->addItem(latestSymbol.getFilePath().toNative());
                    }
                }
            }

            // devices & packages
            foreach (const SExpression& node, boardsRoot.getChildren("board")) {
                FilePath boardFilePath = projectFilepath.getParentDir().getPathTo(node.getValueOfFirstChild<QString>(true));
                SmartSExprFile boardFile(boardFilePath, false, true);
                SExpression boardRoot = boardFile.parseFileAndBuildDomTree();
                foreach (const SExpression& node, boardRoot.getChildren("device")) {
                    Uuid deviceUuid = node.getValueByPath<Uuid>("lib_device", true);
                    FilePath filepath = workspace.getLibraryDb().getLatestDevice(deviceUuid);
                    if (!filepath.isExistingDir()) {
                        qDebug() << filepath.toStr();
                        throw RuntimeError(__FILE__, __LINE__,
                            QString("Missing device: %1").arg(deviceUuid.toStr()));
                    }
                    // open & copy device
                    Device latestDevice(filepath, true);
                    FilePath dest = libDir.getPathTo("dev").getPathTo(filepath.getFilename());
                    if (!dest.isExistingDir()) FileUtils::copyDirRecursively(filepath, dest);
                    ui->log->addItem(latestDevice.getFilePath().toNative());

                    // get package
                    Uuid packUuid = latestDevice.getPackageUuid();
                    filepath = workspace.getLibraryDb().getLatestPackage(packUuid);
                    if (!filepath.isExistingDir()) {
                        qDebug() << filepath.toStr();
                        throw RuntimeError(__FILE__, __LINE__,
                            QString("Missing package: %1").arg(packUuid.toStr()));
                    }
                    // open & copy package
                    Package latestPackage(filepath, true);
                    dest = libDir.getPathTo("pkg").getPathTo(filepath.getFilename());
                    if (!dest.isExistingDir()) FileUtils::copyDirRecursively(filepath, dest);
                    ui->log->addItem(latestPackage.getFilePath().toNative());
                }
            }
        }
    }
    catch (Exception& e)
    {
        ui->log->addItem("ERROR: " % e.getMsg());
    }

    ui->log->addItem("FINISHED");
    ui->log->setCurrentRow(ui->log->count()-1);
}

void MainWindow::on_rescanlib_clicked()
{
    if (ui->workspacepath->text().isEmpty()) return;

    try
    {
        FilePath workspacePath(ui->workspacepath->text());
        Workspace workspace(workspacePath);
        workspace.getLibraryDb().startLibraryRescan();
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
}
