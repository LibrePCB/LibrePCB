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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "clientsettings.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static constexpr std::array sZoneRuleKeys = {
    std::pair{Zone::Rule::NoCopper, "no_copper"},
    std::pair{Zone::Rule::NoPlanes, "no_planes"},
    std::pair{Zone::Rule::NoExposure, "no_exposure"},
    std::pair{Zone::Rule::NoDevices, "no_devices"},
};
static_assert(static_cast<int>(sZoneRuleKeys.size()) ==
              std::popcount(static_cast<uint>(Zone::Rule::All)));

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ClientSettings::ClientSettings() noexcept : mSettings() {
}

ClientSettings::ClientSettings(
    std::initializer_list<QAnyStringView> groups) noexcept
  : mSettings() {
  for (QAnyStringView group : groups) {
    mSettings.beginGroup(group);
  }
}

ClientSettings::~ClientSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ClientSettings::set(QAnyStringView key, Zone::Rules value) noexcept {
  for (const auto& [rule, ruleKey] : sZoneRuleKeys) {
    set(key.toString() % "/" % ruleKey, value.testFlag(rule));
  }
}

std::optional<Angle> ClientSettings::get(
    QAnyStringView key, const std::optional<Angle>& min,
    const std::optional<Angle>& max,
    const std::optional<Angle>& modulo) const noexcept {
  const std::optional<Angle> v = get<Angle>(key);
  if (v && ((!min) || (*v >= *min)) && ((!max) || (*v <= *max)) &&
      ((!modulo) || ((*v % *modulo) == 0))) {
    return v;
  }
  return std::nullopt;
}

std::optional<const Layer*> ClientSettings::get(
    QAnyStringView key,
    const std::optional<QSet<const Layer*>>& validSet) const noexcept {
  const std::optional<const Layer*> v = get<const Layer*>(key);
  if (v && (*v) && ((!validSet) || validSet->contains(*v))) {
    return v;
  }
  return std::nullopt;
}

std::optional<Zone::Rules> ClientSettings::get(
    QAnyStringView key, Zone::Rules defaultFlags) const noexcept {
  Zone::Rules r;
  bool loaded = false;
  for (const auto& [rule, ruleKey] : sZoneRuleKeys) {
    if (auto v = get<bool>(key.toString() % "/" % ruleKey)) {
      r.setFlag(rule, *v);
      loaded = true;
    } else {
      r.setFlag(rule, defaultFlags.testFlag(rule));
    }
  }
  return loaded ? std::make_optional(r) : std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
