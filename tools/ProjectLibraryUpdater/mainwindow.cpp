#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/fpt/footprint.h>
#include <librepcblibrary/pkg/package.h>

using namespace library;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    ui->workspacelibrarypath->setText(s.value("mainwindow/library_directory").toString());
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
    s.setValue("mainwindow/library_directory", ui->workspacelibrarypath->text());
    s.setValue("mainwindow/projects", QVariant::fromValue(projectList));

    delete ui;
}

void MainWindow::on_libBtn_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Library Folder",
                                                    ui->workspacelibrarypath->text());
    if (dir.isEmpty()) return;
    ui->workspacelibrarypath->setText(dir);
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
    if (ui->workspacelibrarypath->text().isEmpty()) return;
    if (ui->projectfiles->count() == 0) return;
    ui->log->clear();

    try
    {
        FilePath libDir(ui->workspacelibrarypath->text());
        Library lib(libDir);

        for (int i = 0; i < ui->projectfiles->count(); i++)
        {
            FilePath projectFilepath(ui->projectfiles->item(i)->text());
            SmartXmlFile projectFile(projectFilepath, false, true);
            QSharedPointer<XmlDomDocument> projectDoc = projectFile.parseFileAndBuildDomTree(true);

            // generic components & symbols
            SmartXmlFile circuitFile(projectFilepath.getParentDir().getPathTo("core/circuit.xml"), false, true);
            QSharedPointer<XmlDomDocument> circuitDoc = circuitFile.parseFileAndBuildDomTree(true);
            for (XmlDomElement* node = circuitDoc->getRoot().getFirstChild("generic_component_instances/*", true, false);
                 node; node = node->getNextSibling())
            {
                QUuid genCompUuid = node->getAttribute<QUuid>("generic_component", true);
                QMap<Version, FilePath> filepaths = lib.getGenericComponents(genCompUuid);
                if (!filepaths.isEmpty())
                {
                    // copy generic component
                    GenericComponent latestGenComp(filepaths.last());
                    FilePath dest = projectFilepath.getParentDir().getPathTo("lib/gencmp");
                    latestGenComp.saveTo(dest);
                    ui->log->addItem(dest.toNative());

                    // search all required symbols
                    foreach (const GenCompSymbVar* symbvar, latestGenComp.getSymbolVariants())
                    {
                        foreach (const GenCompSymbVarItem* item, symbvar->getItems())
                        {
                            QUuid symbolUuid = item->getSymbolUuid();
                            QMap<Version, FilePath> filepaths = lib.getSymbols(symbolUuid);
                            if (!filepaths.isEmpty())
                            {
                                Symbol latestSymbol(filepaths.last());
                                FilePath dest = projectFilepath.getParentDir().getPathTo("lib/sym");
                                latestSymbol.saveTo(dest);
                                ui->log->addItem(dest.toNative());
                            }
                            else
                            {
                                throw RuntimeError(__FILE__, __LINE__, projectFilepath.toStr(),
                                    QString("missing symbol: %1").arg(symbolUuid.toString()));
                            }
                        }
                    }
                }
                else
                {
                    throw RuntimeError(__FILE__, __LINE__, projectFilepath.toStr(),
                        QString("missing generic component: %1").arg(genCompUuid.toString()));
                }
            }


            // components & footprints
            for (XmlDomElement* node = projectDoc->getRoot().getFirstChild("boards/*", true, false);
                 node; node = node->getNextSibling())
            {
                FilePath boardFilePath = projectFilepath.getParentDir().getPathTo("boards/" % node->getText(true));
                SmartXmlFile boardFile(boardFilePath, false, true);
                QSharedPointer<XmlDomDocument> boardDoc = boardFile.parseFileAndBuildDomTree(true);
                for (XmlDomElement* node = boardDoc->getRoot().getFirstChild("component_instances/*", true, false);
                     node; node = node->getNextSibling())
                {
                    QUuid compUuid = node->getAttribute<QUuid>("component", true);
                    QMap<Version, FilePath> filepaths = lib.getComponents(compUuid);
                    if (!filepaths.isEmpty())
                    {
                        // copy component
                        Component latestComp(filepaths.last());
                        FilePath dest = projectFilepath.getParentDir().getPathTo("lib/cmp");
                        latestComp.saveTo(dest);
                        ui->log->addItem(dest.toNative());

                        // get package
                        QUuid packUuid = latestComp.getPackageUuid();
                        QMap<Version, FilePath> filepaths = lib.getPackages(packUuid);
                        if (!filepaths.isEmpty())
                        {
                            // copy package
                            Package latestPackage(filepaths.last());
                            FilePath dest = projectFilepath.getParentDir().getPathTo("lib/pkg");
                            latestPackage.saveTo(dest);
                            ui->log->addItem(dest.toNative());

                            // get footprint
                            QUuid footprintUuid = latestPackage.getFootprintUuid();
                            QMap<Version, FilePath> filepaths = lib.getFootprints(footprintUuid);
                            if (!filepaths.isEmpty())
                            {
                                // copy footprint
                                Footprint latestFootprint(filepaths.last());
                                FilePath dest = projectFilepath.getParentDir().getPathTo("lib/fpt");
                                latestFootprint.saveTo(dest);
                                ui->log->addItem(dest.toNative());
                            }
                            else
                            {
                                throw RuntimeError(__FILE__, __LINE__, projectFilepath.toStr(),
                                    QString("missing footprint: %1").arg(footprintUuid.toString()));
                            }
                        }
                        else
                        {
                            throw RuntimeError(__FILE__, __LINE__, projectFilepath.toStr(),
                                QString("missing package: %1").arg(packUuid.toString()));
                        }
                    }
                    else
                    {
                        throw RuntimeError(__FILE__, __LINE__, projectFilepath.toStr(),
                            QString("missing component: %1").arg(compUuid.toString()));
                    }
                }
            }
        }
    }
    catch (Exception& e)
    {
        ui->log->addItem("ERROR: " % e.getUserMsg());
    }

    ui->log->addItem("FINISHED");
    ui->log->setCurrentRow(ui->log->count()-1);
}

void MainWindow::on_rescanlib_clicked()
{
    if (ui->workspacelibrarypath->text().isEmpty()) return;

    try
    {
        FilePath libDir(ui->workspacelibrarypath->text());
        Library lib(libDir);
        lib.rescan();
        QMessageBox::information(this, tr("Library Rescan"), tr("Successfully"));
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}
