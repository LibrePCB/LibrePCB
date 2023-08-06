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

#ifndef LIBREPCB_CORE_ELECTRICALRULECHECK_H
#define LIBREPCB_CORE_ELECTRICALRULECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../rulecheck/rulecheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class NetSignal;
class Project;
class SI_NetSegment;
class SI_Symbol;
class Schematic;

/*******************************************************************************
 *  Class ElectricalRuleCheck
 ******************************************************************************/

/**
 * @brief The ElectricalRuleCheck class checks a ::librepcb::Board for
 *        design rule violations
 */
class ElectricalRuleCheck final {
public:
  // Constructors / Destructor
  explicit ElectricalRuleCheck(const Project& project) noexcept;
  ~ElectricalRuleCheck() noexcept;

  // General Methods
  RuleCheckMessageList runChecks() const;

private:  // Methods
  void checkNetClasses(RuleCheckMessageList& msgs) const;
  void checkNetSignals(RuleCheckMessageList& msgs) const;
  void checkComponents(RuleCheckMessageList& msgs) const;
  void checkComponentSignals(const ComponentInstance& cmp,
                             RuleCheckMessageList& msgs) const;
  void checkSchematics(RuleCheckMessageList& msgs) const;
  void checkSymbols(const Schematic& schematic,
                    RuleCheckMessageList& msgs) const;
  void checkPins(const SI_Symbol& symbol, RuleCheckMessageList& msgs) const;
  void checkNetSegments(const Schematic& schematic,
                        RuleCheckMessageList& msgs) const;
  void checkNetPoints(const SI_NetSegment& netSegment,
                      RuleCheckMessageList& msgs) const;

private:  // Data
  const Project& mProject;
  mutable QSet<const NetSignal*> mOpenNetSignals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
