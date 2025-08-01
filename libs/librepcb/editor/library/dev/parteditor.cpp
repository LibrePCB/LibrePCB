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
#include "parteditor.h"

#include "../../modelview/attributelistmodel.h"
#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdpartedit.h"

#include <librepcb/core/library/dev/part.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartEditor::PartEditor(std::shared_ptr<Part> part, UndoStack* stack,
                       QObject* parent) noexcept
  : QObject(parent),
    mPart(part),
    mUndoStack(stack),
    mAttributes(new AttributeListModel()) {
  mAttributes->setReferences(&part->getAttributes(), stack);
}

PartEditor::~PartEditor() noexcept {
  mAttributes->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::PartData PartEditor::getUiData() const {
  return ui::PartData{
      q2s(*mPart->getMpn()),  // MPN
      q2s(*mPart->getManufacturer()),  // Manufacturer
      mAttributes,  // Attributes
      ui::PartAction::None,  // Action
  };
}

void PartEditor::setUiData(const ui::PartData& data, bool allowEmpty) noexcept {
  try {
    const QString mpnStr = s2q(data.mpn);
    const SimpleString mpn = cleanSimpleString(mpnStr);
    const QString mfrStr = s2q(data.manufacturer);
    const SimpleString mfr = cleanSimpleString(mfrStr);

    std::unique_ptr<CmdPartEdit> cmd(new CmdPartEdit(mPart));
    if ((mpnStr != mPart->getMpn()) && ((!mpn->isEmpty()) || allowEmpty)) {
      cmd->setMpn(mpn);
    }
    if ((mfrStr != mPart->getManufacturer()) &&
        ((!mfr->isEmpty()) || allowEmpty)) {
      cmd->setManufacturer(mfr);
    }
    execCmd(cmd.release());
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void PartEditor::apply() {
  mAttributes->apply();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PartEditor::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
