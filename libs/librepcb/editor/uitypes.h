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

#ifndef LIBREPCB_EDITOR_UITYPES_H
#define LIBREPCB_EDITOR_UITYPES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/theme.h>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

static_assert(sizeof(ui::Int64) == 8);
static_assert(sizeof(LengthBase_t) == 8);

inline qint64 s2l(const ui::Int64& v) noexcept {
  return (static_cast<int64_t>(v.msb) << 32) | static_cast<uint32_t>(v.lsb);
}

inline Length s2length(const ui::Int64& v) noexcept {
  return Length(s2l(v));
}

inline std::optional<UnsignedLength> s2ulength(const ui::Int64& v) noexcept {
  const Length l = s2length(v);
  return (l >= 0) ? std::make_optional(UnsignedLength(l)) : std::nullopt;
}

inline std::optional<PositiveLength> s2plength(const ui::Int64& v) noexcept {
  const Length l = s2length(v);
  return (l > 0) ? std::make_optional(PositiveLength(l)) : std::nullopt;
}

inline ui::Int64 l2s(const Length& v) noexcept {
  return ui::Int64{
      static_cast<int>((v.toNm() >> 32) & 0xFFFFFFFF),
      static_cast<int>(v.toNm() & 0xFFFFFFFF),
  };
}

inline Theme::GridStyle s2l(ui::GridStyle v) noexcept {
  if (v == ui::GridStyle::Lines) {
    return Theme::GridStyle::Lines;
  } else if (v == ui::GridStyle::Dots) {
    return Theme::GridStyle::Dots;
  } else {
    return Theme::GridStyle::None;
  }
}

inline ui::GridStyle l2s(Theme::GridStyle v) noexcept {
  if (v == Theme::GridStyle::Lines) {
    return ui::GridStyle::Lines;
  } else if (v == Theme::GridStyle::Dots) {
    return ui::GridStyle::Dots;
  } else {
    return ui::GridStyle::None;
  }
}

inline LengthUnit s2l(ui::LengthUnit v) noexcept {
  if (v == ui::LengthUnit::Millimeters) {
    return LengthUnit::millimeters();
  } else if (v == ui::LengthUnit::Micrometers) {
    return LengthUnit::micrometers();
  } else if (v == ui::LengthUnit::Nanometers) {
    return LengthUnit::nanometers();
  } else if (v == ui::LengthUnit::Inches) {
    return LengthUnit::inches();
  } else if (v == ui::LengthUnit::Mils) {
    return LengthUnit::mils();
  } else {
    qCritical() << "Unhandled value in LengthUnit conversion.";
    return LengthUnit::millimeters();
  }
}

inline ui::LengthUnit l2s(const LengthUnit& v) noexcept {
  if (v == LengthUnit::millimeters()) {
    return ui::LengthUnit::Millimeters;
  } else if (v == LengthUnit::micrometers()) {
    return ui::LengthUnit::Micrometers;
  } else if (v == LengthUnit::nanometers()) {
    return ui::LengthUnit::Nanometers;
  } else if (v == LengthUnit::inches()) {
    return ui::LengthUnit::Inches;
  } else if (v == LengthUnit::mils()) {
    return ui::LengthUnit::Mils;
  } else {
    qCritical() << "Unhandled value in LengthUnit conversion.";
    return ui::LengthUnit::Millimeters;
  }
}

inline ui::NotificationType l2s(RuleCheckMessage::Severity v) noexcept {
  if (v == RuleCheckMessage::Severity::Hint) {
    return ui::NotificationType::Info;
  } else if (v == RuleCheckMessage::Severity::Warning) {
    return ui::NotificationType::Warning;
  } else if (v == RuleCheckMessage::Severity::Error) {
    return ui::NotificationType::Critical;
  } else {
    qCritical() << "Unhandled value in RuleCheckMessage::Severity conversion.";
    return ui::NotificationType::Critical;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
