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

#ifndef LIBREPCB_CORE_LIBRARY_H
#define LIBREPCB_CORE_LIBRARY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/simplestring.h"
#include "../types/uuid.h"
#include "librarybaseelement.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Library
 ******************************************************************************/

/**
 * @brief   The Library class represents a library directory
 */
class Library final : public LibraryBaseElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Library() = delete;
  Library(const Library& other) = delete;
  Library(const Uuid& uuid, const Version& version, const QString& author,
          const ElementName& name_en_US, const QString& description_en_US,
          const QString& keywords_en_US);
  ~Library() noexcept;

  // Getters
  template <typename ElementType>
  QString getElementsDirectoryName() const noexcept;
  const QUrl& getUrl() const noexcept { return mUrl; }
  const QSet<Uuid>& getDependencies() const noexcept { return mDependencies; }
  const QByteArray& getIcon() const noexcept { return mIcon; }
  QPixmap getIconAsPixmap() const noexcept;
  const SimpleString& getManufacturer() const noexcept { return mManufacturer; }

  // Setters
  void setUrl(const QUrl& url) noexcept { mUrl = url; }
  void setDependencies(const QSet<Uuid>& deps) noexcept {
    mDependencies = deps;
  }
  void setIcon(const QByteArray& png) noexcept { mIcon = png; }
  void setManufacturer(const SimpleString& value) noexcept {
    if (value != mManufacturer) {
      mManufacturer = value;
      emit manufacturerChanged(mManufacturer);
    }
  }

  // General Methods
  virtual void save() override;
  virtual void moveTo(TransactionalDirectory& dest) override;
  template <typename ElementType>
  QStringList searchForElements() const noexcept;

  // Operator Overloadings
  Library& operator=(const Library& rhs) = delete;

  // Static Methods
  static std::unique_ptr<Library> open(
      std::unique_ptr<TransactionalDirectory> directory,
      bool abortBeforeMigration = false);
  static QString getShortElementName() noexcept {
    return QStringLiteral("lib");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("library");
  }

signals:
  void manufacturerChanged(const SimpleString& mfr);

protected:
  virtual void serialize(SExpression& root) const override;

private:  // Methods
  Library(std::unique_ptr<TransactionalDirectory> directory,
          const SExpression& root);

private:  // Data
  QUrl mUrl;
  QSet<Uuid> mDependencies;
  QByteArray mIcon;
  SimpleString mManufacturer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
