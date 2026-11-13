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
#include "enums.h"

#include "../exceptions.h"
#include "../serialization/serialization.h"
#include "../serialization/sexpression.h"

#include <QString>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Serialization
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const AutoUpdateMode& obj) {
  switch (obj) {
    case AutoUpdateMode::Disabled:
      return SExpression::createToken("disabled");
    case AutoUpdateMode::Check:
      return SExpression::createToken("check");
    case AutoUpdateMode::Notify:
      return SExpression::createToken("notify");
    case AutoUpdateMode::Install:
      return SExpression::createToken("install");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
AutoUpdateMode deserialize(const SExpression& sexpr) {
  const QString str = sexpr.getValue();
  if (str == "disabled") {
    return AutoUpdateMode::Disabled;
  } else if (str == "check") {
    return AutoUpdateMode::Check;
  } else if (str == "notify") {
    return AutoUpdateMode::Notify;
  } else if (str == "install") {
    return AutoUpdateMode::Install;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown auto update mode: '%1'").arg(str));
  }
}

template <>
std::unique_ptr<SExpression> serialize(const GridStyle& obj) {
  switch (obj) {
    case GridStyle::None:
      return SExpression::createToken("none");
    case GridStyle::Dots:
      return SExpression::createToken("dots");
    case GridStyle::Lines:
      return SExpression::createToken("lines");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
GridStyle deserialize(const SExpression& sexpr) {
  const QString str = sexpr.getValue();
  if (str == "none") {
    return GridStyle::None;
  } else if (str == "dots") {
    return GridStyle::Dots;
  } else if (str == "lines") {
    return GridStyle::Lines;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown grid style: '%1'").arg(str));
  }
}

template <>
QString serializeToString(const WireMode& obj) noexcept {
  switch (obj) {
    case WireMode::HV:
      return "h_v";
    case WireMode::VH:
      return "v_h";
    case WireMode::Deg9045:
      return "90_45";
    case WireMode::Deg4590:
      return "45_90";
    case WireMode::Straight:
      return "straight";
    default:
      qCritical() << "Unknown wire mode:" << static_cast<int>(obj);
      Q_ASSERT(false);
      return QString();
  }
}

template <>
std::optional<WireMode> deserialize(const QString& str) noexcept {
  if (str == "h_v") {
    return WireMode::HV;
  } else if (str == "v_h") {
    return WireMode::VH;
  } else if (str == "90_45") {
    return WireMode::Deg9045;
  } else if (str == "45_90") {
    return WireMode::Deg4590;
  } else if (str == "straight") {
    return WireMode::Straight;
  } else {
    return std::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
