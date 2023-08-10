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

#ifndef LIBREPCB_CORE_LIBRARYELEMENT_H
#define LIBREPCB_CORE_LIBRARYELEMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "librarybaseelement.h"
#include "resource.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LibraryElement
 ******************************************************************************/

/**
 * @brief The LibraryElement class extends the LibraryBaseElement class with
 * some attributes and methods which are used for all library classes except
 * categories.
 */
class LibraryElement : public LibraryBaseElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryElement() = delete;
  LibraryElement(const LibraryElement& other) = delete;
  LibraryElement(const QString& shortElementName,
                 const QString& longElementName, const Uuid& uuid,
                 const Version& version, const QString& author,
                 const ElementName& name_en_US,
                 const QString& description_en_US,
                 const QString& keywords_en_US);
  LibraryElement(const QString& shortElementName,
                 const QString& longElementName, bool dirnameMustBeUuid,
                 std::unique_ptr<TransactionalDirectory> directory,
                 const SExpression& root);
  virtual ~LibraryElement() noexcept;

  // Getters
  const QString& getGeneratedBy() const noexcept { return mGeneratedBy; }
  const QSet<Uuid>& getCategories() const noexcept { return mCategories; }
  const ResourceList& getResources() const noexcept { return mResources; }

  // Setters
  void setGeneratedBy(const QString& gen) noexcept { mGeneratedBy = gen; }
  void setCategories(const QSet<Uuid>& uuids) noexcept { mCategories = uuids; }
  void setResources(const ResourceList& resources) noexcept {
    mResources = resources;
  }

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  LibraryElement& operator=(const LibraryElement& rhs) = delete;

protected:
  virtual void serialize(SExpression& root) const override;

  QString mGeneratedBy;  ///< If not empty, the element is generated.
  QSet<Uuid> mCategories;
  ResourceList mResources;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
