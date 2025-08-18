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

#ifndef LIBREPCB_EDITOR_BOMREVIEWDIALOG_H
#define LIBREPCB_EDITOR_BOMREVIEWDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "partinformationprovider.h"

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
class BomReviewDialog;
}

/*******************************************************************************
 *  Class BomReviewDialog
 ******************************************************************************/

/**
 * @brief The BomReviewDialog class
 */
class BomReviewDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BomReviewDialog() = delete;
  BomReviewDialog(const BomReviewDialog& other) = delete;
  BomReviewDialog(const WorkspaceSettings& settings, Project& project,
                  const Board* board = nullptr,
                  QWidget* parent = nullptr) noexcept;
  ~BomReviewDialog() noexcept;

  // General Methods
  virtual bool eventFilter(QObject* obj, QEvent* e) noexcept override;

  // Operator Overloads
  BomReviewDialog& operator=(const BomReviewDialog& rhs) = delete;

signals:
  void projectSettingsModified();

private:  // GUI Event Handlers
  void cbxBoardCurrentIndexChanged(int index) noexcept;
  void btnOpenOutputDirectoryClicked() noexcept;
  void tableCellDoubleClicked(int row, int column) noexcept;

private:  // Methods
  void updateAttributes() noexcept;
  void updateBom() noexcept;
  void updateTable() noexcept;
  void updatePartsInformation() noexcept;
  std::shared_ptr<AssemblyVariant> getAssemblyVariant() const noexcept;
  std::optional<Uuid> getAssemblyVariantUuid(bool throwIfNullopt) const;

private:  // Data
  const WorkspaceSettings& mSettings;
  Project& mProject;
  std::shared_ptr<Bom> mBom;
  QScopedPointer<Ui::BomReviewDialog> mUi;
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
