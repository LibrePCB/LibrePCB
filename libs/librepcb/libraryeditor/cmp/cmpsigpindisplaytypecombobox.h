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

#ifndef LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPECOMBOBOX_H
#define LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPECOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/cmpsigpindisplaytype.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class CmpSigPinDisplayTypeComboBox
 ******************************************************************************/

/**
 * @brief The CmpSigPinDisplayTypeComboBox class
 */
class CmpSigPinDisplayTypeComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit CmpSigPinDisplayTypeComboBox(QWidget* parent = nullptr) noexcept;
  CmpSigPinDisplayTypeComboBox(const CmpSigPinDisplayTypeComboBox& other) =
      delete;
  ~CmpSigPinDisplayTypeComboBox() noexcept;

  // Getters
  CmpSigPinDisplayType getCurrentItem() const noexcept;

  // Setters
  void setCurrentItem(const CmpSigPinDisplayType& type) noexcept;

  // Operator Overloadings
  CmpSigPinDisplayTypeComboBox& operator=(
      const CmpSigPinDisplayTypeComboBox& rhs) = delete;

signals:
  void currentItemChanged(const CmpSigPinDisplayType& type);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_CMPSIGPINDISPLAYTYPECOMBOBOX_H
