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

#ifndef LIBREPCB_EDITOR_PARTEDITOR_H
#define LIBREPCB_EDITOR_PARTEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Part;

namespace editor {

class AttributeListModel;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class PartEditor
 ******************************************************************************/

/**
 * @brief The PartEditor class
 */
class PartEditor final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  PartEditor() = delete;
  PartEditor(const PartEditor& other) = delete;
  explicit PartEditor(std::shared_ptr<Part> part, UndoStack* stack,
                      QObject* parent = nullptr) noexcept;
  ~PartEditor() noexcept;

  // General Methods
  ui::PartData getUiData() const;
  void setUiData(const ui::PartData& data, bool allowEmpty) noexcept;
  void apply();

  // Operator Overloadings
  PartEditor& operator=(const PartEditor& rhs) = delete;

private:
  void execCmd(UndoCommand* cmd);

private:
  std::shared_ptr<Part> mPart;
  QPointer<UndoStack> mUndoStack;

  std::shared_ptr<AttributeListModel> mAttributes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
