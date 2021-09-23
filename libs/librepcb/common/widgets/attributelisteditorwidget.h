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

#ifndef LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H
#define LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../attributes/attribute.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class EditableTableWidget;
class AttributeListModel;

/*******************************************************************************
 *  Class AttributeListEditorWidget
 ******************************************************************************/

/**
 * @brief The AttributeListEditorWidget class
 */
class AttributeListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AttributeListEditorWidget(QWidget* parent = nullptr) noexcept;
  AttributeListEditorWidget(const AttributeListEditorWidget& other) = delete;
  ~AttributeListEditorWidget() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(UndoStack* undoStack, AttributeList* list) noexcept;

  // Operator Overloadings
  AttributeListEditorWidget& operator=(const AttributeListEditorWidget& rhs) =
      delete;

private:  // Data
  QScopedPointer<AttributeListModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H
