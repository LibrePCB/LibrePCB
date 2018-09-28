/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "schematiceditorevent.h"

#include "../schematiceditor.h"
#include "ui_schematiceditor.h"

#include <librepcb/project/schematics/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Class SEE_Base
 ******************************************************************************/

SEE_Base::SEE_Base(EventType_t type) : mType(type), mAccepted(false) {
}

SEE_Base::~SEE_Base() {
}

/*******************************************************************************
 *  Class SEE_StartAddComponent
 ******************************************************************************/

SEE_StartAddComponent::SEE_StartAddComponent()
  : SEE_Base(EventType_t::StartAddComponent), mComponentUuid(), mSymbVarUuid() {
}

SEE_StartAddComponent::SEE_StartAddComponent(const Uuid& cmp,
                                             const Uuid& symbVar)
  : SEE_Base(EventType_t::StartAddComponent),
    mComponentUuid(cmp),
    mSymbVarUuid(symbVar) {
}

SEE_StartAddComponent::~SEE_StartAddComponent() {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
