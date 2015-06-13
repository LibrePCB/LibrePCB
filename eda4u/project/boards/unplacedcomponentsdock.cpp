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
#include "unplacedcomponentsdock.h"
#include "ui_unplacedcomponentsdock.h"
#include "board.h"
#include "../circuit/circuit.h"
#include "../project.h"
#include "../settings/projectsettings.h"
#include "../circuit/gencompinstance.h"
#include <eda4ulibrary/gencmp/genericcomponent.h>
#include <eda4ulibrary/cmp/component.h>
#include <eda4ulibrary/pkg/package.h>
#include "../library/projectlibrary.h"
#include <eda4ucommon/graphics/graphicsview.h>
#include <eda4ucommon/graphics/graphicsscene.h>
#include "cmd/cmdcomponentinstanceadd.h"
#include <eda4ucommon/undostack.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

UnplacedComponentsDock::UnplacedComponentsDock(Project& project) :
    QDockWidget(0), mProject(project), mBoard(nullptr),
    mUi(new Ui::UnplacedComponentsDock),
    mFootprintPreviewGraphicsView(nullptr), mFootprintPreviewGraphicsScene(nullptr),
    mSelectedGenComp(nullptr), mSelectedComponent(nullptr)
{
    mUi->setupUi(this);
    mFootprintPreviewGraphicsScene = new GraphicsScene();
    mFootprintPreviewGraphicsView = new GraphicsView();
    mFootprintPreviewGraphicsView->setScene(mFootprintPreviewGraphicsScene);

    connect(&mProject.getCircuit(), &Circuit::genCompAdded, this, &UnplacedComponentsDock::genCompAddedToOrRemovedFromCircuit);
    connect(&mProject.getCircuit(), &Circuit::genCompRemoved, this, &UnplacedComponentsDock::genCompAddedToOrRemovedFromCircuit);
    updateComponentsList();
}

UnplacedComponentsDock::~UnplacedComponentsDock()
{
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
    updateComponentsList();

    // load new board
    mBoard = board;
    updateComponentsList();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void UnplacedComponentsDock::genCompAddedToOrRemovedFromCircuit(GenCompInstance& genComp)
{
    Q_UNUSED(genComp);
    updateComponentsList();
}

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

void UnplacedComponentsDock::on_btnAddAll_clicked()
{
    if (!mBoard) return;

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
    updateComponentsList();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void UnplacedComponentsDock::updateComponentsList() noexcept
{
    setSelectedGenCompInstance(nullptr);
    mUi->lstUnplacedComponents->clear();

    if (mBoard)
    {
        const QHash<QUuid, GenCompInstance*> genCompList = mProject.getCircuit().getGenCompInstances();
        const QHash<QUuid, ComponentInstance*> boardCompList = mBoard->getComponentInstances();
        foreach (GenCompInstance* genComp, genCompList)
        {
            if (boardCompList.contains(genComp->getUuid())) continue;

            // add generic component to list
            uint compCount = mProject.getLibrary().getComponentsOfGenComp(genComp->getGenComp().getUuid()).count();
            QString name = genComp->getName();
            QString value = genComp->getValue(true).replace("\n", "|");
            QString genCompName = genComp->getGenComp().getName(mProject.getSettings().getLocaleOrder(true));
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
        QStringList localeOrder = mProject.getSettings().getLocaleOrder(true);
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
        CmdComponentInstanceAdd* cmd = new CmdComponentInstanceAdd(*mBoard, genComp, component);
        mProject.getUndoStack().execCmd(cmd);
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
