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
#include "../project.h"
#include "../library/projectlibrary.h"
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcblibrary/gencmp/gencompsymbvar.h>
#include <librepcblibrary/sym/symbol.h>
#include "../settings/projectsettings.h"
#include <librepcblibrary/sym/symbolpreviewgraphicsitem.h>
#include <librepcbworkspace/workspace.h>
#include <librepcbworkspace/settings/workspacesettings.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AddGenCompDialog::AddGenCompDialog(Project& project, QWidget* parent) :
    QDialog(parent), mProject(project), mUi(new Ui::AddGenCompDialog),
    mPreviewScene(nullptr), mSelectedGenComp(nullptr), mSelectedSymbVar(nullptr)
{
    mUi->setupUi(this);
    mPreviewScene = new GraphicsScene();
    mUi->graphicsView->setScene(mPreviewScene);
    mUi->graphicsView->setOriginCrossVisible(false);

    const QStringList& localeOrder = mProject.getSettings().getLocaleOrder(true);

    // list generic components from project (TODO: for temporary use only...)
    foreach (const library::GenericComponent* genComp, mProject.getLibrary().getGenericComponents())
    {
        QListWidgetItem* item = new QListWidgetItem(genComp->getName(localeOrder));
        item->setData(Qt::UserRole, genComp->getXmlFilepath().toStr());
        mUi->listGenericComponents->addItem(item);
    }

    setSelectedGenComp(nullptr);
}

AddGenCompDialog::~AddGenCompDialog()
{
    qDeleteAll(mPreviewSymbolGraphicsItems);    mPreviewSymbolGraphicsItems.clear();
    mSelectedSymbVar = nullptr;
    delete mSelectedGenComp;                    mSelectedGenComp = nullptr;
    delete mPreviewScene;                       mPreviewScene = nullptr;
    delete mUi;                                 mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FilePath AddGenCompDialog::getSelectedGenCompFilePath() const noexcept
{
    if (mSelectedGenComp)
        return mSelectedGenComp->getXmlFilepath();
    else
        return FilePath();
}

QUuid AddGenCompDialog::getSelectedSymbVarUuid() const noexcept
{
    if (mSelectedGenComp && mSelectedSymbVar)
        return mSelectedSymbVar->getUuid();
    else
        return QUuid();
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void AddGenCompDialog::on_listGenericComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    try
    {
        if (current)
        {
            library::GenericComponent* genComp = new library::GenericComponent(
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
        setSelectedSymbVar(mSelectedGenComp->getSymbolVariantByUuid(mUi->cbxSymbVar->itemData(index).toUuid()));
    else
        setSelectedSymbVar(nullptr);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AddGenCompDialog::setSelectedGenComp(const library::GenericComponent* genComp)
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
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder(true);

        mUi->lblGenCompUuid->setText(genComp->getUuid().toString());
        mUi->lblGenCompName->setText(genComp->getName(localeOrder));
        mUi->lblGenCompDescription->setText(genComp->getDescription(localeOrder));

        mUi->gbxGenComp->setEnabled(true);
        mUi->gbxSymbVar->setEnabled(true);
        mSelectedGenComp = genComp;

        mUi->cbxSymbVar->clear();
        foreach (const library::GenCompSymbVar* symbVar, genComp->getSymbolVariants())
        {
            QString text = symbVar->getName(localeOrder);
            if (symbVar->isDefault()) text.append(tr(" [default]"));
            mUi->cbxSymbVar->addItem(text, symbVar->getUuid());
        }
        mUi->cbxSymbVar->setCurrentIndex(mUi->cbxSymbVar->findData(genComp->getDefaultSymbolVariantUuid()));
    }
}

void AddGenCompDialog::setSelectedSymbVar(const library::GenCompSymbVar* symbVar)
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
        const QStringList& localeOrder = mProject.getSettings().getLocaleOrder(true);

        mUi->lblSymbVarUuid->setText(symbVar->getUuid().toString());
        mUi->lblSymbVarNorm->setText(symbVar->getNorm());
        mUi->lblSymbVarDescription->setText(symbVar->getDescription(localeOrder));

        foreach (const library::GenCompSymbVarItem* item, symbVar->getItems())
        {
            const library::Symbol* symbol = mProject.getLibrary().getSymbol(item->getSymbolUuid());
            if (!symbol) continue; // TODO: show warning
            library::SymbolPreviewGraphicsItem* graphicsItem = new library::SymbolPreviewGraphicsItem(
                mProject, localeOrder, *symbol, mSelectedGenComp, symbVar->getUuid(), item->getUuid());
            graphicsItem->setDrawBoundingRect(Workspace::instance().getSettings().getDebugTools()->getShowGraphicsItemsBoundingRect());
            mPreviewSymbolGraphicsItems.append(graphicsItem);
            qreal y = mPreviewScene->itemsBoundingRect().bottom() + graphicsItem->boundingRect().height();
            graphicsItem->setPos(QPointF(0, y));
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
