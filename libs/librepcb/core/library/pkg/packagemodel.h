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

#ifndef LIBREPCB_CORE_PACKAGEMODEL_H
#define LIBREPCB_CORE_PACKAGEMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/serializableobjectlist.h"
#include "../../types/elementname.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PackageModel
 ******************************************************************************/

/**
 * @brief Represents a 3D model of a ::librepcb::Package
 */
class PackageModel final {
  Q_DECLARE_TR_FUNCTIONS(PackageModel)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
  };
  Signal<PackageModel, Event> onEdited;
  typedef Slot<PackageModel, Event> OnEditedSlot;

  // Constructors / Destructor
  PackageModel() = delete;
  PackageModel(const PackageModel& other) noexcept;
  PackageModel(const Uuid& uuid, const ElementName& name) noexcept;
  explicit PackageModel(const SExpression& node);
  ~PackageModel() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  QString getFileName() const noexcept { return mUuid.toStr() % ".step"; }

  // Setters
  bool setName(const ElementName& name) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const PackageModel& rhs) const noexcept;
  bool operator!=(const PackageModel& rhs) const noexcept {
    return !(*this == rhs);
  }
  PackageModel& operator=(const PackageModel& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  ElementName mName;
};

/*******************************************************************************
 *  Class PackageModelList
 ******************************************************************************/

struct PackageModelListNameProvider {
  static constexpr const char* tagname = "3d_model";
};
using PackageModelList =
    SerializableObjectList<PackageModel, PackageModelListNameProvider,
                           PackageModel::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
