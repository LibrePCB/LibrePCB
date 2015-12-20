/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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
#include "unplacedcomponentsdock.h"
#include "ui_unplacedcomponentsdock.h"
#include <librepcbproject/boards/board.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/project.h>
#include <librepcbproject/settings/projectsettings.h>
#include <librepcbproject/circuit/gencompinstance.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/elements.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbcommon/graphics/graphicsscene.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceadd.h>
#include <librepcbcommon/undostack.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbworkspace/workspace.h>
#include "../projecteditor.h"
#include <librepcbproject/library/cmd/cmdprojectlibraryaddelement.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UnplacedComponentsDock::UnplacedComponentsDock(ProjectEditor& editor) :
    QDockWidget(0), mProjectEditor(editor), mProject(editor.getProject()), mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mFootprintPreviewGraphicsView(nullptr), mFootprintPreviewGraphicsScene(nullptr),
    mSelectedGenComp(nullptr), mSelectedDevice(nullptr),
    mCircuitConnection1(), mCircuitConnection2(), mBoardConnection1(), mBoardConnection2(),
    mDisableListUpdate(false)
{
    mUi->setupUi(this);
    /*mFootprintPreviewGraphicsScene = new GraphicsScene();
    mFootprintPreviewGraphicsView = new GraphicsView();
    mFootprintPreviewGraphicsView->setScene(mFootprintPreviewGraphicsScene);

    mCircuitConnection1 = connect(&mProject.getCircuit(), &Circuit::genCompAdded,
                                  [this](GenCompInstance& gc){Q_UNUSED(gc); updateComponentsList();});
    mCircuitConnection2 = connect(&mProject.getCircuit(), &Circuit::genCompRemoved,
                                  [this](GenCompInstance& gc){Q_UNUSED(gc); updateComponentsList();});

    updateComponentsList();*/
}

UnplacedComponentsDock::~UnplacedComponentsDock()
{
    setBoard(nullptr);
    mDisableListUpdate = true;
    disconnect(mCircuitConnection1);        mCircuitConnection1 = QMetaObject::Connection();
    disconnect(mCircuitConnection2);        mCircuitConnection2 = QMetaObject::Connection();
    delete mFootprintPreviewGraphicsView;   mFootprintPreviewGraphicsView = nullptr;
    delete mFootprintPreviewGraphicsScene;  mFootprintPreviewGraphicsScene = nullptr;
    delete mUi;                             mUi = nullptr;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void UnplacedComponentsDock::setBoard(Board* board)
{
    // clean up
    /*mBoard = nullptr;
    disconnect(mBoardConnection1);  mBoardConnection1 = QMetaObject::Connection();
    disconnect(mBoardConnection2);  mBoardConnection2 = QMetaObject::Connection();
    updateComponentsList();

    // load new board
    mBoard = board;
    if (board)
    {
        mBoardConnection1 = connect(board, &Board::deviceAdded, [this](DeviceInstance& c){Q_UNUSED(c); updateComponentsList();});
        mBoardConnection2 = connect(board, &Board::deviceRemoved, [this](DeviceInstance& c){Q_UNUSED(c); updateComponentsList();});
        mNextPosition = Point::fromMm(0, -20).mappedToGrid(board->getGridProperties().getInterval());
        updateComponentsList();
    }*/
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void UnplacedComponentsDock::on_lstUnplacedComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    /*Q_UNUSED(previous);

    GenCompInstance* genComp = nullptr;
    if (mBoard && current)
    {
        QUuid genCompUuid = current->data(Qt::UserRole).toUuid();
        Q_ASSERT(genCompUuid.isNull() == false);
        genComp = mProject.getCircuit().getGenCompInstanceByUuid(genCompUuid);
    }
    setSelectedGenCompInstance(genComp);*/
}

void UnplacedComponentsDock::on_cbxSelectedComponent_currentIndexChanged(int index)
{
    /*QUuid deviceUuid = mUi->cbxSelectedComponent->itemData(index, Qt::UserRole).toUuid();
    FilePath devFp = mProjectEditor.getWorkspace().getLibrary().getLatestDevice(deviceUuid);
    if (devFp.isValid())
    {
        const library::Device* device = new library::Device(devFp);
        setSelectedDevice(device);
    }
    else
    {
        setSelectedDevice(nullptr);
    }*/
}

void UnplacedComponentsDock::on_btnAdd_clicked()
{
    /*if (mBoard && mSelectedGenComp && mSelectedComponent)
        addComponent(*mSelectedGenComp, mSelectedComponent->getUuid());
    updateComponentsList();*/
}

void UnplacedComponentsDock::on_pushButton_clicked()
{
    /*if ((!mBoard) || (!mSelectedGenComp) || (!mSelectedComponent)) return;

    QUuid genCompLibUuid = mSelectedGenComp->getGenComp().getUuid();
    QUuid compLibUuid = mSelectedComponent->getUuid();

    mDisableListUpdate = true;
    for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++)
    {
        QUuid genCompUuid = mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toUuid();
        Q_ASSERT(genCompUuid.isNull() == false);
        GenCompInstance* genComp = mProject.getCircuit().getGenCompInstanceByUuid(genCompUuid);
        if (!genComp) continue;
        if (genComp->getGenComp().getUuid() != genCompLibUuid) continue;
        addComponent(*genComp, compLibUuid);
    }
    mDisableListUpdate = false;

    updateComponentsList();*/
}

void UnplacedComponentsDock::on_btnAddAll_clicked()
{
    /*if (!mBoard) return;

    mDisableListUpdate = true;
    for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++)
    {
        QUuid genCompUuid = mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toUuid();
        Q_ASSERT(genCompUuid.isNull() == false);
        GenCompInstance* genComp = mProject.getCircuit().getGenCompInstanceByUuid(genCompUuid);
        if (genComp)
        {
            QList<QUuid> components = mProjectEditor.getWorkspace().getLibrary().
                getComponentsOfGenericComponent(genComp->getGenComp().getUuid()).toList();
            if (components.count() > 0)
                addComponent(*genComp, components.first());
        }
    }
    mDisableListUpdate = false;

    updateComponentsList();*/
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

/*void UnplacedComponentsDock::updateComponentsList() noexcept
{
    if (mDisableListUpdate) return;

    setSelectedGenCompInstance(nullptr);
    mUi->lstUnplacedComponents->clear();

    if (mBoard)
    {
        const QHash<QUuid, GenCompInstance*> genCompList = mProject.getCircuit().getGenCompInstances();
        const QHash<QUuid, ComponentInstance*> boardCompList = mBoard->getComponentInstances();
        foreach (GenCompInstance* genComp, genCompList)
        {
            if (boardCompList.contains(genComp->getUuid())) continue;
            if (genComp->getGenComp().isSchematicOnly()) continue;

            // add generic component to list
            int compCount = mProjectEditor.getWorkspace().getLibrary().getComponentsOfGenericComponent(genComp->getGenComp().getUuid()).count();
            QString name = genComp->getName();
            QString value = genComp->getValue(true).replace("\n", "|");
            QString genCompName = genComp->getGenComp().getName(mProject.getSettings().getLocaleOrder());
            QString text = QString("{%1} %2 (%3) [%4]").arg(compCount).arg(name, value, genCompName);
            QListWidgetItem* item = new QListWidgetItem(text, mUi->lstUnplacedComponents);
            item->setData(Qt::UserRole, genComp->getUuid());
        }
    }
}

void UnplacedComponentsDock::setSelectedGenCompInstance(GenCompInstance* genComp) noexcept
{
    setSelectedComponent(nullptr);
    mUi->cbxSelectedComponent->clear();
    mSelectedGenComp = genComp;

    if (mBoard && mSelectedGenComp)
    {
        QStringList localeOrder = mProject.getSettings().getLocaleOrder();
        QSet<QUuid> components = mProjectEditor.getWorkspace().getLibrary().getComponentsOfGenericComponent(mSelectedGenComp->getGenComp().getUuid());
        foreach (const QUuid& compUuid, components)
        {
            // TODO: use library metadata instead of loading the XML files
            FilePath cmpFp = mProjectEditor.getWorkspace().getLibrary().getLatestComponent(compUuid);
            if (!cmpFp.isValid()) continue;
            const library::Component component(cmpFp);

            QUuid pkgUuid;
            mProjectEditor.getWorkspace().getLibrary().getComponentMetadata(cmpFp, &pkgUuid);
            FilePath pkgFp = mProjectEditor.getWorkspace().getLibrary().getLatestPackage(pkgUuid);
            const library::Package package(pkgFp);

            QString cmpName = component.getName(localeOrder);
            QString pkgName = package.getName(localeOrder);
            QString text = QString("%1 [%2]").arg(cmpName, pkgName);
            mUi->cbxSelectedComponent->addItem(text, compUuid);
        }
        if (mUi->cbxSelectedComponent->count() > 0)
            mUi->cbxSelectedComponent->setCurrentIndex(0);
    }
}

void UnplacedComponentsDock::setSelectedDevice(const library::Device* device) noexcept
{
    mUi->btnAdd->setEnabled(false);
    delete mSelectedComponent;
    mSelectedComponent = nullptr;

    if (mBoard && mSelectedGenComp && device)
    {
        if (device->getGenCompUuid() == mSelectedGenComp->getGenComp().getUuid())
        {
            mSelectedComponent = device;
            mUi->btnAdd->setEnabled(true);
        }
    }
}

void UnplacedComponentsDock::addComponent(GenCompInstance& genComp, const QUuid& component) noexcept
{
    Q_ASSERT(mBoard);
    bool cmdActive = false;

    try
    {
        mProjectEditor.getUndoStack().beginCommand(tr("Add component to board"));
        cmdActive = true;

        const library::Component* cmp = mProject.getLibrary().getComponent(component);
        if (!cmp)
        {
            // copy the component to the project's library
            FilePath cmpFp = mProjectEditor.getWorkspace().getLibrary().getLatestComponent(component);
            if (!cmpFp.isValid())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("Component not found in library: %1"))
                    .arg(component.toString()));
            }
            cmp = new library::Component(cmpFp);
            auto cmd = new CmdProjectLibraryAddElement<library::Component>(mProject.getLibrary(), *cmp);
            mProjectEditor.getUndoStack().appendToCommand(cmd);
        }

        const library::Package* pkg = mProject.getLibrary().getPackage(cmp->getPackageUuid());
        if (!pkg)
        {
            // copy the package to the project's library
            FilePath pkgFp = mProjectEditor.getWorkspace().getLibrary().getLatestPackage(cmp->getPackageUuid());
            if (!pkgFp.isValid())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("Package not found in library: %1"))
                    .arg(cmp->getPackageUuid().toString()));
            }
            pkg = new library::Package(pkgFp);
            auto cmd = new CmdProjectLibraryAddElement<library::Package>(mProject.getLibrary(), *pkg);
            mProjectEditor.getUndoStack().appendToCommand(cmd);
        }

        const library::Footprint* fpt = mProject.getLibrary().getFootprint(pkg->getFootprintUuid());
        if (!fpt)
        {
            // copy the package to the project's library
            FilePath fptFp = mProjectEditor.getWorkspace().getLibrary().getLatestFootprint(pkg->getFootprintUuid());
            if (!fptFp.isValid())
            {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("Package not found in library: %1"))
                    .arg(pkg->getFootprintUuid().toString()));
            }
            fpt = new library::Footprint(fptFp);
            auto cmd = new CmdProjectLibraryAddElement<library::Footprint>(mProject.getLibrary(), *fpt);
            mProjectEditor.getUndoStack().appendToCommand(cmd);
        }

        // add component to board
        CmdComponentInstanceAdd* cmd = new CmdComponentInstanceAdd(*mBoard, genComp, component, mNextPosition);
        mProjectEditor.getUndoStack().appendToCommand(cmd);
        if (mNextPosition.getX() > Length::fromMm(200))
            mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
        else
            mNextPosition += Point::fromMm(10, 0);
        mNextPosition.mapToGrid(mBoard->getGridProperties().getInterval());

        mProjectEditor.getUndoStack().endCommand();
        cmdActive = false;
    }
    catch (Exception& e)
    {
        try {if (cmdActive) mProjectEditor.getUndoStack().abortCommand();} catch (...) {}
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}*/

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
