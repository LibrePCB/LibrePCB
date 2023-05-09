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

#ifndef LIBREPCB_EDITOR_FOOTPRINTLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_FOOTPRINTLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/footprint.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;
class Package;

namespace editor {

class EditableTableWidget;
class FootprintListModel;
class LengthDelegate;
class UndoStack;

/*******************************************************************************
 *  Class FootprintListEditorWidget
 ******************************************************************************/

/**
 * @brief The FootprintListEditorWidget class
 */
class FootprintListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit FootprintListEditorWidget(QWidget* parent = nullptr) noexcept;
  FootprintListEditorWidget(const FootprintListEditorWidget& other) = delete;
  ~FootprintListEditorWidget() noexcept;

  // Setters
  void setFrameStyle(int style) noexcept;
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(Package* package, UndoStack* stack) noexcept;
  void setLengthUnit(const LengthUnit& unit) noexcept;

  // Operator Overloadings
  FootprintListEditorWidget& operator=(const FootprintListEditorWidget& rhs) =
      delete;

signals:
  void currentFootprintChanged(int index);

private:
  QScopedPointer<FootprintListModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
  LengthDelegate* mLengthDelegateX;
  LengthDelegate* mLengthDelegateY;
  LengthDelegate* mLengthDelegateZ;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
