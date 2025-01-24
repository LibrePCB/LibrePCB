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
#include "apptoolbox.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

slint::PhysicalPosition q2s(const QPoint& p) noexcept {
  return slint::PhysicalPosition({p.x(), p.y()});
}

slint::PhysicalSize q2s(const QSize& s) noexcept {
  return slint::PhysicalSize({static_cast<uint32_t>(std::max(s.width(), 0)),
                              static_cast<uint32_t>(std::max(s.height(), 0))});
}

slint::SharedString q2s(const QString& s) noexcept {
  return slint::SharedString(s.toUtf8().data());
}

slint::Image q2s(const QPixmap& p) noexcept {
  if (p.isNull()) {
    return slint::Image();
  }

  QImage img = p.toImage();
  img.convertTo(QImage::Format_RGBA8888);
  return slint::Image(slint::SharedPixelBuffer<slint::Rgba8Pixel>(
      img.width(), img.height(),
      reinterpret_cast<const slint::Rgba8Pixel*>(img.bits())));
}

slint::Color q2s(const QColor& c) noexcept {
  return slint::Color::from_argb_uint8(c.alpha(), c.red(), c.green(), c.blue());
}

QPoint s2q(const slint::PhysicalPosition& p) noexcept {
  return QPoint(p.x, p.y);
}

QSize s2q(const slint::PhysicalSize& s) noexcept {
  return QSize(s.width, s.height);
}

QString s2q(const slint::SharedString& s) noexcept {
  std::string_view view(s);
  return QString::fromUtf8(view.data(), view.size());
}

bool operator==(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() == std::string_view(s2);
}

bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() != std::string_view(s2);
}

bool operator==(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) == s2.toUtf8().data();
}

bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) != s2.toUtf8().data();
}

static slint::SharedString getError(const QString& input) {
  if (input.trimmed().isEmpty()) {
    return q2s(QCoreApplication::translate("AppToolbox", "Required"));
  } else {
    return q2s(QCoreApplication::translate("AppToolbox", "Invalid"));
  }
}

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept {
  const std::optional<ElementName> ret =
      parseElementName(cleanElementName(input));
  if (!ret) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept {
  const std::optional<Version> ret = Version::tryFromString(input.trimmed());
  if (!ret) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix) noexcept {
  std::optional<FileProofName> ret =
      parseFileProofName(cleanFileProofName(input));
  if (!ret) {
    error = getError(input);
  } else if ((!requiredSuffix.isEmpty()) &&
             (!input.trimmed().endsWith(requiredSuffix))) {
    ret = std::nullopt;
    error = q2s(QCoreApplication::translate("AppToolbox", "Suffix '%1' missing")
                    .arg(requiredSuffix));
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty) noexcept {
  const QUrl url = QUrl::fromUserInput(input.trimmed());
  const std::optional<QUrl> ret =
      url.isValid() ? std::make_optional(url) : std::nullopt;
  if ((!ret) && ((!input.isEmpty()) || (!allowEmpty))) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
