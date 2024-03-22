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
#include "zonepropertiesdialog.h"

#include "../cmd/cmdzoneedit.h"
#include "../graphics/graphicslayer.h"
#include "../project/cmd/cmdboardzoneedit.h"
#include "../undostack.h"
#include "ui_zonepropertiesdialog.h"

#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

ZonePropertiesDialog::ZonePropertiesDialog(Zone* libZone, BI_Zone* boardZone,
                                           const QList<const Layer*> allLayers,
                                           UndoStack& undoStack,
                                           const LengthUnit& lengthUnit,
                                           const IF_GraphicsLayerProvider& lp,
                                           const QString& settingsPrefix,
                                           QWidget* parent) noexcept
  : QDialog(parent),
    mLibraryObj(libZone),
    mBoardObj(boardZone),
    mUndoStack(undoStack),
    mUi(new Ui::ZonePropertiesDialog) {
  Q_UNUSED(settingsPrefix);

  mUi->setupUi(this);
  mUi->pathEditorWidget->setMinimumVertexCount(2);
  mUi->pathEditorWidget->setLengthUnit(lengthUnit);

  foreach (const Layer* layer, allLayers) {
    QString text;
    if (!layer) {
      text = tr("Inner Layers");
    } else if (layer->isTop()) {
      text = tr("Top Side");
    } else if (layer->isBottom()) {
      text = tr("Bottom Side");
    } else {
      text = layer->getNameTr();
    }
    QListWidgetItem* item = new QListWidgetItem(text, mUi->lstLayers);
    const Layer* colorLayer = layer ? layer : Layer::innerCopper(1);
    Q_ASSERT(colorLayer);
    if (auto graphicsLayer = lp.getLayer(*colorLayer)) {
      item->setData(Qt::DecorationRole, graphicsLayer->getColor());
    }
    item->setData(Qt::UserRole, QVariant::fromValue(layer));
  }

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &ZonePropertiesDialog::buttonBoxClicked);
}

ZonePropertiesDialog::ZonePropertiesDialog(Zone& zone, UndoStack& undoStack,
                                           const LengthUnit& lengthUnit,
                                           const IF_GraphicsLayerProvider& lp,
                                           const QString& settingsPrefix,
                                           QWidget* parent) noexcept
  : ZonePropertiesDialog(&zone, nullptr,
                         {&Layer::topCopper(), nullptr, &Layer::botCopper()},
                         undoStack, lengthUnit, lp, settingsPrefix, parent) {
  QSet<const Layer*> layers;
  if (zone.getLayers().testFlag(Zone::Layer::Top)) {
    layers.insert(&Layer::topCopper());
  }
  if (zone.getLayers().testFlag(Zone::Layer::Inner)) {
    layers.insert(nullptr);
  }
  if (zone.getLayers().testFlag(Zone::Layer::Bottom)) {
    layers.insert(&Layer::botCopper());
  }
  load(zone, layers);
  mUi->gbxOptions->hide();
}

ZonePropertiesDialog::ZonePropertiesDialog(BI_Zone& zone, UndoStack& undoStack,
                                           const LengthUnit& lengthUnit,
                                           const IF_GraphicsLayerProvider& lp,
                                           const QString& settingsPrefix,
                                           QWidget* parent) noexcept
  : ZonePropertiesDialog(nullptr, &zone,
                         Toolbox::sortedQSet(zone.getBoard().getCopperLayers(),
                                             &Layer::lessThan),
                         undoStack, lengthUnit, lp, settingsPrefix, parent) {
  load(zone.getData(), zone.getData().getLayers());
  mUi->cbxLock->setChecked(zone.getData().isLocked());
}

ZonePropertiesDialog::~ZonePropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ZonePropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->lstLayers->setEnabled(!readOnly);
  mUi->cbxNoCopper->setEnabled(!readOnly);
  mUi->cbxNoPlanes->setEnabled(!readOnly);
  mUi->cbxNoExposure->setEnabled(!readOnly);
  mUi->cbxNoDevices->setEnabled(!readOnly);
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
void ZonePropertiesDialog::load(
    const T& obj, const QSet<const Layer*> checkedLayers) noexcept {
  for (int i = 0; i < mUi->lstLayers->count(); ++i) {
    QListWidgetItem* item = mUi->lstLayers->item(i);
    Q_ASSERT(item);
    const Layer* layer = item->data(Qt::UserRole).value<const Layer*>();
    item->setSelected(checkedLayers.contains(layer));
  }
  mUi->cbxNoCopper->setChecked(obj.getRules().testFlag(Zone::Rule::NoCopper));
  mUi->cbxNoPlanes->setChecked(obj.getRules().testFlag(Zone::Rule::NoPlanes));
  mUi->cbxNoExposure->setChecked(
      obj.getRules().testFlag(Zone::Rule::NoExposure));
  mUi->cbxNoDevices->setChecked(obj.getRules().testFlag(Zone::Rule::NoDevices));
  mUi->pathEditorWidget->setPath(obj.getOutline());
}

void ZonePropertiesDialog::buttonBoxClicked(QAbstractButton* button) noexcept {
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

bool ZonePropertiesDialog::applyChanges() noexcept {
  try {
    QSet<const Layer*> enabledLayers;
    QSet<const Layer*> disabledLayers;
    for (int i = 0; i < mUi->lstLayers->count(); ++i) {
      const QListWidgetItem* item = mUi->lstLayers->item(i);
      Q_ASSERT(item);
      const Layer* layer = item->data(Qt::UserRole).value<const Layer*>();
      if (item->isSelected()) {
        enabledLayers.insert(layer);
      } else {
        disabledLayers.insert(layer);
      }
    }

    if (mLibraryObj) {
      QScopedPointer<CmdZoneEdit> cmd(new CmdZoneEdit(*mLibraryObj));
      applyChanges(*cmd);
      Zone::Layers layers = mLibraryObj->getLayers();
      layers.setFlag(Zone::Layer::Top,
                     enabledLayers.contains(&Layer::topCopper()));
      layers.setFlag(Zone::Layer::Inner, enabledLayers.contains(nullptr));
      layers.setFlag(Zone::Layer::Bottom,
                     enabledLayers.contains(&Layer::botCopper()));
      cmd->setLayers(layers, false);
      mUndoStack.execCmd(cmd.take());  // can throw
    }

    if (mBoardObj) {
      QScopedPointer<CmdBoardZoneEdit> cmd(new CmdBoardZoneEdit(*mBoardObj));
      applyChanges(*cmd);
      QSet<const Layer*> layers = mBoardObj->getData().getLayers();
      layers -= disabledLayers;
      layers += enabledLayers;
      cmd->setLayers(layers, false);
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
void ZonePropertiesDialog::applyChanges(T& cmd) {
  Zone::Rules rules(0);
  rules.setFlag(Zone::Rule::NoCopper, mUi->cbxNoCopper->isChecked());
  rules.setFlag(Zone::Rule::NoPlanes, mUi->cbxNoPlanes->isChecked());
  rules.setFlag(Zone::Rule::NoExposure, mUi->cbxNoExposure->isChecked());
  rules.setFlag(Zone::Rule::NoDevices, mUi->cbxNoDevices->isChecked());
  cmd.setRules(rules, false);

  cmd.setOutline(mUi->pathEditorWidget->getPath().toOpenPath(), false);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
