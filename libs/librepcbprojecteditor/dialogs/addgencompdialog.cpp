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
#include "addgencompdialog.h"
#include "ui_addgencompdialog.h"
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

AddGenCompDialog::AddGenCompDialog(Workspace& workspace, Project& project, QWidget* parent) :
    QDialog(parent), mWorkspace(workspace), mProject(project),
    mUi(new Ui::AddGenCompDialog), mPreviewScene(nullptr), mCategoryTreeModel(nullptr),
    mSelectedGenComp(nullptr), mSelectedSymbVar(nullptr)
{
    mUi->setupUi(this);
    mPreviewScene = new GraphicsScene();
    mUi->graphicsView->setScene(mPreviewScene);
    mUi->graphicsView->setOriginCrossVisible(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();
    mCategoryTreeModel = new library::CategoryTreeModel(mWorkspace.getLibrary(), localeOrder);
    mUi->treeCategories->setModel(mCategoryTreeModel);
    connect(mUi->treeCategories->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AddGenCompDialog::treeCategories_currentItemChanged);

    //setSelectedCategory(Uuid());
}

AddGenCompDialog::~AddGenCompDialog() noexcept
{
    qDeleteAll(mPreviewSymbolGraphicsItems);    mPreviewSymbolGraphicsItems.clear();
    mSelectedSymbVar = nullptr;
    delete mSelectedGenComp;                    mSelectedGenComp = nullptr;
    delete mCategoryTreeModel;                  mCategoryTreeModel = nullptr;
    delete mPreviewScene;                       mPreviewScene = nullptr;
    delete mUi;                                 mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FilePath AddGenCompDialog::getSelectedGenCompFilePath() const noexcept
{
    if (mSelectedGenComp)
        return mSelectedGenComp->getDirectory();
    else
        return FilePath();
}

Uuid AddGenCompDialog::getSelectedSymbVarUuid() const noexcept
{
    if (mSelectedGenComp && mSelectedSymbVar)
        return mSelectedSymbVar->getUuid();
    else
        return Uuid();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void AddGenCompDialog::treeCategories_currentItemChanged(const QModelIndex& current, const QModelIndex& previous)
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

void AddGenCompDialog::on_listGenericComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    try
    {
        if (current)
        {
            library::Component* genComp = new library::Component(
                FilePath(current->data(Qt::UserRole).toString())); // ugly...
            setSelectedGenComp(genComp);
        }
        else
        {
            setSelectedGenComp(nullptr);
        }
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        setSelectedGenComp(nullptr);
    }
}

void AddGenCompDialog::on_cbxSymbVar_currentIndexChanged(int index)
{
    if ((mSelectedGenComp) && (index >= 0))
        setSelectedSymbVar(mSelectedGenComp->getSymbolVariantByUuid(Uuid(mUi->cbxSymbVar->itemData(index).toString())));
    else
        setSelectedSymbVar(nullptr);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AddGenCompDialog::setSelectedCategory(const Uuid& categoryUuid)
{
    if ((categoryUuid == mSelectedCategoryUuid) && (!categoryUuid.isNull())) return;

    setSelectedGenComp(nullptr);
    mUi->listGenericComponents->clear();
    //mUi->listGenericComponents->setEnabled(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

    mSelectedCategoryUuid = categoryUuid;
    QSet<Uuid> genComps = mWorkspace.getLibrary().getComponentsByCategory(categoryUuid);
    foreach (const Uuid& genCompUuid, genComps)
    {
        FilePath genCompFp = mWorkspace.getLibrary().getLatestComponent(genCompUuid);
        if (!genCompFp.isValid()) continue;
        library::Component genComp(genCompFp); // TODO: use library metadata instead of loading the whole component

        QListWidgetItem* item = new QListWidgetItem(genComp.getName(localeOrder));
        item->setData(Qt::UserRole, genCompFp.toStr());
        mUi->listGenericComponents->addItem(item);
    }
}

void AddGenCompDialog::setSelectedGenComp(const library::Component* genComp)
{
    if (genComp == mSelectedGenComp) return;

    mUi->lblGenCompUuid->clear();
    mUi->lblGenCompName->clear();
    mUi->lblGenCompDescription->clear();
    mUi->gbxGenComp->setEnabled(false);
    mUi->gbxSymbVar->setEnabled(false);
    setSelectedSymbVar(nullptr);
    delete mSelectedGenComp;
    mSelectedGenComp = nullptr;

    if (genComp)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblGenCompUuid->setText(genComp->getUuid().toStr());
        mUi->lblGenCompName->setText(genComp->getName(localeOrder));
        mUi->lblGenCompDescription->setText(genComp->getDescription(localeOrder));

        mUi->gbxGenComp->setEnabled(true);
        mUi->gbxSymbVar->setEnabled(true);
        mSelectedGenComp = genComp;

        mUi->cbxSymbVar->clear();
        foreach (const library::ComponentSymbolVariant* symbVar, genComp->getSymbolVariants())
        {
            QString text = symbVar->getName(localeOrder);
            if (symbVar->isDefault()) text.append(tr(" [default]"));
            mUi->cbxSymbVar->addItem(text, symbVar->getUuid().toStr());
        }
        mUi->cbxSymbVar->setCurrentIndex(mUi->cbxSymbVar->findData(genComp->getDefaultSymbolVariantUuid().toStr()));
    }
}

void AddGenCompDialog::setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar)
{
    if (symbVar == mSelectedSymbVar) return;
    qDeleteAll(mPreviewSymbolGraphicsItems);
    mPreviewSymbolGraphicsItems.clear();
    mUi->lblSymbVarUuid->clear();
    mUi->lblSymbVarNorm->clear();
    mUi->lblSymbVarDescription->clear();
    mSelectedSymbVar = symbVar;

    if (mSelectedGenComp && symbVar)
    {
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder();

        mUi->lblSymbVarUuid->setText(symbVar->getUuid().toStr());
        mUi->lblSymbVarNorm->setText(symbVar->getNorm());
        mUi->lblSymbVarDescription->setText(symbVar->getDescription(localeOrder));

        foreach (const library::ComponentSymbolVariantItem* item, symbVar->getItems())
        {
            FilePath symbolFp = mWorkspace.getLibrary().getLatestSymbol(item->getSymbolUuid());
            if (!symbolFp.isValid()) continue; // TODO: show warning
            const library::Symbol* symbol = new library::Symbol(symbolFp); // TODO: fix memory leak...
            library::SymbolPreviewGraphicsItem* graphicsItem = new library::SymbolPreviewGraphicsItem(
                mProject, localeOrder, *symbol, mSelectedGenComp, symbVar->getUuid(), item->getUuid());
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

void AddGenCompDialog::accept() noexcept
{
    if ((!mSelectedGenComp) || (!mSelectedSymbVar))
    {
        QMessageBox::information(this, tr("Invalid Selection"),
            tr("Please select a generic component and a symbol variant."));
        return;
    }

    QDialog::accept();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
