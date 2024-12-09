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

#ifndef LIBREPCB_EDITOR_BOMGENERATORDIALOG_H
#define LIBREPCB_EDITOR_BOMGENERATORDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "partinformationprovider.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AssemblyVariant;
class Board;
class Bom;
class Project;
class Uuid;
class WorkspaceSettings;

namespace editor {

class PartInformationToolTip;

namespace Ui {
class BomGeneratorDialog;
}

/*******************************************************************************
 *  Class BomGeneratorDialog
 ******************************************************************************/

/**
 * @brief The BomGeneratorDialog class
 */
class BomGeneratorDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BomGeneratorDialog() = delete;
  BomGeneratorDialog(const BomGeneratorDialog& other) = delete;
  BomGeneratorDialog(const WorkspaceSettings& settings, Project& project,
                     const Board* board = nullptr,
                     QWidget* parent = nullptr) noexcept;
  ~BomGeneratorDialog() noexcept;

  // General Methods
  virtual bool eventFilter(QObject* obj, QEvent* e) noexcept override;

  // Operator Overloads
  BomGeneratorDialog& operator=(const BomGeneratorDialog& rhs) = delete;

signals:
  void projectSettingsModified();

private:  // GUI Event Handlers
  void cbxBoardCurrentIndexChanged(int index) noexcept;
  void btnChooseOutputPathClicked() noexcept;
  void btnOpenOutputDirectoryClicked() noexcept;
  void btnGenerateClicked() noexcept;
  void tableCellDoubleClicked(int row, int column) noexcept;

private:  // Methods
  void updateAttributes() noexcept;
  void updateBom() noexcept;
  void updateTable() noexcept;
  void updatePartsInformation() noexcept;
  std::shared_ptr<AssemblyVariant> getAssemblyVariant() const noexcept;
  std::optional<Uuid> getAssemblyVariantUuid(bool throwIfNullopt) const;
  FilePath getOutputFilePath() const noexcept;

private:  // Data
  const WorkspaceSettings& mSettings;
  Project& mProject;
  std::shared_ptr<Bom> mBom;
  QScopedPointer<Ui::BomGeneratorDialog> mUi;
  QPointer<QPushButton> mBtnGenerate;
  QScopedPointer<PartInformationToolTip> mPartToolTip;
  uint mPartInfoProgress;
  bool mUpdatePartInformationScheduled;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
