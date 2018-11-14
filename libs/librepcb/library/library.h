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

#ifndef LIBREPCB_LIBRARY_LIBRARY_H
#define LIBREPCB_LIBRARY_LIBRARY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "librarybaseelement.h"

#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class Library
 ******************************************************************************/

/**
 * @brief   The Library class represents a library directory
 *
 * @author  ubruhin
 * @date    2016-08-03
 */
class Library final : public LibraryBaseElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Library()                     = delete;
  Library(const Library& other) = delete;
  Library(const Uuid& uuid, const Version& version, const QString& author,
          const ElementName& name_en_US, const QString& description_en_US,
          const QString& keywords_en_US);
  Library(const FilePath& libDir, bool readOnly);
  ~Library() noexcept;

  // Getters
  template <typename ElementType>
  FilePath          getElementsDirectory() const noexcept;
  const QUrl&       getUrl() const noexcept { return mUrl; }
  const QSet<Uuid>& getDependencies() const noexcept { return mDependencies; }
  FilePath          getIconFilePath() const noexcept;
  const QByteArray& getIcon() const noexcept { return mIcon; }
  QPixmap           getIconAsPixmap() const noexcept;

  // Setters
  void setUrl(const QUrl& url) noexcept { mUrl = url; }
  void setDependencies(const QSet<Uuid>& deps) noexcept {
    mDependencies = deps;
  }
  void setIcon(const QByteArray& png) noexcept { mIcon = png; }

  // General Methods
  virtual void save() override;
  template <typename ElementType>
  QList<FilePath> searchForElements() const noexcept;

  // Operator Overloadings
  Library& operator=(const Library& rhs) = delete;

  // Static Methods
  static QString getShortElementName() noexcept {
    return QStringLiteral("lib");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("library");
  }

private:  // Methods
  // Private Methods
  virtual void copyTo(const FilePath& destination, bool removeSource) override;
  /// @copydoc librepcb::SerializableObject::serialize()
  virtual void serialize(SExpression& root) const override;

private:  // Data
  QUrl       mUrl;
  QSet<Uuid> mDependencies;
  QByteArray mIcon;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_LIBRARY_H
