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

#ifndef LIBREPCB_EDITOR_ATTRIBUTEUNITCOMBOBOX_H
#define LIBREPCB_EDITOR_ATTRIBUTEUNITCOMBOBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeType;
class AttributeUnit;

namespace editor {

/*******************************************************************************
 *  Class AttributeUnitComboBox
 ******************************************************************************/

/**
 * @brief The AttributeUnitComboBox class
 */
class AttributeUnitComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AttributeUnitComboBox(QWidget* parent = nullptr) noexcept;
  AttributeUnitComboBox(const AttributeUnitComboBox& other) = delete;
  ~AttributeUnitComboBox() noexcept;

  // Getters
  const AttributeUnit* getCurrentItem() const noexcept;

  // Setters
  void setAttributeType(const AttributeType& type) noexcept;
  void setCurrentItem(const AttributeUnit* unit) noexcept;

  // Operator Overloadings
  AttributeUnitComboBox& operator=(const AttributeUnitComboBox& rhs) = delete;

signals:
  void currentItemChanged(const AttributeUnit* unit);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
  const AttributeType* mAttributeType;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
