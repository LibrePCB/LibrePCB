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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
