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

#ifndef LIBREPCB_LIBRARY_LIBRARYCREATOR_H
#define LIBREPCB_LIBRARY_LIBRARYCREATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/fileproofname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {
namespace app {

/*******************************************************************************
 *  Class LibraryCreator
 ******************************************************************************/

/**
 * @brief The LibraryCreator class
 */
class LibraryCreator : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryCreator() = delete;
  LibraryCreator(const LibraryCreator& other) = delete;
  explicit LibraryCreator(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~LibraryCreator() noexcept;

  // General Methods
  QString setName(const QString& input) noexcept;
  QString getName() const noexcept { return mName ? **mName : QString(); }
  QString setDescription(const QString& input) noexcept;
  QString getDescription() const noexcept { return mDescription; }
  QString setAuthor(const QString& input) noexcept;
  QString getAuthor() const noexcept { return mAuthor; }
  QString setVersion(const QString& input) noexcept;
  QString getVersion() const noexcept {
    return mVersion ? mVersion->toStr() : QString();
  }
  QString setDirectory(const QString& input, const QString& fallback) noexcept;
  QString getDirectory() const noexcept {
    return mDirectory ? **mDirectory : QString();
  }
  static QString getDirectoryForName(const QString& input) noexcept;
  QString create() noexcept;

  // Operator Overloadings
  LibraryCreator& operator=(const LibraryCreator& rhs) = delete;

signals:
  void validChanged(bool valid);

private:
  void validate() noexcept;

  Workspace& mWorkspace;
  std::optional<ElementName> mName;
  QString mDescription;
  QString mAuthor;
  std::optional<Version> mVersion;
  std::optional<FileProofName> mDirectory;
  std::optional<FileProofName> mFallbackDirectory;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
