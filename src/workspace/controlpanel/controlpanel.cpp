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

ControlPanel::ControlPanel(Workspace* workspace, QAbstractItemModel* projectTreeModel,
                           QAbstractItemModel* recentProjectsModel,
                           QAbstractItemModel* favoriteProjectsModel) :
    QMainWindow(0), mUi(new Ui::ControlPanel), mWorkspace(workspace)
{
    mUi->setupUi(this);

    setWindowTitle("EDA4U Control Panel - " % mWorkspace->getWorkspaceDir().absolutePath());

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(mUi->actionOpen_Library_Editor, SIGNAL(triggered()), mWorkspace, SLOT(openLibraryEditor()));
    connect(mUi->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    mUi->projectTreeView->setModel(projectTreeModel);
    mUi->recentProjectsListView->setModel(recentProjectsModel);
    mUi->favoriteProjectsListView->setModel(favoriteProjectsModel);

    mUi->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(mUi->webView, &QWebView::linkClicked, [](const QUrl& url){QDesktopServices::openUrl(url);});

    // restore all settings
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    restoreGeometry(settings.value("controlpanel/window_geometry").toByteArray());
    restoreState(settings.value("controlpanel/window_state").toByteArray());
    mUi->splitter_h->restoreState(settings.value("controlpanel/splitter_h_state").toByteArray());
    mUi->splitter_v->restoreState(settings.value("controlpanel/splitter_v_state").toByteArray());
}

ControlPanel::~ControlPanel()
{
    delete mUi;              mUi = 0;
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    // save all settings
    QSettings settings(mWorkspace->getWorkspaceSettingsIniFilename(), QSettings::IniFormat);
    settings.setValue("controlpanel/window_geometry", saveGeometry());
    settings.setValue("controlpanel/window_state", saveState());
    settings.setValue("controlpanel/splitter_h_state", mUi->splitter_h->saveState());
    settings.setValue("controlpanel/splitter_v_state", mUi->splitter_v->saveState());

    // close all projects, unsaved projects will ask for saving
    mWorkspace->closeAllProjects(true);

    // if the control panel is closed, we will quit the whole application
    QApplication::quit();
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
    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());
    if (!item)
        return;

    QString htmlFilename;

    if ((item->getType() == ProjectTreeItem::ProjectFolder) || (item->getType() == ProjectTreeItem::ProjectFile))
        htmlFilename = item->getFileInfo().dir().absoluteFilePath("description" % QDir::separator() % "index.html");

    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilename));
}

void ControlPanel::on_projectTreeView_doubleClicked(const QModelIndex& index)
{
    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());

    if (!item)
        return;

    switch (item->getType())
    {
        case ProjectTreeItem::File:
            QDesktopServices::openUrl(QUrl::fromLocalFile(item->getFileInfo().absoluteFilePath()));
            break;

        case ProjectTreeItem::Folder:
        case ProjectTreeItem::ProjectFolder:
            mUi->projectTreeView->setExpanded(index, !mUi->projectTreeView->isExpanded(index));
            break;

        case ProjectTreeItem::ProjectFile:
            mWorkspace->openProject(item->getFileInfo().filePath());
            break;

        default:
            break;
    }
}

void ControlPanel::on_projectTreeView_customContextMenuRequested(const QPoint& pos)
{
    QMenu menu;
    QMap<unsigned int, QAction*> actions;

    QModelIndex index = mUi->projectTreeView->indexAt(pos);
    ProjectTreeItem* item = 0;

    if (index.isValid())
    {
        item = static_cast<ProjectTreeItem*>(index.internalPointer());
        if (item)
        {
            if (item->getType() == ProjectTreeItem::ProjectFile)
            {
                if (!mWorkspace->getOpenProject(item->getFileInfo().absoluteFilePath()))
                {
                    // this project is not open
                    actions.insert(1, menu.addAction("Open Project"));
                    actions.value(1)->setIcon(QIcon(":/img/actions/open.png"));
                }
                else
                {
                    // this project is open
                    actions.insert(2, menu.addAction("Close Project"));
                    actions.value(2)->setIcon(QIcon(":/img/actions/close.png"));
                }

                if (mWorkspace->isFavoriteProject(item->getFileInfo().filePath()))
                {
                    // this is a favorite project
                    actions.insert(3, menu.addAction("Remove from favorites"));
                    actions.value(3)->setIcon(QIcon(":/img/actions/bookmark.png"));
                }
                else
                {
                    // this is not a favorite project
                    actions.insert(4, menu.addAction("Add to favorites"));
                    actions.value(4)->setIcon(QIcon(":/img/actions/bookmark_gray.png"));
                }

                actions.insert(100, menu.addSeparator());
            }
            else
            {
                // a folder or a file is selected

                actions.insert(10, menu.addAction("New Project"));
                actions.value(10)->setIcon(QIcon(":/img/actions/new.png"));
            }

            actions.insert(20, menu.addAction("New Folder"));
            actions.value(20)->setIcon(QIcon(":/img/actions/new_folder.png"));

            actions.insert(101, menu.addSeparator());

            actions.insert(21, menu.addAction("Open Directory"));
            actions.value(21)->setIcon(QIcon(":/img/places/folder_open.png"));

            actions.insert(102, menu.addSeparator());
        }
    }

    switch (actions.key(menu.exec(QCursor::pos()), 0))
    {
        case 1: // open project
            mWorkspace->openProject(item->getFileInfo().absoluteFilePath());
            break;

        case 2: // close project
            mWorkspace->closeProject(item->getFileInfo().absoluteFilePath(), true);
            break;

        case 3: // remove project from favorites
            mWorkspace->removeFavoriteProject(item->getFileInfo().filePath());
            break;

        case 4: // add project to favorites
            mWorkspace->addFavoriteProject(item->getFileInfo().filePath());
            break;

        case 10: // new project
            break;

        case 20: // new folder
            break;

        case 21: // open project directory
            QDesktopServices::openUrl(QUrl::fromLocalFile(item->getFileInfo().absoluteFilePath()));
            break;

        default:
            break;
    }

    qDeleteAll(actions);
}

void ControlPanel::on_recentProjectsListView_entered(const QModelIndex &index)
{
    QFileInfo fileInfo(index.data(Qt::UserRole).toString());
    QString htmlFilename = fileInfo.dir().absoluteFilePath("description" %
                                                           QDir::separator() %
                                                           "index.html");
    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilename));
}

void ControlPanel::on_favoriteProjectsListView_entered(const QModelIndex &index)
{
    QFileInfo fileInfo(index.data(Qt::UserRole).toString());
    QString htmlFilename = fileInfo.dir().absoluteFilePath("description" %
                                                           QDir::separator() %
                                                           "index.html");
    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilename));
}

void ControlPanel::on_recentProjectsListView_clicked(const QModelIndex &index)
{
    QFileInfo fileInfo(index.data(Qt::UserRole).toString());
    if ((!fileInfo.isFile()) || (!fileInfo.exists()))
        return;

    mWorkspace->openProject(fileInfo.filePath());
}

void ControlPanel::on_favoriteProjectsListView_clicked(const QModelIndex &index)
{
    QFileInfo fileInfo(index.data(Qt::UserRole).toString());
    if ((!fileInfo.isFile()) || (!fileInfo.exists()))
        return;

    mWorkspace->openProject(fileInfo.filePath());
}

void ControlPanel::on_favoriteProjectsListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = mUi->favoriteProjectsListView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu menu;
    QAction* removeAction = menu.addAction(QIcon(":/img/actions/cancel.png"),
                                           "Remove from favorites");

    if (menu.exec(QCursor::pos()) == removeAction)
        mWorkspace->removeFavoriteProject(index.data(Qt::UserRole).toString());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
