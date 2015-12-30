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
#include "addcomponentdialog.h"
#include "ui_addcomponentdialog.h"
#include <librepcbcommon/graphics/graphicsscene.h>
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbproject/project.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/cmp/componentsymbolvariant.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcbproject/settings/projectsettings.h>
#include <librepcblibrary/sym/symbolpreviewgraphicsitem.h>
#include <librepcbworkspace/workspace.h>
#include <librepcbworkspace/settings/workspacesettings.h>
#include <librepcblibrary/cat/categorytreemodel.h>
#include <librepcbworkspace/workspace.h>
#include <librepcblibrary/cat/componentcategory.h>
#include <librepcblibrary/library.h>
#include <librepcbcommon/gridproperties.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AddComponentDialog::AddComponentDialog(Workspace& workspace, Project& project, QWidget* parent) :
    QDialog(parent), mWorkspace(workspace), mProject(project),
    mUi(new Ui::AddComponentDialog), mPreviewScene(nullptr), mCategoryTreeModel(nullptr),
    mSelectedComponent(nullptr), mSelectedSymbVar(nullptr)
{
    mUi->setupUi(this);
    mPreviewScene = new GraphicsScene();
    mUi->graphicsView->setScene(mPreviewScene);
    mUi->graphicsView->setOriginCrossVisible(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();
    mCategoryTreeModel = new library::CategoryTreeModel(mWorkspace.getLibrary(), localeOrder);
    mUi->treeCategories->setModel(mCategoryTreeModel);
    connect(mUi->treeCategories->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AddComponentDialog::treeCategories_currentItemChanged);

    //setSelectedCategory(Uuid());
}

AddComponentDialog::~AddComponentDialog() noexcept
{
    qDeleteAll(mPreviewSymbolGraphicsItems);    mPreviewSymbolGraphicsItems.clear();
    mSelectedSymbVar = nullptr;
    delete mSelectedComponent;                    mSelectedComponent = nullptr;
    delete mCategoryTreeModel;                  mCategoryTreeModel = nullptr;
    delete mPreviewScene;                       mPreviewScene = nullptr;
    delete mUi;                                 mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FilePath AddComponentDialog::getSelectedComponentFilePath() const noexcept
{
    if (mSelectedComponent)
        return mSelectedComponent->getDirectory();
    else
        return FilePath();
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
                FilePath(current->data(Qt::UserRole).toString())); // ugly...
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
    QSet<Uuid> components = mWorkspace.getLibrary().getComponentsByCategory(categoryUuid);
    foreach (const Uuid& cmpUuid, components)
    {
        FilePath cmpFp = mWorkspace.getLibrary().getLatestComponent(cmpUuid);
        if (!cmpFp.isValid()) continue;
        library::Component component(cmpFp); // TODO: use library metadata instead of loading the whole component

        QListWidgetItem* item = new QListWidgetItem(component.getName(localeOrder));
        item->setData(Qt::UserRole, cmpFp.toStr());
        mUi->listComponents->addItem(item);
    }
}

void AddComponentDialog::setSelectedComponent(const library::Component* cmp)
{
    if (cmp == mSelectedComponent) return;

    mUi->lblCompUuid->clear();
    mUi->lblCompName->clear();
    mUi->lblCompDescription->clear();
    mUi->gbxComponent->setEnabled(false);
    mUi->gbxSymbVar->setEnabled(false);
    setSelectedSymbVar(nullptr);
    delete mSelectedComponent;
    mSelectedComponent = nullptr;

    if (cmp)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblCompUuid->setText(cmp->getUuid().toStr());
        mUi->lblCompName->setText(cmp->getName(localeOrder));
        mUi->lblCompDescription->setText(cmp->getDescription(localeOrder));

        mUi->gbxComponent->setEnabled(true);
        mUi->gbxSymbVar->setEnabled(true);
        mSelectedComponent = cmp;

        mUi->cbxSymbVar->clear();
        for (int i = 0; i < cmp->getSymbolVariantCount(); i++) {
            const library::ComponentSymbolVariant* symbVar = cmp->getSymbolVariant(i);
            Q_ASSERT(symbVar); if (!symbVar) continue;

            QString text = symbVar->getName(localeOrder);
            if (symbVar == cmp->getDefaultSymbolVariant()) text.append(tr(" [default]"));
            mUi->cbxSymbVar->addItem(text, symbVar->getUuid().toStr());
        }
        mUi->cbxSymbVar->setCurrentIndex(mUi->cbxSymbVar->findData(cmp->getDefaultSymbolVariantUuid().toStr()));
    }
}

void AddComponentDialog::setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar)
{
    if (symbVar == mSelectedSymbVar) return;
    qDeleteAll(mPreviewSymbolGraphicsItems);
    mPreviewSymbolGraphicsItems.clear();
    mUi->lblSymbVarUuid->clear();
    mUi->lblSymbVarNorm->clear();
    mUi->lblSymbVarDescription->clear();
    mSelectedSymbVar = symbVar;

    if (mSelectedComponent && symbVar)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblSymbVarUuid->setText(symbVar->getUuid().toStr());
        mUi->lblSymbVarNorm->setText(symbVar->getNorm());
        mUi->lblSymbVarDescription->setText(symbVar->getDescription(localeOrder));

        for (int i = 0; i < symbVar->getItemCount(); i++) {
            const library::ComponentSymbolVariantItem* item = symbVar->getItem(i);
            Q_ASSERT(item); if (!item) continue;

            FilePath symbolFp = mWorkspace.getLibrary().getLatestSymbol(item->getSymbolUuid());
            if (!symbolFp.isValid()) continue; // TODO: show warning
            const library::Symbol* symbol = new library::Symbol(symbolFp); // TODO: fix memory leak...
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

} // namespace project
