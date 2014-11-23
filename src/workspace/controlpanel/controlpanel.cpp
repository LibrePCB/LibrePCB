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
#include "../settings/workspacesettings.h"
#include "../../project/project.h"
#include "../projecttreemodel.h"
#include "../projecttreeitem.h"

using namespace project;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ControlPanel::ControlPanel(QAbstractItemModel* projectTreeModel,
                           QAbstractItemModel* recentProjectsModel,
                           QAbstractItemModel* favoriteProjectsModel) :
    QMainWindow(0), mUi(new Ui::ControlPanel)
{
    mUi->setupUi(this);

    setWindowTitle(QString(tr("Control Panel - EDA4U %1 - %2"))
        .arg(QCoreApplication::applicationVersion()).arg(Workspace::instance().getPath().toNative()));
    mUi->statusBar->addWidget(new QLabel(QString(tr("Workspace: %1"))
                                         .arg(Workspace::instance().getPath().toNative())));

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(mUi->actionOpen_Library_Editor, SIGNAL(triggered()), &Workspace::instance(), SLOT(openLibraryEditor()));
    connect(mUi->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(mUi->actionWorkspace_Settings, SIGNAL(triggered()), &Workspace::instance().getSettings(), SLOT(showSettingsDialog()));

    mUi->projectTreeView->setModel(projectTreeModel);
    mUi->recentProjectsListView->setModel(recentProjectsModel);
    mUi->favoriteProjectsListView->setModel(favoriteProjectsModel);

    mUi->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(mUi->webView, &QWebView::linkClicked, [](const QUrl& url){QDesktopServices::openUrl(url);});

    loadSettings();
}

ControlPanel::~ControlPanel()
{
    delete mUi;              mUi = 0;
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    saveSettings();

    // close all projects, unsaved projects will ask for saving
    if (!Workspace::instance().closeAllProjects(true))
    {
        event->ignore();
        return; // do NOT close the application, there are still open projects!
    }

    QMainWindow::closeEvent(event);

    // if the control panel is closed, we will quit the whole application
    QApplication::quit();
}

/*****************************************************************************************
 *  General private methods
 ****************************************************************************************/

void ControlPanel::saveSettings()
{
    QSettings clientSettings;
    clientSettings.beginGroup("controlpanel");

    // main window
    clientSettings.setValue("window_geometry", saveGeometry());
    clientSettings.setValue("window_state", saveState());
    clientSettings.setValue("splitter_h_state", mUi->splitter_h->saveState());
    clientSettings.setValue("splitter_v_state", mUi->splitter_v->saveState());

    // projects treeview (expanded items)
    ProjectTreeModel* model = dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model());
    if (model)
    {
        QStringList list;
        foreach (QModelIndex index, model->getPersistentIndexList())
        {
            if (mUi->projectTreeView->isExpanded(index))
            {
                list.append(FilePath(index.data(Qt::UserRole).toString())
                            .toRelative(Workspace::instance().getPath()));
            }
        }
        clientSettings.setValue("expanded_projecttreeview_items", QVariant::fromValue(list));
    }

    clientSettings.endGroup();
}

void ControlPanel::loadSettings()
{
    QSettings clientSettings;
    clientSettings.beginGroup("controlpanel");

    // main window
    restoreGeometry(clientSettings.value("window_geometry").toByteArray());
    restoreState(clientSettings.value("window_state").toByteArray());
    mUi->splitter_h->restoreState(clientSettings.value("splitter_h_state").toByteArray());
    mUi->splitter_v->restoreState(clientSettings.value("splitter_v_state").toByteArray());

    // projects treeview (expanded items)
    ProjectTreeModel* model = dynamic_cast<ProjectTreeModel*>(mUi->projectTreeView->model());
    if (model)
    {
        QStringList list = clientSettings.value("expanded_projecttreeview_items").toStringList();
        foreach (QString item, list)
        {
            FilePath filepath = FilePath::fromRelative(Workspace::instance().getPath(), item);
            QModelIndexList items = model->match(model->index(0, 0), Qt::UserRole,
                QVariant::fromValue(filepath.toStr()), 1, Qt::MatchExactly | Qt::MatchWrap | Qt::MatchRecursive);
            if (!items.isEmpty())
                mUi->projectTreeView->setExpanded(items.first(), true);
        }
    }

    clientSettings.endGroup();
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void ControlPanel::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About"),
                       tr("EDA4U is a free & OpenSource Schematic/Layout-Editor"));
}

void ControlPanel::on_actionNew_Project_triggered()
{
    QSettings settings(Workspace::instance().getMetadataPath().getPathTo("settings.ini").toStr(),
                       QSettings::IniFormat);
    FilePath lastNewFile(settings.value("controlpanel/last_new_project").toString());
    if (!lastNewFile.getParentDir().isExistingDir())
        lastNewFile.setPath(Workspace::instance().getProjectsPath().toStr());

    FilePath filepath(QFileDialog::getSaveFileName(this, tr("New Project"), lastNewFile.toStr(),
                                    tr("EDA4U project files (%1)").arg("*.e4u")));

    if (!filepath.isValid())
        return;

    settings.setValue("controlpanel/last_new_project", filepath.toNative());

    Workspace::instance().createProject(filepath);
}

void ControlPanel::on_actionOpen_Project_triggered()
{
    QSettings settings(Workspace::instance().getMetadataPath().getPathTo("settings.ini").toStr(),
                       QSettings::IniFormat);
    QString lastOpenedFile = settings.value("controlpanel/last_open_project",
                             Workspace::instance().getPath().toStr()).toString();

    FilePath filepath(QFileDialog::getOpenFileName(this, tr("Open Project"), lastOpenedFile,
                                    tr("EDA4U project files (%1)").arg("*.e4u")));

    if (!filepath.isValid())
        return;

    settings.setValue("controlpanel/last_open_project", filepath.toNative());

    Workspace::instance().openProject(filepath);
}

void ControlPanel::on_actionClose_all_open_projects_triggered()
{
    Workspace::instance().closeAllProjects(true);
}

void ControlPanel::on_actionSwitch_Workspace_triggered()
{
    FilePath wsPath = Workspace::chooseWorkspacePath();
    if (!wsPath.isValid())
        return;

    Workspace::setMostRecentlyUsedWorkspacePath(wsPath);
    QMessageBox::information(this, tr("Workspace changed"),
        tr("The chosen workspace will be used after restarting the application."));
}

void ControlPanel::on_projectTreeView_clicked(const QModelIndex& index)
{
    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());
    if (!item)
        return;

    FilePath htmlFilepath;

    if ((item->getType() == ProjectTreeItem::ProjectFolder) || (item->getType() == ProjectTreeItem::ProjectFile))
        htmlFilepath = item->getFilePath().getParentDir().getPathTo("description/index.html");

    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilepath.toStr()));
}

void ControlPanel::on_projectTreeView_doubleClicked(const QModelIndex& index)
{
    ProjectTreeItem* item = static_cast<ProjectTreeItem*>(index.internalPointer());

    if (!item)
        return;

    switch (item->getType())
    {
        case ProjectTreeItem::File:
            QDesktopServices::openUrl(QUrl::fromLocalFile(item->getFilePath().toStr()));
            break;

        case ProjectTreeItem::Folder:
        case ProjectTreeItem::ProjectFolder:
            mUi->projectTreeView->setExpanded(index, !mUi->projectTreeView->isExpanded(index));
            break;

        case ProjectTreeItem::ProjectFile:
            Workspace::instance().openProject(item->getFilePath());
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
                if (!Workspace::instance().getOpenProject(item->getFilePath()))
                {
                    // this project is not open
                    actions.insert(1, menu.addAction(tr("Open Project")));
                    actions.value(1)->setIcon(QIcon(":/img/actions/open.png"));
                }
                else
                {
                    // this project is open
                    actions.insert(2, menu.addAction(tr("Close Project")));
                    actions.value(2)->setIcon(QIcon(":/img/actions/close.png"));
                }

                if (Workspace::instance().isFavoriteProject(item->getFilePath()))
                {
                    // this is a favorite project
                    actions.insert(3, menu.addAction(tr("Remove from favorites")));
                    actions.value(3)->setIcon(QIcon(":/img/actions/bookmark.png"));
                }
                else
                {
                    // this is not a favorite project
                    actions.insert(4, menu.addAction(tr("Add to favorites")));
                    actions.value(4)->setIcon(QIcon(":/img/actions/bookmark_gray.png"));
                }

                actions.insert(100, menu.addSeparator());
            }
            else
            {
                // a folder or a file is selected

                actions.insert(10, menu.addAction(tr("New Project")));
                actions.value(10)->setIcon(QIcon(":/img/actions/new.png"));
            }

            actions.insert(20, menu.addAction(tr("New Folder")));
            actions.value(20)->setIcon(QIcon(":/img/actions/new_folder.png"));

            actions.insert(101, menu.addSeparator());

            actions.insert(21, menu.addAction(tr("Open Directory")));
            actions.value(21)->setIcon(QIcon(":/img/places/folder_open.png"));

            actions.insert(102, menu.addSeparator());
        }
    }

    switch (actions.key(menu.exec(QCursor::pos()), 0))
    {
        case 1: // open project
            Workspace::instance().openProject(item->getFilePath());
            break;

        case 2: // close project
            Workspace::instance().closeProject(item->getFilePath(), true);
            break;

        case 3: // remove project from favorites
            Workspace::instance().removeFavoriteProject(item->getFilePath());
            break;

        case 4: // add project to favorites
            Workspace::instance().addFavoriteProject(item->getFilePath());
            break;

        case 10: // new project
            break;

        case 20: // new folder
            break;

        case 21: // open project directory
            QDesktopServices::openUrl(QUrl::fromLocalFile(item->getFilePath().toStr()));
            break;

        default:
            break;
    }

    qDeleteAll(actions);
}

void ControlPanel::on_recentProjectsListView_entered(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    FilePath htmlFilepath = filepath.getParentDir().getPathTo("description/index.html");
    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilepath.toStr()));
}

void ControlPanel::on_favoriteProjectsListView_entered(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    FilePath htmlFilepath = filepath.getParentDir().getPathTo("description/index.html");
    mUi->webView->setUrl(QUrl::fromLocalFile(htmlFilepath.toStr()));
}

void ControlPanel::on_recentProjectsListView_clicked(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    Workspace::instance().openProject(filepath);
}

void ControlPanel::on_favoriteProjectsListView_clicked(const QModelIndex &index)
{
    FilePath filepath(index.data(Qt::UserRole).toString());
    Workspace::instance().openProject(filepath);
}

void ControlPanel::on_recentProjectsListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = mUi->recentProjectsListView->indexAt(pos);
    if (!index.isValid())
        return;

    bool isFavorite = Workspace::instance().isFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));

    QMenu menu;
    QAction* action;
    if (isFavorite)
    {
        action = menu.addAction(QIcon(":/img/actions/bookmark.png"),
                                           tr("Remove from favorites"));
    }
    else
    {
        action = menu.addAction(QIcon(":/img/actions/bookmark_gray.png"),
                                           tr("Add to favorites"));
    }

    if (menu.exec(QCursor::pos()) == action)
    {
        if (isFavorite)
            Workspace::instance().removeFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
        else
            Workspace::instance().addFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
    }
}

void ControlPanel::on_favoriteProjectsListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = mUi->favoriteProjectsListView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu menu;
    QAction* removeAction = menu.addAction(QIcon(":/img/actions/cancel.png"),
                                           tr("Remove from favorites"));

    if (menu.exec(QCursor::pos()) == removeAction)
        Workspace::instance().removeFavoriteProject(FilePath(index.data(Qt::UserRole).toString()));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
