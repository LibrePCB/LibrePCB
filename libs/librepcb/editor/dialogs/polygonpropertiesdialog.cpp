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
#include "../undostack.h"
#include "ui_polygonpropertiesdialog.h"

#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/graphics/graphicslayer.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

PolygonPropertiesDialog::PolygonPropertiesDialog(Polygon& polygon,
                                                 UndoStack& undoStack,
                                                 QList<GraphicsLayer*> layers,
                                                 const LengthUnit& lengthUnit,
                                                 const QString& settingsPrefix,
                                                 QWidget* parent) noexcept
  : QDialog(parent),
    mPolygon(polygon),
    mUndoStack(undoStack),
    mUi(new Ui::PolygonPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/line_width");
  mUi->pathEditorWidget->setLengthUnit(lengthUnit);

  foreach (const GraphicsLayer* layer, layers) {
    mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
  }

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &PolygonPropertiesDialog::buttonBoxClicked);

  // load polygon attributes
  selectLayerNameInCombobox(*mPolygon.getLayerName());
  mUi->edtLineWidth->setValue(mPolygon.getLineWidth());
  mUi->cbxFillArea->setChecked(mPolygon.isFilled());
  mUi->cbxIsGrabArea->setChecked(mPolygon.isGrabArea());

  // load vertices
  mUi->pathEditorWidget->setPath(mPolygon.getPath());
}

PolygonPropertiesDialog::~PolygonPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PolygonPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxLayer->setDisabled(readOnly);
  mUi->edtLineWidth->setReadOnly(readOnly);
  mUi->cbxFillArea->setCheckable(!readOnly);
  mUi->cbxIsGrabArea->setCheckable(!readOnly);
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
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(mPolygon));
    if (mUi->cbxLayer->currentIndex() >= 0 &&
        mUi->cbxLayer->currentData().isValid()) {
      cmd->setLayerName(
          GraphicsLayerName(mUi->cbxLayer->currentData().toString()),
          false);  // can throw
    }
    cmd->setIsFilled(mUi->cbxFillArea->isChecked(), false);
    cmd->setIsGrabArea(mUi->cbxIsGrabArea->isChecked(), false);
    cmd->setLineWidth(mUi->edtLineWidth->getValue(), false);
    cmd->setPath(mUi->pathEditorWidget->getPath(), false);  // can throw
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

void PolygonPropertiesDialog::selectLayerNameInCombobox(
    const QString& name) noexcept {
  mUi->cbxLayer->setCurrentIndex(mUi->cbxLayer->findData(name));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
