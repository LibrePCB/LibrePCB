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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
