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

#ifndef LIBREPCB_EDITOR_LIBRARYEDITOR2_H
#define LIBREPCB_EDITOR_LIBRARYEDITOR2_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../utils/uiobjectlist.h"
#include "appwindow.h"

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class Workspace;

/*******************************************************************************
 *  Class LibraryEditor2
 ******************************************************************************/

/**
 * @brief The LibraryEditor2 class
 */
class LibraryEditor2 final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<LibraryEditor2> onUiDataChanged;

  // Constructors / Destructor
  LibraryEditor2() = delete;
  LibraryEditor2(const LibraryEditor2& other) = delete;
  explicit LibraryEditor2(GuiApplication& app,
                          QObject* parent = nullptr) noexcept;
  ~LibraryEditor2() noexcept;

  // General Methods
  GuiApplication& getApp() noexcept { return mApp; }
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  // int getUiIndex() const noexcept { return mUiIndex; }
  // void setUiIndex(int index) noexcept;
  // ui::ProjectData getUiData() const noexcept;
  // void setUiData(const ui::ProjectData& data) noexcept;

  // Operator Overloadings
  LibraryEditor2& operator=(const LibraryEditor2& rhs) = delete;

signals:
  // void uiIndexChanged();
  void statusBarMessageChanged(const QString& message, int timeoutMs);

private:
  GuiApplication& mApp;
  Workspace& mWorkspace;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
