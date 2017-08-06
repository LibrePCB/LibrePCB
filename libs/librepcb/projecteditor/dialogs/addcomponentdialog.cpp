/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "addcomponentdialog.h"
#include "ui_addcomponentdialog.h"
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/project/project.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/cmp/componentsymbolvariant.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/library/sym/symbolpreviewgraphicsitem.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/common/gridproperties.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AddComponentDialog::AddComponentDialog(workspace::Workspace& workspace, Project& project,
                                       QWidget* parent) :
    QDialog(parent), mWorkspace(workspace), mProject(project),
    mUi(new Ui::AddComponentDialog), mPreviewScene(nullptr), mCategoryTreeModel(nullptr),
    mSelectedComponent(nullptr), mSelectedSymbVar(nullptr)
{
    mUi->setupUi(this);
    mPreviewScene = new GraphicsScene();
    mUi->graphicsView->setScene(mPreviewScene);
    mUi->graphicsView->setOriginCrossVisible(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();
    mCategoryTreeModel = new workspace::CategoryTreeModel(mWorkspace.getLibraryDb(), localeOrder);
    mUi->treeCategories->setModel(mCategoryTreeModel);
    connect(mUi->treeCategories->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AddComponentDialog::treeCategories_currentItemChanged);

    //setSelectedCategory(Uuid());
}

AddComponentDialog::~AddComponentDialog() noexcept
{
    qDeleteAll(mPreviewSymbolGraphicsItems);    mPreviewSymbolGraphicsItems.clear();
    mSelectedSymbVar = nullptr;
    delete mSelectedComponent;                  mSelectedComponent = nullptr;
    delete mCategoryTreeModel;                  mCategoryTreeModel = nullptr;
    delete mPreviewScene;                       mPreviewScene = nullptr;
    delete mUi;                                 mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Uuid AddComponentDialog::getSelectedComponentUuid() const noexcept
{
    if (mSelectedComponent && mSelectedSymbVar)
        return mSelectedComponent->getUuid();
    else
        return Uuid();
}

Uuid AddComponentDialog::getSelectedSymbVarUuid() const noexcept
{
    if (mSelectedComponent && mSelectedSymbVar)
        return mSelectedSymbVar->getUuid();
    else
        return Uuid();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void AddComponentDialog::treeCategories_currentItemChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    try
    {
        Uuid categoryUuid = Uuid(current.data(Qt::UserRole).toString());
        setSelectedCategory(categoryUuid);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void AddComponentDialog::on_listComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    try
    {
        if (current)
        {
            library::Component* component = new library::Component(
                FilePath(current->data(Qt::UserRole).toString()), true); // ugly...
            setSelectedComponent(component);
        }
        else
        {
            setSelectedComponent(nullptr);
        }
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        setSelectedComponent(nullptr);
    }
}

void AddComponentDialog::on_cbxSymbVar_currentIndexChanged(int index)
{
    if ((mSelectedComponent) && (index >= 0))
        setSelectedSymbVar(mSelectedComponent->getSymbolVariantByUuid(Uuid(mUi->cbxSymbVar->itemData(index).toString())));
    else
        setSelectedSymbVar(nullptr);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AddComponentDialog::setSelectedCategory(const Uuid& categoryUuid)
{
    if ((categoryUuid == mSelectedCategoryUuid) && (!categoryUuid.isNull())) return;

    setSelectedComponent(nullptr);
    mUi->listComponents->clear();
    //mUi->listComponents->setEnabled(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

    mSelectedCategoryUuid = categoryUuid;
    QSet<Uuid> components = mWorkspace.getLibraryDb().getComponentsByCategory(categoryUuid);
    foreach (const Uuid& cmpUuid, components)
    {
        FilePath cmpFp = mWorkspace.getLibraryDb().getLatestComponent(cmpUuid);
        if (!cmpFp.isValid()) continue;
        library::Component component(cmpFp, true); // TODO: use library metadata instead of loading the whole component

        QListWidgetItem* item = new QListWidgetItem(component.getNames().value(localeOrder));
        item->setData(Qt::UserRole, cmpFp.toStr());
        mUi->listComponents->addItem(item);
    }
}

void AddComponentDialog::setSelectedComponent(const library::Component* cmp)
{
    if (cmp == mSelectedComponent) return;

    mUi->lblCompUuid->setText(QString("00000000-0000-0000-0000-000000000000"));
    mUi->lblCompName->setText(QString("-"));
    mUi->lblCompDescription->setText(QString("-"));
    mUi->gbxComponent->setEnabled(false);
    mUi->gbxSymbVar->setEnabled(false);
    setSelectedSymbVar(nullptr);
    delete mSelectedComponent;
    mSelectedComponent = nullptr;

    if (cmp)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblCompUuid->setText(cmp->getUuid().toStr());
        mUi->lblCompName->setText(cmp->getNames().value(localeOrder));
        mUi->lblCompDescription->setText(cmp->getDescriptions().value(localeOrder));

        mUi->gbxComponent->setEnabled(true);
        mUi->gbxSymbVar->setEnabled(true);
        mSelectedComponent = cmp;

        mUi->cbxSymbVar->clear();
        for (int i = 0; i < cmp->getSymbolVariantCount(); i++) {
            const library::ComponentSymbolVariant* symbVar = cmp->getSymbolVariant(i);
            Q_ASSERT(symbVar); if (!symbVar) continue;

            mUi->cbxSymbVar->addItem(symbVar->getNames().value(localeOrder),
                                     symbVar->getUuid().toStr());
        }
        mUi->cbxSymbVar->setCurrentIndex(0);
    }
}

void AddComponentDialog::setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar)
{
    if (symbVar == mSelectedSymbVar) return;
    qDeleteAll(mPreviewSymbolGraphicsItems);
    mPreviewSymbolGraphicsItems.clear();
    mUi->lblSymbVarUuid->setText(QString("00000000-0000-0000-0000-000000000000"));
    mUi->lblSymbVarNorm->setText(QString("-"));
    mUi->lblSymbVarDescription->setText(QString("-"));
    mSelectedSymbVar = symbVar;

    if (mSelectedComponent && symbVar)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblSymbVarUuid->setText(symbVar->getUuid().toStr());
        mUi->lblSymbVarNorm->setText(symbVar->getNorm());
        mUi->lblSymbVarDescription->setText(symbVar->getDescriptions().value(localeOrder));

        for (int i = 0; i < symbVar->getItemCount(); i++) {
            const library::ComponentSymbolVariantItem* item = symbVar->getItem(i);
            Q_ASSERT(item); if (!item) continue;

            FilePath symbolFp = mWorkspace.getLibraryDb().getLatestSymbol(item->getSymbolUuid());
            if (!symbolFp.isValid()) continue; // TODO: show warning
            const library::Symbol* symbol = new library::Symbol(symbolFp, true); // TODO: fix memory leak...
            library::SymbolPreviewGraphicsItem* graphicsItem = new library::SymbolPreviewGraphicsItem(
                mProject, localeOrder, *symbol, mSelectedComponent, symbVar->getUuid(), item->getUuid());
            //graphicsItem->setDrawBoundingRect(mProject.getWorkspace().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect());
            mPreviewSymbolGraphicsItems.append(graphicsItem);
            Point pos = Point::fromPx(0, mPreviewScene->itemsBoundingRect().bottom()
                                      + graphicsItem->boundingRect().height(),
                                      mUi->graphicsView->getGridProperties().getInterval());
            graphicsItem->setPos(pos.toPxQPointF());
            mPreviewScene->addItem(*graphicsItem);
            mUi->graphicsView->zoomAll();
        }
    }
}

void AddComponentDialog::accept() noexcept
{
    if ((!mSelectedComponent) || (!mSelectedSymbVar))
    {
        QMessageBox::information(this, tr("Invalid Selection"),
            tr("Please select a component and a symbol variant."));
        return;
    }

    QDialog::accept();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
