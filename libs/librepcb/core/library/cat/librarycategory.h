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

#ifndef LIBREPCB_CORE_LIBRARYCATEGORY_H
#define LIBREPCB_CORE_LIBRARYCATEGORY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../librarybaseelement.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LibraryCategory
 ******************************************************************************/

/**
 * @brief The LibraryCategory class extends the LibraryBaseElement class with
 * some attributes and methods which are used for all library category classes.
 */
class LibraryCategory : public LibraryBaseElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryCategory() = delete;
  LibraryCategory(const LibraryCategory& other) = delete;
  LibraryCategory(const QString& shortElementName,
                  const QString& longElementName, const Uuid& uuid,
                  const Version& version, const QString& author,
                  const ElementName& name_en_US,
                  const QString& description_en_US,
                  const QString& keywords_en_US);
  LibraryCategory(const QString& shortElementName,
                  const QString& longElementName,
                  std::unique_ptr<TransactionalDirectory> directory,
                  const SExpression& root);
  virtual ~LibraryCategory() noexcept;

  // Getters: Attributes
  const std::optional<Uuid>& getParentUuid() const noexcept {
    return mParentUuid;
  }

  // Setters: Attributes
  void setParentUuid(const std::optional<Uuid>& parentUuid) noexcept {
    mParentUuid = parentUuid;
  }

  // Operator Overloadings
  LibraryCategory& operator=(const LibraryCategory& rhs) = delete;

protected:
  virtual void serialize(SExpression& root) const override;

  std::optional<Uuid> mParentUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
