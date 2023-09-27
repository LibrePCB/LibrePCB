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
#include "librarygui.h"

#include "openedproject.h"

#include <librepcb/core/library/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryGui::LibraryGui(OpenedProject& project, Library& library) noexcept
  : QObject(&project), mProject(project), mLibrary(&library) {
}

LibraryGui::~LibraryGui() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QString& LibraryGui::getName() const noexcept {
  Q_ASSERT(mLibrary);
  return *mLibrary->getNames().getDefaultValue();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
