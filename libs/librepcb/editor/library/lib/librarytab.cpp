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
#include "librarytab.h"

#include "../libraryeditor2.h"
#include "libraryelementsmodel.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/library/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryTab::LibraryTab(GuiApplication& app, LibraryEditor2& editor,
                       QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mLibrary(mEditor.getLibrary()),
    mElementsModel(new LibraryElementsModel(
        mEditor.getWorkspace(), mLibrary.getDirectory().getAbsPath())) {
}

LibraryTab::~LibraryTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int LibraryTab::getLibraryIndex() const noexcept {
  return mEditor.getUiIndex();
}

ui::TabData LibraryTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Library,  // Type
      q2s(*mLibrary.getNames().getDefaultValue()),  // Title
      ui::TabFeatures{},  // Features
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::LibraryTabData LibraryTab::getDerivedUiData() const noexcept {
  return ui::LibraryTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mLibrary.getIconAsPixmap()),  // Icon
      q2s(*mLibrary.getNames().getDefaultValue()),  // Name
      q2s(mLibrary.getDescriptions().getDefaultValue()),  // Description
      nullptr,  // Keywords
      q2s(mLibrary.getAuthor()),  // Author
      q2s(mLibrary.getVersion().toStr()),  // Version
      mLibrary.isDeprecated(),  // Deprecated
      q2s(mLibrary.getUrl().toString()),  // URL
      nullptr,  // Dependencies
      q2s(*mLibrary.getManufacturer()),  // Manufacturer
      mElementsModel,  // Elements
  };
}

void LibraryTab::setDerivedUiData(const ui::LibraryTabData& data) noexcept {
  Q_UNUSED(data);
}

void LibraryTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
