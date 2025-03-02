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
#include "boardplanepropertiesdialog.h"

#include "../../project/cmd/cmdboardplaneedit.h"
#include "../../undostack.h"
#include "ui_boardplanepropertiesdialog.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPlanePropertiesDialog::BoardPlanePropertiesDialog(
    Project& project, BI_Plane& plane, UndoStack& undoStack,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mProject(project),
    mPlane(plane),
    mUi(new Ui::BoardPlanePropertiesDialog),
    mUndoStack(undoStack) {
  mUi->setupUi(this);
  mUi->edtMinWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                              settingsPrefix % "/min_width");
  mUi->edtMinClearance->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  settingsPrefix % "/min_clearance");
  mUi->edtThermalGap->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                settingsPrefix % "/thermal_gap");
  mUi->edtThermalSpokeWidth->configure(lengthUnit,
                                       LengthEditBase::Steps::generic(),
                                       settingsPrefix % "/thermal_spoke");
  mUi->pathEditorWidget->setLengthUnit(lengthUnit);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &BoardPlanePropertiesDialog::buttonBoxClicked);

  // net signal combobox
  connect(
      mUi->cbxNetSignal,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, [this](int index) {
        const bool hasNet =
            !mUi->cbxNetSignal->itemData(index).toString().isEmpty();
        mUi->cbxConnectStyle->setEnabled(hasNet);
        mUi->cbKeepIslands->setEnabled(hasNet);
      });
  QList<NetSignal*> netSignals = mPlane.getCircuit().getNetSignals().values();
  Toolbox::sortNumeric(
      netSignals,
      [](const QCollator& cmp, const NetSignal* lhs, const NetSignal* rhs) {
        return cmp(*lhs->getName(), *rhs->getName());
      },
      Qt::CaseInsensitive, false);
  mUi->cbxNetSignal->addItem("[" % tr("None") % "]", QString());
  foreach (NetSignal* netsignal, netSignals) {
    mUi->cbxNetSignal->addItem(*netsignal->getName(),
                               netsignal->getUuid().toStr());
  }
  mUi->cbxNetSignal->setCurrentIndex(mUi->cbxNetSignal->findData(
      mPlane.getNetSignal() ? mPlane.getNetSignal()->getUuid().toStr()
                            : QString()));

  // layer combobox
  mUi->cbxLayer->setLayers(mPlane.getBoard().getCopperLayers());
  mUi->cbxLayer->setCurrentLayer(mPlane.getLayer());

  // minimum width / clearance spinbox
  mUi->edtMinWidth->setValue(mPlane.getMinWidth());
  mUi->edtMinClearance->setValue(mPlane.getMinClearance());

  // connect style combobox
  mUi->cbxConnectStyle->addItem(tr("None"),
                                static_cast<int>(BI_Plane::ConnectStyle::None));
  mUi->cbxConnectStyle->addItem(
      tr("Thermal Relief"),
      static_cast<int>(BI_Plane::ConnectStyle::ThermalRelief));
  mUi->cbxConnectStyle->addItem(
      tr("Solid"), static_cast<int>(BI_Plane::ConnectStyle::Solid));
  mUi->cbxConnectStyle->setCurrentIndex(mUi->cbxConnectStyle->findData(
      static_cast<int>(mPlane.getConnectStyle())));

  // thermal gap/width spinbox
  mUi->edtThermalGap->setValue(mPlane.getThermalGap());
  mUi->edtThermalSpokeWidth->setValue(mPlane.getThermalSpokeWidth());

  // priority spinbox
  mUi->spbPriority->setValue(mPlane.getPriority());

  // checkboxes
  mUi->cbKeepIslands->setChecked(mPlane.getKeepIslands());
  mUi->cbxLock->setChecked(mPlane.isLocked());

  // vertices
  mUi->pathEditorWidget->setPath(mPlane.getOutline());

  // Make sure the thermal spoke width is >= minimum plane width.
  connect(mUi->edtMinWidth, &UnsignedLengthEdit::valueChanged, this,
          [this](const UnsignedLength& value) {
            if (value > 0) {
              mUi->edtThermalSpokeWidth->clipToMinimum(PositiveLength(*value));
            }
          });
  connect(mUi->edtThermalSpokeWidth, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            mUi->edtMinWidth->clipToMaximum(positiveToUnsigned(value));
          });
}

BoardPlanePropertiesDialog::~BoardPlanePropertiesDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPlanePropertiesDialog::buttonBoxClicked(
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

bool BoardPlanePropertiesDialog::applyChanges() noexcept {
  try {
    std::unique_ptr<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(mPlane));

    // net signal
    const std::optional<Uuid> netSignalUuid =
        Uuid::tryFromString(mUi->cbxNetSignal->currentData().toString());
    if (!netSignalUuid) {
      cmd->setNetSignal(nullptr);
    } else if (NetSignal* netsignal =
                   mPlane.getCircuit().getNetSignals().value(*netSignalUuid)) {
      cmd->setNetSignal(netsignal);
    } else {
      qWarning() << "No valid netsignal selected in plane properties dialog!";
    }

    // layer
    if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
      cmd->setLayer(*layer, false);  // can throw
    }

    // min width/clearance
    cmd->setMinWidth(mUi->edtMinWidth->getValue());
    cmd->setMinClearance(mUi->edtMinClearance->getValue());

    // connect style
    cmd->setConnectStyle(static_cast<BI_Plane::ConnectStyle>(
        mUi->cbxConnectStyle->currentData().toInt()));

    // thermal gap & spoke width spinbox
    cmd->setThermalGap(mUi->edtThermalGap->getValue());
    cmd->setThermalSpokeWidth(mUi->edtThermalSpokeWidth->getValue());

    // priority
    cmd->setPriority(mUi->spbPriority->value());

    // booleans
    cmd->setKeepIslands(mUi->cbKeepIslands->isChecked());
    cmd->setLocked(mUi->cbxLock->isChecked());

    // vertices
    cmd->setOutline(mUi->pathEditorWidget->getPath(), false);  // can throw

    // apply changes
    mUndoStack.execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
