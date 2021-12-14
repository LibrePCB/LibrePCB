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

#ifndef LIBREPCB_EDITOR_PACKAGEPADCOMBOBOX_H
#define LIBREPCB_EDITOR_PACKAGEPADCOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/package.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PackagePadComboBox
 ******************************************************************************/

/**
 * @brief The PackagePadComboBox class
 */
class PackagePadComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PackagePadComboBox(QWidget* parent = nullptr) noexcept;
  PackagePadComboBox(const PackagePadComboBox& other) = delete;
  ~PackagePadComboBox() noexcept;

  // Getters
  tl::optional<Uuid> getCurrentPad() const noexcept;

  // Setters
  void setPads(const PackagePadList& pads) noexcept;
  void setCurrentPad(tl::optional<Uuid> pad) noexcept;

  // Operator Overloadings
  PackagePadComboBox& operator=(const PackagePadComboBox& rhs) = delete;

signals:
  void currentPadChanged(tl::optional<Uuid> pad);

private:  // Methods
  tl::optional<Uuid> getPadAtIndex(int index) const noexcept;
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
