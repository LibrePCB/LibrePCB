/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "componentchooserdialog.h"

#include "../../graphics/graphicsscene.h"
#include "../../widgets/waitingspinnerwidget.h"
#include "../../workspace/categorytreemodel.h"
#include "../sym/symbolgraphicsitem.h"
#include "ui_componentchooserdialog.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ComponentChooserDialog::ComponentChooserDialog(const Workspace& ws,
                                               const GraphicsLayerList* layers,
                                               QWidget* parent) noexcept
  : QDialog(parent),
    mWorkspace(ws),
    mLayers(layers),
    mUi(new Ui::ComponentChooserDialog),
    mCategorySelected(false),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);

  const Theme& theme = mWorkspace.getSettings().themes.getActive();
  mGraphicsScene->setBackgroundColors(
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mGraphicsScene->setOriginCrossVisible(false);

  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
  mUi->graphicsView->setScene(mGraphicsScene.data());

  mCategoryTreeModel.reset(
      new CategoryTreeModel(mWorkspace.getLibraryDb(), localeOrder(),
                            CategoryTreeModel::Filter::CmpCatWithComponents));
  mUi->treeCategories->setModel(mCategoryTreeModel.data());
  connect(mUi->treeCategories->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &ComponentChooserDialog::treeCategories_currentItemChanged);
  connect(mUi->listComponents, &QListWidget::currentItemChanged, this,
          &ComponentChooserDialog::listComponents_currentItemChanged);
  connect(mUi->listComponents, &QListWidget::itemDoubleClicked, this,
          &ComponentChooserDialog::listComponents_itemDoubleClicked);
  connect(mUi->edtSearch, &QLineEdit::textChanged, this,
          &ComponentChooserDialog::searchEditTextChanged);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [&ws](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&ws.getLibraryDb(), &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(ws.getLibraryDb().isScanInProgress());
  };
  addSpinner(mUi->treeCategories);
  addSpinner(mUi->listComponents);

  setSelectedComponent(std::nullopt);
}

ComponentChooserDialog::~ComponentChooserDialog() noexcept {
  setSelectedComponent(std::nullopt);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ComponentChooserDialog::searchEditTextChanged(
    const QString& text) noexcept {
  try {
    QModelIndex catIndex = mUi->treeCategories->currentIndex();
    if (text.trimmed().isEmpty() && catIndex.isValid()) {
      setSelectedCategory(
          Uuid::tryFromString(catIndex.data(Qt::UserRole).toString()));
    } else {
      searchComponents(text.trimmed());
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void ComponentChooserDialog::treeCategories_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);
  setSelectedCategory(
      Uuid::tryFromString(current.data(Qt::UserRole).toString()));
}

void ComponentChooserDialog::listComponents_currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  if (current) {
    setSelectedComponent(
        Uuid::tryFromString(current->data(Qt::UserRole).toString()));
  } else {
    setSelectedComponent(std::nullopt);
  }
}

void ComponentChooserDialog::listComponents_itemDoubleClicked(
    QListWidgetItem* item) noexcept {
  if (item) {
    setSelectedComponent(
        Uuid::tryFromString(item->data(Qt::UserRole).toString()));
    accept();
  }
}

void ComponentChooserDialog::searchComponents(const QString& input) {
  setSelectedComponent(std::nullopt);
  mUi->listComponents->clear();
  mCategorySelected = false;

  // min. 2 chars to avoid freeze on entering first character due to huge result
  if (input.length() > 1) {
    QList<Uuid> components = mWorkspace.getLibraryDb().find<Component>(input);
    foreach (const Uuid& uuid, components) {
      FilePath fp =
          mWorkspace.getLibraryDb().getLatest<Component>(uuid);  // can throw
      QString name;
      mWorkspace.getLibraryDb().getTranslations<Component>(fp, localeOrder(),
                                                           &name);  // can throw
      bool deprecated = false;
      mWorkspace.getLibraryDb().getMetadata<Component>(
          fp, nullptr, nullptr, &deprecated);  // can throw
      QListWidgetItem* item = new QListWidgetItem(name);
      item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
      item->setData(Qt::UserRole, uuid.toStr());
      mUi->listComponents->addItem(item);
    }
  }
}

void ComponentChooserDialog::setSelectedCategory(
    const std::optional<Uuid>& uuid) noexcept {
  if ((mCategorySelected) && (uuid == mSelectedCategoryUuid)) return;

  setSelectedComponent(std::nullopt);
  mUi->listComponents->clear();
  mSelectedCategoryUuid = uuid;
  mCategorySelected = true;

  try {
    QSet<Uuid> components =
        mWorkspace.getLibraryDb().getByCategory<Component>(uuid);  // can throw
    foreach (const Uuid& cmpUuid, components) {
      try {
        FilePath fp = mWorkspace.getLibraryDb().getLatest<Component>(
            cmpUuid);  // can throw
        QString name;
        mWorkspace.getLibraryDb().getTranslations<Component>(
            fp, localeOrder(),
            &name);  // can throw
        bool deprecated = false;
        mWorkspace.getLibraryDb().getMetadata<Component>(
            fp, nullptr, nullptr, &deprecated);  // can throw
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setForeground(deprecated ? QBrush(Qt::red) : QBrush());
        item->setData(Qt::UserRole, cmpUuid.toStr());
        mUi->listComponents->addItem(item);
      } catch (const Exception& e) {
        continue;  // should we do something here?
      }
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not load components"), e.getMsg());
  }
}

void ComponentChooserDialog::setSelectedComponent(
    const std::optional<Uuid>& uuid) noexcept {
  FilePath fp;
  QString name = tr("No component selected");
  QString desc;
  mSelectedComponentUuid = uuid;

  if (uuid) {
    try {
      fp = mWorkspace.getLibraryDb().getLatest<Component>(*uuid);  // can throw
      mWorkspace.getLibraryDb().getTranslations<Component>(
          fp, localeOrder(), &name, &desc);  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Could not load component metadata"),
                            e.getMsg());
    }
  }

  mUi->lblComponentName->setText(name);
  mUi->lblComponentDescription->setText(desc);
  updatePreview(fp);
}

void ComponentChooserDialog::updatePreview(const FilePath& fp) noexcept {
  mSymbolGraphicsItems.clear();
  mSymbols.clear();
  mComponent.reset();

  if (fp.isValid() && mLayers) {
    try {
      mComponent.reset(
          Component::open(std::unique_ptr<TransactionalDirectory>(
                              new TransactionalDirectory(
                                  TransactionalFileSystem::openRO(fp))))
              .release());  // can throw
      if (mComponent && mComponent->getSymbolVariants().count() > 0) {
        const ComponentSymbolVariant& symbVar =
            *mComponent->getSymbolVariants().first();
        for (const ComponentSymbolVariantItem& item :
             symbVar.getSymbolItems()) {
          try {
            FilePath fp = mWorkspace.getLibraryDb().getLatest<Symbol>(
                item.getSymbolUuid());  // can throw
            std::shared_ptr<Symbol> sym(
                Symbol::open(std::unique_ptr<TransactionalDirectory>(
                                 new TransactionalDirectory(
                                     TransactionalFileSystem::openRO(fp))))
                    .release());  // can throw
            mSymbols.append(sym);

            std::shared_ptr<SymbolGraphicsItem> graphicsItem =
                std::make_shared<SymbolGraphicsItem>(
                    *sym, *mLayers, mComponent,
                    symbVar.getSymbolItems().get(item.getUuid()),
                    localeOrder());
            graphicsItem->setPosition(item.getSymbolPosition());
            graphicsItem->setRotation(item.getSymbolRotation());
            mGraphicsScene->addItem(*graphicsItem);
            mSymbolGraphicsItems.append(graphicsItem);
          } catch (const Exception& e) {
            // what could we do here? ;)
          }
        }
        mUi->graphicsView->zoomAll();
      }
    } catch (const Exception& e) {
      // ignore errors...
    }
  }
}

void ComponentChooserDialog::accept() noexcept {
  if (!mSelectedComponentUuid) {
    QMessageBox::information(this, tr("Invalid Selection"),
                             tr("Please select a component."));
    return;
  }
  QDialog::accept();
}

const QStringList& ComponentChooserDialog::localeOrder() const noexcept {
  return mWorkspace.getSettings().libraryLocaleOrder.get();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
