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

#ifndef LIBREPCB_CORE_CORPORATEPCBPRODUCT_H
#define LIBREPCB_CORE_CORPORATEPCBPRODUCT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../project/board/drc/boarddesignrulechecksettings.h"
#include "../../serialization/serializablekeyvaluemap.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class CorporatePcbProduct
 ******************************************************************************/

/**
 * @brief The CorporatePcbProduct class
 */
class CorporatePcbProduct final {
public:
  // Constructors / Destructor
  CorporatePcbProduct() noexcept = delete;
  CorporatePcbProduct(const CorporatePcbProduct& other) noexcept;
  explicit CorporatePcbProduct(const SExpression& node);
  ~CorporatePcbProduct() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const LocalizedNameMap& getNames() const noexcept { return mNames; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }
  const QUrl& getUrl() const noexcept { return mUrl; }
  BoardDesignRuleCheckSettings getDrcSettings() const noexcept;

  // Setters
  void setNames(const LocalizedNameMap& names) noexcept { mNames = names; }
  void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept {
    mDescriptions = descriptions;
  }
  void setDrcSettings(const BoardDesignRuleCheckSettings& s) noexcept {
    mDrcSettings = s;
  }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  CorporatePcbProduct& operator=(const CorporatePcbProduct& rhs) noexcept;

private:  // Data
  // Attributes
  Uuid mUuid;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  QUrl mUrl;

  // This dependency from the "library" sources to "project" sources is actually
  // violating our software architecture. It's not critical, but it would be
  // better to move the BoardDesignRuleCheckSettings class into the common
  // sources so both "library" and "project" sources can depend on it.
  BoardDesignRuleCheckSettings mDrcSettings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
