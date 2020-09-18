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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDPICKPLACEGENERATORDIALOG_H
#define LIBREPCB_PROJECT_EDITOR_BOARDPICKPLACEGENERATORDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PickPlaceData;

namespace project {

class Board;

namespace editor {

namespace Ui {
class BoardPickPlaceGeneratorDialog;
}

/*******************************************************************************
 *  Class BoardPickPlaceGeneratorDialog
 ******************************************************************************/

/**
 * @brief The BoardPickPlaceGeneratorDialog class
 */
class BoardPickPlaceGeneratorDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardPickPlaceGeneratorDialog() = delete;
  BoardPickPlaceGeneratorDialog(const BoardPickPlaceGeneratorDialog& other) =
      delete;
  explicit BoardPickPlaceGeneratorDialog(Board& board,
                                         QWidget* parent = nullptr);
  ~BoardPickPlaceGeneratorDialog();

private:  // Methods
  void btnGenerateClicked() noexcept;
  void updateTable() noexcept;
  FilePath getOutputFilePath(const QString& text) const noexcept;

private:  // Data
  Board& mBoard;
  std::shared_ptr<PickPlaceData> mData;
  QScopedPointer<Ui::BoardPickPlaceGeneratorDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_EDITOR_BOARDPICKPLACEGENERATORDIALOG_H
