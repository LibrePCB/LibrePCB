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
#include "newelementwizardpage_copyfrom.h"

#include "../../widgets/waitingspinnerwidget.h"
#include "../../workspace/categorytreemodel.h"
#include "ui_newelementwizardpage_copyfrom.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_CopyFrom::NewElementWizardPage_CopyFrom(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_CopyFrom),
    mIsCategoryElement(false),
    mIsComplete(false) {
  mUi->setupUi(this);
  connect(mUi->treeView, &QTreeView::doubleClicked, this,
          &NewElementWizardPage_CopyFrom::treeView_doubleClicked);
  connect(mUi->listWidget, &QListWidget::currentItemChanged, this,
          &NewElementWizardPage_CopyFrom::listWidget_currentItemChanged);
  connect(mUi->listWidget, &QListWidget::itemDoubleClicked, this,
          &NewElementWizardPage_CopyFrom::listWidget_itemDoubleClicked);

  // Add waiting spinner during workspace library scan.
  auto addSpinner = [&context](QWidget* widget) {
    WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget);
    connect(&context.getWorkspace().getLibraryDb(),
            &WorkspaceLibraryDb::scanStarted, spinner,
            &WaitingSpinnerWidget::show);
    connect(&context.getWorkspace().getLibraryDb(),
            &WorkspaceLibraryDb::scanFinished, spinner,
            &WaitingSpinnerWidget::hide);
    spinner->setVisible(
        context.getWorkspace().getLibraryDb().isScanInProgress());
  };
  addSpinner(mUi->treeView);
  addSpinner(mUi->listWidget);
}

NewElementWizardPage_CopyFrom::~NewElementWizardPage_CopyFrom() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_CopyFrom::validatePage() noexcept {
  if (!QWizardPage::validatePage()) return false;
  return mIsComplete;
}

bool NewElementWizardPage_CopyFrom::isComplete() const noexcept {
  return mIsComplete;
}

int NewElementWizardPage_CopyFrom::nextId() const noexcept {
  return NewElementWizardContext::ID_EnterMetadata;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_CopyFrom::treeView_currentItemChanged(
    const QModelIndex& current, const QModelIndex& previous) noexcept {
  Q_UNUSED(previous);
  setSelectedCategory(
      Uuid::tryFromString(current.data(Qt::UserRole).toString()));
}

void NewElementWizardPage_CopyFrom::treeView_doubleClicked(
    const QModelIndex& item) noexcept {
  setSelectedCategory(Uuid::tryFromString(item.data(Qt::UserRole).toString()));
  if (mIsCategoryElement) wizard()->next();
}

void NewElementWizardPage_CopyFrom::listWidget_currentItemChanged(
    QListWidgetItem* current, QListWidgetItem* previous) noexcept {
  Q_UNUSED(previous);
  if (mIsCategoryElement) return;
  if (current) {
    setSelectedElement(FilePath(current->data(Qt::UserRole).toString()));
  } else {
    setSelectedElement(FilePath());
  }
}

void NewElementWizardPage_CopyFrom::listWidget_itemDoubleClicked(
    QListWidgetItem* item) noexcept {
  if (mIsCategoryElement) return;
  if (item) {
    setSelectedElement(FilePath(item->data(Qt::UserRole).toString()));
    wizard()->next();
  }
}

void NewElementWizardPage_CopyFrom::setSelectedCategory(
    const tl::optional<Uuid>& uuid) noexcept {
  if (uuid && (uuid == mSelectedCategoryUuid)) return;

  setSelectedElement(FilePath());
  mUi->listWidget->clear();
  mSelectedCategoryUuid = uuid;

  try {
    if (mIsCategoryElement) {
      setSelectedElement(getCategoryFilePath(uuid));  // can throw
    } else {
      QSet<Uuid> elements = getElementsByCategory(uuid);  // can throw
      foreach (const Uuid& elementUuid, elements) {
        try {
          FilePath fp;
          QString name;
          getElementMetadata(elementUuid, fp, name);
          QListWidgetItem* item = new QListWidgetItem(name);
          item->setData(Qt::UserRole, fp.toStr());
          mUi->listWidget->addItem(item);
        } catch (const Exception& e) {
          continue;  // should we do something here?
        }
      }
    }
  } catch (const Exception& e) {
    // what could we do here?
  }
}

void NewElementWizardPage_CopyFrom::setSelectedElement(
    const FilePath& fp) noexcept {
  try {
    mContext.reset(mContext.mElementType);
    if (fp.isValid()) {
      mContext.copyElement(mContext.mElementType, fp);  // can throw
    }
    mIsComplete = true;
  } catch (const Exception& e) {
    mIsComplete = false;
  }

  emit completeChanged();
}

void NewElementWizardPage_CopyFrom::setCategoryTreeModel(
    QAbstractItemModel* model) noexcept {
  mUi->treeView->setModel(model);
  mUi->treeView->setCurrentIndex(QModelIndex());
  mUi->listWidget->clear();
  mCategoryTreeModel.reset(model);
  connect(mUi->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &NewElementWizardPage_CopyFrom::treeView_currentItemChanged);
}

FilePath NewElementWizardPage_CopyFrom::getCategoryFilePath(
    const tl::optional<Uuid>& category) const {
  if (category) {
    switch (mContext.mElementType) {
      case NewElementWizardContext::ElementType::ComponentCategory:
        return mContext.getWorkspace()
            .getLibraryDb()
            .getLatest<ComponentCategory>(*category);
      case NewElementWizardContext::ElementType::PackageCategory:
        return mContext.getWorkspace()
            .getLibraryDb()
            .getLatest<PackageCategory>(*category);
      default:
        throw LogicError(__FILE__, __LINE__);
    }
  } else {
    return FilePath();
  }
}

QSet<Uuid> NewElementWizardPage_CopyFrom::getElementsByCategory(
    const tl::optional<Uuid>& category) const {
  switch (mContext.mElementType) {
    case NewElementWizardContext::ElementType::Symbol:
      return mContext.getWorkspace().getLibraryDb().getByCategory<Symbol>(
          category);
    case NewElementWizardContext::ElementType::Component:
      return mContext.getWorkspace().getLibraryDb().getByCategory<Component>(
          category);
    case NewElementWizardContext::ElementType::Device:
      return mContext.getWorkspace().getLibraryDb().getByCategory<Device>(
          category);
    case NewElementWizardContext::ElementType::Package:
      return mContext.getWorkspace().getLibraryDb().getByCategory<Package>(
          category);
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

void NewElementWizardPage_CopyFrom::getElementMetadata(const Uuid& uuid,
                                                       FilePath& fp,
                                                       QString& name) const {
  switch (mContext.mElementType) {
    case NewElementWizardContext::ElementType::Symbol:
      fp = mContext.getWorkspace().getLibraryDb().getLatest<Symbol>(
          uuid);  // can throw
      mContext.getWorkspace().getLibraryDb().getTranslations<Symbol>(
          fp, mContext.getLibLocaleOrder(), &name);  // can throw
      return;
    case NewElementWizardContext::ElementType::Component:
      fp = mContext.getWorkspace().getLibraryDb().getLatest<Component>(
          uuid);  // can throw
      mContext.getWorkspace().getLibraryDb().getTranslations<Component>(
          fp, mContext.getLibLocaleOrder(), &name);  // can throw
      return;
    case NewElementWizardContext::ElementType::Device:
      fp = mContext.getWorkspace().getLibraryDb().getLatest<Device>(
          uuid);  // can throw
      mContext.getWorkspace().getLibraryDb().getTranslations<Device>(
          fp, mContext.getLibLocaleOrder(), &name);  // can throw
      return;
    case NewElementWizardContext::ElementType::Package:
      fp = mContext.getWorkspace().getLibraryDb().getLatest<Package>(
          uuid);  // can throw
      mContext.getWorkspace().getLibraryDb().getTranslations<Package>(
          fp, mContext.getLibLocaleOrder(), &name);  // can throw
      return;
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

void NewElementWizardPage_CopyFrom::initializePage() noexcept {
  QWizardPage::initializePage();
  setSelectedElement(FilePath());
  mIsCategoryElement = false;
  switch (mContext.mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory: {
      mIsCategoryElement = true;
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::CmpCat));
      break;
    }
    case NewElementWizardContext::ElementType::Symbol: {
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::CmpCatWithSymbols));
      break;
    }
    case NewElementWizardContext::ElementType::Component: {
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::CmpCatWithComponents));
      break;
    }
    case NewElementWizardContext::ElementType::Device: {
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::CmpCatWithDevices));
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory: {
      mIsCategoryElement = true;
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::PkgCat));
      break;
    }
    case NewElementWizardContext::ElementType::Package: {
      setCategoryTreeModel(new CategoryTreeModel(
          mContext.getWorkspace().getLibraryDb(), mContext.getLibLocaleOrder(),
          CategoryTreeModel::Filter::PkgCatWithPackages));
      break;
    }
    default: {
      qCritical() << "Unhandled switch-case in "
                     "NewElementWizardPage_CopyFrom::initializePage():"
                  << static_cast<int>(mContext.mElementType);
      setCategoryTreeModel(nullptr);
      break;
    }
  }
  mUi->treeView->setExpandsOnDoubleClick(!mIsCategoryElement);
  mUi->listWidget->setVisible(!mIsCategoryElement);
}

void NewElementWizardPage_CopyFrom::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
