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

#ifndef LIBREPCB_CORE_ASSEMBLYVARIANT_H
#define LIBREPCB_CORE_ASSEMBLYVARIANT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializableobjectlist.h"
#include "../../types/fileproofname.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AssemblyVariant
 ******************************************************************************/

/**
 * @brief The AssemblyVariant class
 */
class AssemblyVariant final {
public:
  // Signals
  enum class Event {
    NameChanged,
    DescriptionChanged,
  };
  Signal<AssemblyVariant, Event> onEdited;
  typedef Slot<AssemblyVariant, Event> OnEditedSlot;

  // Constructors / Destructor
  AssemblyVariant() = delete;
  AssemblyVariant(const AssemblyVariant& other) noexcept;
  explicit AssemblyVariant(const SExpression& node);
  AssemblyVariant(const Uuid& uuid, const FileProofName& name,
                  const QString& description);
  ~AssemblyVariant() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const FileProofName& getName() const noexcept { return mName; }
  const QString& getDescription() const noexcept { return mDescription; }
  QString getDisplayText() const noexcept;

  // Setters
  void setName(const FileProofName& name) noexcept;
  void setDescription(const QString& description) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  AssemblyVariant& operator=(const AssemblyVariant& rhs) = delete;
  bool operator==(const AssemblyVariant& rhs) = delete;

private:
  Uuid mUuid;
  FileProofName mName;
  QString mDescription;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const std::shared_ptr<AssemblyVariant>& key,
                  uint seed = 0) noexcept {
  return ::qHash(key.get(), seed);
}

/*******************************************************************************
 *  Class AssemblyVariantList
 ******************************************************************************/

struct AssemblyVariantListNameProvider {
  static constexpr const char* tagname = "variant";
};
using AssemblyVariantList =
    SerializableObjectList<AssemblyVariant, AssemblyVariantListNameProvider,
                           AssemblyVariant::Event>;

}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
