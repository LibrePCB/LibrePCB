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
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbcommon/graphics/graphicsscene.h>
#include <librepcbproject/boards/cmd/cmdcomponentinstanceadd.h>
#include <librepcbcommon/undostack.h>
#include <librepcbcommon/gridproperties.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UnplacedComponentsDock::UnplacedComponentsDock(Project& project, UndoStack& undoStack) :
    QDockWidget(0), mProject(project), mUndoStack(undoStack), mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mFootprintPreviewGraphicsView(nullptr), mFootprintPreviewGraphicsScene(nullptr),
    mSelectedGenComp(nullptr), mSelectedComponent(nullptr),
    mCircuitConnection1(), mCircuitConnection2(), mBoardConnection1(), mBoardConnection2(),
    mDisableListUpdate(false)
{
    mUi->setupUi(this);
    mFootprintPreviewGraphicsScene = new GraphicsScene();
    mFootprintPreviewGraphicsView = new GraphicsView();
    mFootprintPreviewGraphicsView->setScene(mFootprintPreviewGraphicsScene);

    mCircuitConnection1 = connect(&mProject.getCircuit(), &Circuit::genCompAdded,
                                  [this](GenCompInstance& gc){Q_UNUSED(gc); updateComponentsList();});
    mCircuitConnection2 = connect(&mProject.getCircuit(), &Circuit::genCompRemoved,
                                  [this](GenCompInstance& gc){Q_UNUSED(gc); updateComponentsList();});

    updateComponentsList();
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
    mBoard = nullptr;
    disconnect(mBoardConnection1);  mBoardConnection1 = QMetaObject::Connection();
    disconnect(mBoardConnection2);  mBoardConnection2 = QMetaObject::Connection();
    updateComponentsList();

    // load new board
    mBoard = board;
    if (board)
    {
        mBoardConnection1 = connect(board, &Board::componentAdded, [this](ComponentInstance& c){Q_UNUSED(c); updateComponentsList();});
        mBoardConnection2 = connect(board, &Board::componentRemoved, [this](ComponentInstance& c){Q_UNUSED(c); updateComponentsList();});
        mNextPosition = Point::fromMm(0, -20).mappedToGrid(board->getGridProperties().getInterval());
        updateComponentsList();
    }
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void UnplacedComponentsDock::on_lstUnplacedComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    GenCompInstance* genComp = nullptr;
    if (mBoard && current)
    {
        QUuid genCompUuid = current->data(Qt::UserRole).toUuid();
        Q_ASSERT(genCompUuid.isNull() == false);
        genComp = mProject.getCircuit().getGenCompInstanceByUuid(genCompUuid);
    }
    setSelectedGenCompInstance(genComp);
}

void UnplacedComponentsDock::on_cbxSelectedComponent_currentIndexChanged(int index)
{
    QUuid componentUuid = mUi->cbxSelectedComponent->itemData(index, Qt::UserRole).toUuid();
    const library::Component* component = mProject.getLibrary().getComponent(componentUuid);
    setSelectedComponent(component);
}

void UnplacedComponentsDock::on_btnAdd_clicked()
{
    if (mBoard && mSelectedGenComp && mSelectedComponent)
        addComponent(*mSelectedGenComp, mSelectedComponent->getUuid());
    updateComponentsList();
}

void UnplacedComponentsDock::on_pushButton_clicked()
{
    if ((!mBoard) || (!mSelectedGenComp) || (!mSelectedComponent)) return;

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

    updateComponentsList();
}

void UnplacedComponentsDock::on_btnAddAll_clicked()
{
    if (!mBoard) return;

    mDisableListUpdate = true;
    for (int i = 0; i < mUi->lstUnplacedComponents->count(); i++)
    {
        QUuid genCompUuid = mUi->lstUnplacedComponents->item(i)->data(Qt::UserRole).toUuid();
        Q_ASSERT(genCompUuid.isNull() == false);
        GenCompInstance* genComp = mProject.getCircuit().getGenCompInstanceByUuid(genCompUuid);
        if (genComp)
        {
            QHash<QUuid, const library::Component*> components = mProject.getLibrary().getComponentsOfGenComp(genComp->getGenComp().getUuid());
            if (components.count() > 0)
            {
                addComponent(*genComp, components.keys().first());
            }
        }
    }
    mDisableListUpdate = false;

    updateComponentsList();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void UnplacedComponentsDock::updateComponentsList() noexcept
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
            int compCount = mProject.getLibrary().getComponentsOfGenComp(genComp->getGenComp().getUuid()).count();
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
        QHash<QUuid, const library::Component*> components = mProject.getLibrary().getComponentsOfGenComp(mSelectedGenComp->getGenComp().getUuid());
        foreach (const library::Component* component, components)
        {
            const library::Package* package = mProject.getLibrary().getPackage(component->getPackageUuid());
            QString cmpName = component->getName(localeOrder);
            QString pkgName = package ? package->getName(localeOrder) : "Package not found";
            QString text = QString("%1 [%2]").arg(cmpName, pkgName);
            mUi->cbxSelectedComponent->addItem(text, component->getUuid());
        }
        if (mUi->cbxSelectedComponent->count() > 0)
            mUi->cbxSelectedComponent->setCurrentIndex(0);
    }
}

void UnplacedComponentsDock::setSelectedComponent(const library::Component* component) noexcept
{
    mUi->btnAdd->setEnabled(false);
    mSelectedComponent = nullptr;

    if (mBoard && mSelectedGenComp && component)
    {
        if (component->getGenCompUuid() == mSelectedGenComp->getGenComp().getUuid())
        {
            mSelectedComponent = component;
            mUi->btnAdd->setEnabled(true);
        }
    }
}

void UnplacedComponentsDock::addComponent(GenCompInstance& genComp, const QUuid& component) noexcept
{
    Q_ASSERT(mBoard);

    try
    {
        CmdComponentInstanceAdd* cmd = new CmdComponentInstanceAdd(*mBoard, genComp, component, mNextPosition);
        mUndoStack.execCmd(cmd);
        if (mNextPosition.getX() > Length::fromMm(200))
            mNextPosition = Point::fromMm(0, mNextPosition.getY().toMm() - 10);
        else
            mNextPosition += Point::fromMm(10, 0);
        mNextPosition.mapToGrid(mBoard->getGridProperties().getInterval());
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
