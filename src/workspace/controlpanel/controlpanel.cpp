/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <QFileDialog>
#include "controlpanel.h"
#include "ui_controlpanel.h"

#include "../workspace.h"
#include "../../project/project.h"
#include "../projecttreeitem.h"

using namespace project;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ControlPanel::ControlPanel(Workspace* workspace, QAbstractItemModel* projectTreeModel) :
    QMainWindow(0), ui(new Ui::ControlPanel), mWorkspace(workspace)
{
    ui->setupUi(this);

    setWindowTitle("EDA4U Control Panel - " + mWorkspace->getWorkspaceDir().absolutePath());

    // connect some actions which are created with the Qt Designer
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionOpen_Library_Editor, SIGNAL(triggered()), mWorkspace, SLOT(openLibraryEditor()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    ui->projectTreeView->setModel(projectTreeModel);

    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(webViewLinkClicked(QUrl)));
}

ControlPanel::~ControlPanel()
{
    delete ui;              ui = 0;
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    mWorkspace->closeAllProjects(true); // close all projects, unsaved projects will ask for saving
    QApplication::quit(); // if the control panel is closed, we will quit the whole application
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void ControlPanel::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About", "EDA4U is a free & OpenSource Schematic/Layout-Editor");
}

void ControlPanel::on_projectTreeView_clicked(const QModelIndex& index)
{
    QString html;

    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());
    if (!item)
        return;

    do
    {
        if (!item->isProject())
            break;

        QString htmlFilename = QFileInfo(item->getProjectFilename()).absoluteDir().absoluteFilePath("description.html");
        QFile htmlFile(htmlFilename);
        if (!htmlFile.exists())
            break;
        if (!htmlFile.open(QFile::ReadOnly))
            break;
        html = htmlFile.readAll();
        htmlFile.close();
    } while (0);

    ui->webView->setHtml(html);
}

void ControlPanel::on_projectTreeView_doubleClicked(const QModelIndex& index)
{
    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());
    if (!item)
        return;
    if (!item->isProject())
        return;

    mWorkspace->openProject(item->getProjectFilename());
}

void ControlPanel::on_projectTreeView_customContextMenuRequested(const QPoint& pos)
{
    QMenu menu;
    QMap<unsigned int, QAction*> actions;

    QModelIndex index = ui->projectTreeView->indexAt(pos);
    ProjectTreeItem* item = 0;

    if (index.isValid())
    {
        item = static_cast<ProjectTreeItem*>(index.internalPointer());
        if (item)
        {
            if (item->isProject())
            {
                actions.insert(1, menu.addAction("Open Project"));
                actions.insert(2, menu.addAction("Close Project"));
                actions.insert(3, menu.addSeparator());
            }

            actions.insert(4, menu.addAction("Open Directory"));
            actions.insert(5, menu.addSeparator());
        }
    }

    static bool showFiles = false;
    actions.insert(6, menu.addAction("Show Files"));
    actions.value(6)->setCheckable(true);
    actions.value(6)->setChecked(showFiles);

    switch (actions.key(menu.exec(QCursor::pos()), 0))
    {
        case 1: // open project
            qDebug() << "open" << item->getName();
            break;

        case 2: // close project
            qDebug() << "close" << item->getName();
            break;

        case 4: // open project directory
            QDesktopServices::openUrl(QUrl("file:///" % item->getDir().absolutePath(), QUrl::TolerantMode));
            break;

        case 6: // show files (checkable)
            showFiles = actions.value(6)->isChecked();
            qDebug() << "show files = " << showFiles;
            break;

        default:
            break;
    }

    qDeleteAll(actions);
}

void ControlPanel::webViewLinkClicked(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
