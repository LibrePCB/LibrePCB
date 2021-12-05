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

#ifndef LIBREPCB_PROJECTEDITOR_FABRICATIONOUTPUTDIALOG_H
#define LIBREPCB_PROJECTEDITOR_FABRICATIONOUTPUTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Board;
class Project;

namespace editor {

namespace Ui {
class FabricationOutputDialog;
}

/*******************************************************************************
 *  Class FabricationOutputDialog
 ******************************************************************************/

/**
 * @brief The FabricationOutputDialog class
 */
class FabricationOutputDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  FabricationOutputDialog() = delete;
  FabricationOutputDialog(const FabricationOutputDialog& other) = delete;
  explicit FabricationOutputDialog(Board& board, QWidget* parent = 0);
  ~FabricationOutputDialog();

private slots:
  void on_btnDefaultSuffixes_clicked();
  void on_btnProtelSuffixes_clicked();
  void on_btnGenerate_clicked();
  void on_btnBrowseOutputDir_clicked();

private:
  QStringList getTopSilkscreenLayers() const noexcept;
  QStringList getBotSilkscreenLayers() const noexcept;

  Project& mProject;
  Board& mBoard;
  Ui::FabricationOutputDialog* mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
