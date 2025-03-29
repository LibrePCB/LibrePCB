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

#ifndef LIBREPCB_EDITOR_DOWNLOADLIBRARYTAB_H
#define LIBREPCB_EDITOR_DOWNLOADLIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class LibraryDownload;

/*******************************************************************************
 *  Class DownloadLibraryTab
 ******************************************************************************/

/**
 * @brief The DownloadLibraryTab class
 */
class DownloadLibraryTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<DownloadLibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  DownloadLibraryTab() = delete;
  DownloadLibraryTab(const DownloadLibraryTab& other) = delete;
  explicit DownloadLibraryTab(GuiApplication& app,
                              QObject* parent = nullptr) noexcept;
  ~DownloadLibraryTab() noexcept;

  // General Methods
  ui::TabData getUiData() const noexcept override;
  const ui::DownloadLibraryTabData& getDerivedUiData() const noexcept {
    return mUiData;
  }
  void setDerivedUiData(const ui::DownloadLibraryTabData& data) noexcept;

  // Operator Overloadings
  DownloadLibraryTab& operator=(const DownloadLibraryTab& rhs) = delete;

protected:
  void triggerAsync(ui::Action a) noexcept override;

private:
  void validate() noexcept;
  void downloadFinished(bool success, const QString& errMsg) noexcept;

  ui::DownloadLibraryTabData mUiData;
  std::optional<QUrl> mUrl;
  FilePath mDirectory;
  std::unique_ptr<LibraryDownload> mDownload;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
