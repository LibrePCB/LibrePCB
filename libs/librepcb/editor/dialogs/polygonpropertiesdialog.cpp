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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "polygonpropertiesdialog.h"

#include "../cmd/cmdpolygonedit.h"
#include "../project/cmd/cmdboardpolygonedit.h"
#include "../undostack.h"
#include "ui_polygonpropertiesdialog.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/board/items/bi_polygon.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

PolygonPropertiesDialog::PolygonPropertiesDialog(
    Polygon* libPolygon, BI_Polygon* boardPolygon, UndoStack& undoStack,
    const QSet<const Layer*>& layers, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mLibraryObj(libPolygon),
    mBoardObj(boardPolygon),
    mUndoStack(undoStack),
    mUi(new Ui::PolygonPropertiesDialog) {
  mUi->setupUi(this);
  mUi->cbxLayer->setLayers(layers);
  mUi->edtLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/line_width");
  mUi->pathEditorWidget->setLengthUnit(lengthUnit);

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &PolygonPropertiesDialog::buttonBoxClicked);
}

PolygonPropertiesDialog::PolygonPropertiesDialog(
    Polygon& polygon, UndoStack& undoStack, const QSet<const Layer*>& layers,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : PolygonPropertiesDialog(&polygon, nullptr, undoStack, layers, lengthUnit,
                            settingsPrefix, parent) {
  load(polygon);
  mUi->cbxLock->hide();
}

PolygonPropertiesDialog::PolygonPropertiesDialog(
    BI_Polygon& polygon, UndoStack& undoStack, const QSet<const Layer*>& layers,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : PolygonPropertiesDialog(nullptr, &polygon, undoStack, layers, lengthUnit,
                            settingsPrefix, parent) {
  load(polygon.getData());
  mUi->cbxLock->setChecked(polygon.getData().isLocked());
}

PolygonPropertiesDialog::~PolygonPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PolygonPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxLayer->setDisabled(readOnly);
  mUi->edtLineWidth->setReadOnly(readOnly);
  mUi->cbxFillArea->setEnabled(!readOnly);
  mUi->cbxIsGrabArea->setEnabled(!readOnly);
  mUi->cbxLock->setEnabled(!readOnly);
  mUi->pathEditorWidget->setReadOnly(readOnly);
  if (readOnly) {
    mUi->buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Close);
  } else {
    mUi->buttonBox->setStandardButtons(
        QDialogButtonBox::StandardButton::Apply |
        QDialogButtonBox::StandardButton::Cancel |
        QDialogButtonBox::StandardButton::Ok);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename T>
void PolygonPropertiesDialog::load(const T& obj) noexcept {
  mUi->cbxLayer->setCurrentLayer(obj.getLayer());
  mUi->edtLineWidth->setValue(obj.getLineWidth());
  mUi->cbxFillArea->setChecked(obj.isFilled());
  mUi->cbxIsGrabArea->setChecked(obj.isGrabArea());
  mUi->pathEditorWidget->setPath(obj.getPath());
}

void PolygonPropertiesDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      if (applyChanges()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

bool PolygonPropertiesDialog::applyChanges() noexcept {
  try {
    if (mLibraryObj) {
      QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(*mLibraryObj));
      applyChanges(*cmd);
      mUndoStack.execCmd(cmd.take());  // can throw
    }
    if (mBoardObj) {
      QScopedPointer<CmdBoardPolygonEdit> cmd(
          new CmdBoardPolygonEdit(*mBoardObj));
      applyChanges(*cmd);
      cmd->setLocked(mUi->cbxLock->isChecked());
      mUndoStack.execCmd(cmd.take());  // can throw
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

template <typename T>
void PolygonPropertiesDialog::applyChanges(T& cmd) {
  if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
    cmd.setLayer(*layer, false);
  }
  cmd.setIsFilled(mUi->cbxFillArea->isChecked(), false);
  cmd.setIsGrabArea(mUi->cbxIsGrabArea->isChecked(), false);
  cmd.setLineWidth(mUi->edtLineWidth->getValue(), false);
  cmd.setPath(mUi->pathEditorWidget->getPath(), false);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
