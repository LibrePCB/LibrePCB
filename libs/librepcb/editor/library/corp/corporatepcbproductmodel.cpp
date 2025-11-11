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
#include "corporatepcbproductmodel.h"

#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdcorporateedit.h"

#include <librepcb/core/library/corp/corporate.h>

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

CorporatePcbProductModel::CorporatePcbProductModel(QObject* parent) noexcept
  : QObject(parent) {
}

CorporatePcbProductModel::~CorporatePcbProductModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CorporatePcbProductModel::setReferences(
    Corporate* corporate, UndoStack* stack,
    std::function<void(CorporatePcbProduct&)> editCallback) noexcept {
  if ((corporate == mCorporate) && (stack == mUndoStack)) {
    return;
  }

  if (mCorporate) {
    disconnect(mCorporate, &Corporate::pcbProductsModified, this,
               &CorporatePcbProductModel::refresh);
  }

  mCorporate = corporate;
  mUndoStack = stack;
  mEditCallback = editCallback;

  if (mCorporate) {
    connect(mCorporate.get(), &Corporate::pcbProductsModified, this,
            &CorporatePcbProductModel::refresh);
  }

  notify_reset();
}

void CorporatePcbProductModel::addProduct() noexcept {
  if ((!mCorporate) || (!mUndoStack)) return;

  try {
    const QString name = askForName(QString());
    if (name.isEmpty()) return;

    auto list = mCorporate->getPcbProducts();
    list.append(CorporatePcbProduct(
        Uuid::createRandom(), ElementName(cleanElementName(name)), QString(),
        QUrl(), BoardDesignRuleCheckSettings()));
    setList(list);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t CorporatePcbProductModel::row_count() const {
  return mCorporate ? mCorporate->getPcbProducts().count() : 9;
}

std::optional<ui::CorporatePcbProductData> CorporatePcbProductModel::row_data(
    std::size_t i) const {
  if ((!mCorporate) ||
      (i >= static_cast<std::size_t>(mCorporate->getPcbProducts().count()))) {
    return std::nullopt;
  }

  return ui::CorporatePcbProductData{
      q2s(*mCorporate->getPcbProducts()
               .at(i)
               .getNames()
               .getDefaultValue()),  // Name
      ui::CorporatePcbProductAction::None,  // Action
  };
}

void CorporatePcbProductModel::set_row_data(
    std::size_t i, const ui::CorporatePcbProductData& data) noexcept {
  if ((!mCorporate) ||
      (i >= static_cast<std::size_t>(mCorporate->getPcbProducts().count()))) {
    return;
  }

  if (data.action != ui::CorporatePcbProductAction::None) {
    const auto& obj = mCorporate->getPcbProducts().at(i);
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this,
        [this, i, uuid = obj.getUuid(), a = data.action]() {
          trigger(i, uuid, a);
        },
        Qt::QueuedConnection);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CorporatePcbProductModel::refresh() noexcept {
  notify_reset();
}

void CorporatePcbProductModel::trigger(
    int index, const Uuid& uuid, ui::CorporatePcbProductAction a) noexcept {
  if ((!mCorporate) || (!mUndoStack) ||
      (index >= mCorporate->getPcbProducts().count()) ||
      (mCorporate->getPcbProducts().at(index).getUuid() != uuid)) {
    return;
  }

  try {
    auto list = mCorporate->getPcbProducts();
    if (a == ui::CorporatePcbProductAction::Edit) {
      if (mEditCallback) {
        mEditCallback(list[index]);  // can throw
      }
    } else if (a == ui::CorporatePcbProductAction::Rename) {
      auto names = list[index].getNames();
      const QString name = askForName(*names.getDefaultValue());
      if (name.isEmpty()) return;
      names.setDefaultValue(ElementName(cleanElementName(name)));
      list[index].setNames(names);
    } else if (a == ui::CorporatePcbProductAction::Duplicate) {
      auto copy = list.at(index);
      copy.setUuid(Uuid::createRandom());
      list.append(list.at(index));
    } else if (a == ui::CorporatePcbProductAction::Delete) {
      list.removeAt(index);
    }
    setList(list);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

void CorporatePcbProductModel::setList(
    const QVector<CorporatePcbProduct>& list) {
  std::unique_ptr<CmdCorporateEdit> cmd(new CmdCorporateEdit(*mCorporate));
  cmd->setPcbProducts(list);
  mUndoStack->execCmd(cmd.release());
}

QString CorporatePcbProductModel::askForName(
    const QString& defaultValue) const {
  return QInputDialog::getText(qApp->activeWindow(), tr("PCB Product Name"),
                               tr("New name of the PCB product:"),
                               QLineEdit::EchoMode::Normal, defaultValue);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
