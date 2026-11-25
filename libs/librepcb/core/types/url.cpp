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
#include "url.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/serialization/sexpression.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Url Url::fromString(const QString& str, QUrl::ParsingMode mode,
                    bool canonicalize) {
  if (auto obj = tryFromString(str, mode, canonicalize)) {
    return *obj;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid URL: '%1'").arg(str));
  }
}

std::optional<Url> Url::tryFromString(const QString& str,
                                      QUrl::ParsingMode mode,
                                      bool canonicalize) noexcept {
  if (str.isEmpty()) {
    return std::nullopt;
  }
  // Note: In strict mode, the string must already be fully encoded. Since
  // QUrl::StrictMode still accepts some not fully encoded characters (e.g.
  // unencoded non-ASCII characters), this needs to be checked explicitly by
  // comparing the string with its fully encoded form.
  const QString encoded = (canonicalize || (mode == QUrl::StrictMode))
      ? QUrl(str, mode).toString(QUrl::FullyEncoded)  // Canonicalize string.
      : str;  // Keep original string.
  const QUrl obj(encoded, canonicalize ? QUrl::StrictMode : mode);
  if ((!obj.isValid()) || ((mode == QUrl::StrictMode) && (encoded != str))) {
    return std::nullopt;
  }
  return Url(encoded, obj);
}

std::optional<Url> Url::tryFromUserInput(const QString& str) noexcept {
  const QString trimmed = str.trimmed();
  if (trimmed.isEmpty()) {
    return std::nullopt;
  }
  // Converting the string to a fully encoded URL.
  return tryFromString(
      QUrl::fromUserInput(trimmed).toString(QUrl::FullyEncoded),
      QUrl::StrictMode, false);
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const Url& obj) {
  return SExpression::createString(obj.toStr());
}

template <>
std::unique_ptr<SExpression> serialize(const std::optional<Url>& obj) {
  return SExpression::createString(obj ? obj->toStr() : "");
}

template <>
Url deserialize(const SExpression& node) {
  // IMPORTANT: Do *not* canonicalize since that would not be round-trip safe!
  // NOTE: Actually we want to parse in strict mode, but that is currently not
  // possible since we serialized in PrettyDecoded mode in earlier LibrePCB
  // versions, so those URLs may not be successfully parsed back in strict mode.
  return Url::fromString(node.getValue(), QUrl::TolerantMode,
                         false);  // can throw
}

template <>
std::optional<Url> deserialize(const SExpression& node) {
  // IMPORTANT: Do *not* canonicalize since that would not be round-trip safe!
  // NOTE: Actually we want to parse in strict mode, but that is currently not
  // possible since we serialized in PrettyDecoded mode in earlier LibrePCB
  // versions, so those URLs may not be successfully parsed back in strict mode.
  const QString str = node.getValue();
  return str.isEmpty() ? std::nullopt
                       : std::make_optional(Url::fromString(
                             str, QUrl::TolerantMode, false));  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
