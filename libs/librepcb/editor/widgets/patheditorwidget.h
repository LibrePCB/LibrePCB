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

#ifndef LIBREPCB_EDITOR_PATHEDITORWIDGET_H
#define LIBREPCB_EDITOR_PATHEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/path.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;

namespace editor {

class EditableTableWidget;
class LengthDelegate;
class PathModel;

/*******************************************************************************
 *  Class PathEditorWidget
 ******************************************************************************/

/**
 * @brief The PathEditorWidget class
 */
class PathEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PathEditorWidget(QWidget* parent = nullptr) noexcept;
  PathEditorWidget(const PathEditorWidget& other) = delete;
  ~PathEditorWidget() noexcept;

  // General Methods
  void setFrameShape(QFrame::Shape shape) noexcept;
  void setReadOnly(bool readOnly) noexcept;
  void setMinimumVertexCount(int count) noexcept;
  void setPath(const Path& path) noexcept;
  const Path& getPath() const noexcept;
  void setLengthUnit(const LengthUnit& unit) noexcept;

  // Operator Overloadings
  PathEditorWidget& operator=(const PathEditorWidget& rhs) = delete;

signals:
  void pathChanged(const Path& path);

private:  // Data
  QScopedPointer<PathModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
  LengthDelegate* mLengthDelegateX;
  LengthDelegate* mLengthDelegateY;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
