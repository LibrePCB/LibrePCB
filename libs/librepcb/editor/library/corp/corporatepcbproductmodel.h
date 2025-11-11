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

#ifndef LIBREPCB_EDITOR_CORPORATEPCBPRODUCTMODEL_H
#define LIBREPCB_EDITOR_CORPORATEPCBPRODUCTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/corp/corporatepcbproduct.h>

#include <QtCore>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Corporate;

namespace editor {

class UndoStack;

/*******************************************************************************
 *  Class CorporatePcbProductModel
 ******************************************************************************/

/**
 * @brief The CorporatePcbProductModel class
 */
class CorporatePcbProductModel final
  : public QObject,
    public slint::Model<ui::CorporatePcbProductData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // CorporatePcbProductModel() = delete;
  CorporatePcbProductModel(const CorporatePcbProductModel& other) = delete;
  explicit CorporatePcbProductModel(QObject* parent = nullptr) noexcept;
  ~CorporatePcbProductModel() noexcept;

  // General Methods
  void setReferences(
      Corporate* corporate, UndoStack* stack,
      std::function<void(CorporatePcbProduct&)> editCallback) noexcept;
  void addProduct() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::CorporatePcbProductData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::CorporatePcbProductData& data) noexcept override;

  // Operator Overloadings
  CorporatePcbProductModel& operator=(const CorporatePcbProductModel& rhs) =
      delete;

private:
  void refresh() noexcept;
  void trigger(int index, const Uuid& uuid,
               ui::CorporatePcbProductAction a) noexcept;
  void setList(const QVector<CorporatePcbProduct>& list);
  QString askForName(const QString& defaultValue) const;

private:
  QPointer<Corporate> mCorporate;
  QPointer<UndoStack> mUndoStack;
  std::function<void(CorporatePcbProduct&)> mEditCallback;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
