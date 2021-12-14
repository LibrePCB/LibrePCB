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

#ifndef LIBREPCB_COMMON_ATTRIBUTETYPECOMBOBOX_H
#define LIBREPCB_COMMON_ATTRIBUTETYPECOMBOBOX_H

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

/*******************************************************************************
 *  Class AttributeTypeComboBox
 ******************************************************************************/

/**
 * @brief The AttributeTypeComboBox class
 */
class AttributeTypeComboBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AttributeTypeComboBox(QWidget* parent = nullptr) noexcept;
  AttributeTypeComboBox(const AttributeTypeComboBox& other) = delete;
  ~AttributeTypeComboBox() noexcept;

  // Getters
  const AttributeType& getCurrentItem() const noexcept;

  // Setters
  void setCurrentItem(const AttributeType& type) noexcept;

  // Operator Overloadings
  AttributeTypeComboBox& operator=(const AttributeTypeComboBox& rhs) = delete;

signals:
  void currentItemChanged(const AttributeType* type);

private:  // Methods
  void currentIndexChanged(int index) noexcept;

private:  // Data
  QComboBox* mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
