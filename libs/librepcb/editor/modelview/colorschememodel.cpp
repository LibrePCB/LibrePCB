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
#include "colorschememodel.h"

#include "../utils/slinthelpers.h"

#include <librepcb/core/workspace/colorrole.h>
#include <librepcb/core/workspace/usercolorscheme.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ColorSchemeModel::ColorSchemeModel(
    std::shared_ptr<UserColorScheme> scheme) noexcept
  : mScheme(scheme) {
}

ColorSchemeModel::~ColorSchemeModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ColorSchemeModel::row_count() const {
  return mScheme->getBase().getAllColors().count();
}

static QString toHexRgba(const QColor& c) noexcept {
  QString s = c.name(QColor::HexRgb);
  if (c.alpha() < 255) {
    if (c.alpha() < 0x10) {
      s += "0";
    }
    s += QString::number(c.alpha(), 16);
  }
  return s;
}

static std::optional<QColor> fromHexRgba(QString s) {
  if (s.length() == 7) {
    s.append("ff");
  }
  if ((!s.startsWith('#')) || (s.length() != 9)) {
    return std::nullopt;
  }
  bool ok;
  const uint value = s.mid(1).toUInt(&ok, 16);
  if (!ok) {
    return std::nullopt;
  }
  return QColor((value >> 24) & 0xFF,  // R
                (value >> 16) & 0xFF,  // G
                (value >> 8) & 0xFF,  // B
                (value >> 0) & 0xFF  // A
  );
}

static ui::ColorSchemeColorData toData(const QColor& c,
                                       const QColor& override) noexcept {
  return ui::ColorSchemeColorData{
      qBound(0.0f, c.hueF(), 1.0f),  // Returns -1 for grayscale colors.
      c.saturationF(),
      c.valueF(),
      c.alphaF(),
      q2s(toHexRgba(c)),
      override.isValid(),
      ui::ColorSchemeColorAction::None,
  };
}

static std::optional<QColor> toColor(
    const ui::ColorSchemeColorData& d) noexcept {
  if (d.action == ui::ColorSchemeColorAction::SetHsv) {
    return QColor::fromHsvF(d.hue, d.saturation, d.value, d.alpha);
  } else if (d.action == ui::ColorSchemeColorAction::SetHex) {
    return fromHexRgba(s2q(d.hex).trimmed());
  } else if (d.action == ui::ColorSchemeColorAction::RestoreDefault) {
    return QColor();
  } else {
    return std::nullopt;
  }
}

std::optional<ui::ColorSchemeItemData> ColorSchemeModel::row_data(
    std::size_t i) const {
  const auto baseColors = mScheme->getBase().getAllColors().value(i);
  const ColorRole* role = baseColors.role;
  if (!role) return std::nullopt;
  const auto userColors = mScheme->getColors(*role);
  return ui::ColorSchemeItemData{
      q2s(role->getNameTr()),
      toData(userColors.primary, mScheme->getPrimaryOverride(role->getId())),
      toData(userColors.secondary,
             mScheme->getSecondaryOverride(role->getId())),
  };
}

void ColorSchemeModel::set_row_data(
    std::size_t i, const ui::ColorSchemeItemData& data) noexcept {
  const auto baseColors = mScheme->getBase().getAllColors().value(i);
  if (!baseColors.role) return;
  if (auto color = toColor(data.primary)) {
    mScheme->setPrimary(baseColors.role->getId(), *color);
  }
  if (auto color = toColor(data.secondary)) {
    mScheme->setSecondary(baseColors.role->getId(), *color);
  }
  notify_row_changed(i);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
