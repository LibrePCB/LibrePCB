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
#include "boardviapropertiesdialog.h"

#include "ui_boardviapropertiesdialog.h"

#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/circuit/netsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardViaPropertiesDialog::BoardViaPropertiesDialog(Project&   project,
                                                   BI_Via&    via,
                                                   UndoStack& undoStack,
                                                   QWidget*   parent) noexcept
  : QDialog(parent),
    mProject(project),
    mVia(via),
    mUi(new Ui::BoardViaPropertiesDialog),
    mUndoStack(undoStack) {
  mUi->setupUi(this);

  // shape combobox
  mUi->cbxShape->addItem(tr("Round"), static_cast<int>(BI_Via::Shape::Round));
  mUi->cbxShape->addItem(tr("Square"), static_cast<int>(BI_Via::Shape::Square));
  mUi->cbxShape->addItem(tr("Octagon"),
                         static_cast<int>(BI_Via::Shape::Octagon));
  mUi->cbxShape->setCurrentIndex(
      mUi->cbxShape->findData(static_cast<int>(mVia.getShape())));

  // Position spinboxes
  mUi->spbxPosX->setValue(mVia.getPosition().getX().toMm());
  mUi->spbxPosY->setValue(mVia.getPosition().getY().toMm());

  // size spinbox
  mUi->spbxSize->setValue(mVia.getSize()->toMm());

  // drill diameter spinbox
  mUi->spbxDrillDiameter->setValue(mVia.getDrillDiameter()->toMm());

  // netsignal combobox
  mUi->lblNetSignal->setText(*mVia.getNetSignalOfNetSegment().getName());

  BoardLayerStack* layerStack = &via.getBoard().getLayerStack();
  for (int i = 0; i < layerStack->getCopperLayerCount() - 1; ++i){
    mUi->cbxStartLayer->addItem(layerStack->getCopperLayer(i)->getNameTr());
  }
  connect(mUi->cbxStartLayer,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
          this, &BoardViaPropertiesDialog::startLayerChanged);
  mUi->cbxStartLayer->setCurrentIndex(via.getStartLayer());

  for (int i = 1; i < layerStack->getCopperLayerCount(); ++i){
    mUi->cbxStopLayer->addItem(layerStack->getCopperLayer(i)->getNameTr());
  }
  connect(mUi->cbxStopLayer,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
          this, &BoardViaPropertiesDialog::stopLayerChanged);
  int stopLayerIndex = via.getStopLayer();
  if (stopLayerIndex > layerStack->getInnerLayerCount()){
    stopLayerIndex = GraphicsLayer::getInnerLayerCount() + 1;
  }
  mUi->cbxStopLayer->setCurrentIndex(stopLayerIndex);
  qDebug() << "via properties" << via.getStartLayer() << " " << via.getStopLayer() << " " << stopLayerIndex;
}

BoardViaPropertiesDialog::~BoardViaPropertiesDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Slots
 ******************************************************************************/

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardViaPropertiesDialog::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Return:
      accept();
      break;
    case Qt::Key_Escape:
      reject();
      break;
    default:
      QDialog::keyPressEvent(e);
      break;
  }
}

void BoardViaPropertiesDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool BoardViaPropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(mVia));
    cmd->setShape(
        static_cast<BI_Via::Shape>(mUi->cbxShape->currentData().toInt()),
        false);
    cmd->setPosition(Point(Length::fromMm(mUi->spbxPosX->value()),
                           Length::fromMm(mUi->spbxPosY->value())),
                     false);
    cmd->setSize(PositiveLength(Length::fromMm(mUi->spbxSize->value())),
                 false);  // can throw
    cmd->setDrillDiameter(
        PositiveLength(Length::fromMm(mUi->spbxDrillDiameter->value())),
        false);  // can throw
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

void BoardViaPropertiesDialog::startLayerChanged(int index) noexcept{
  if (mUi->cbxStopLayer->currentIndex() <= index){
    mUi->cbxStopLayer->setCurrentIndex(index);
  }
}

void BoardViaPropertiesDialog::stopLayerChanged(int index) noexcept{
  if (mUi->cbxStartLayer->currentIndex() >= index){
    mUi->cbxStartLayer->setCurrentIndex(index);
  }

}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
