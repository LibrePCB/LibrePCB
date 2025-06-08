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
#include "uihelpers.h"

#include "../editorcommand.h"
#include "slinthelpers.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

qint64 s2l(const ui::Int64& v) noexcept {
  return (static_cast<int64_t>(v.msb) << 32) | static_cast<uint32_t>(v.lsb);
}

ui::Int64 l2s(const Length& v) noexcept {
  return ui::Int64{
      static_cast<int>((v.toNm() >> 32) & 0xFFFFFFFF),
      static_cast<int>(v.toNm() & 0xFFFFFFFF),
  };
}

Length s2length(const ui::Int64& v) noexcept {
  return Length(s2l(v));
}

std::optional<UnsignedLength> s2ulength(const ui::Int64& v) noexcept {
  const Length l = s2length(v);
  return (l >= 0) ? std::make_optional(UnsignedLength(l)) : std::nullopt;
}

std::optional<PositiveLength> s2plength(const ui::Int64& v) noexcept {
  const Length l = s2length(v);
  return (l > 0) ? std::make_optional(PositiveLength(l)) : std::nullopt;
}

ui::GridStyle l2s(Theme::GridStyle v) noexcept {
  switch (v) {
    case Theme::GridStyle::Lines:
      return ui::GridStyle::Lines;
    case Theme::GridStyle::Dots:
      return ui::GridStyle::Dots;
    case Theme::GridStyle::None:
      return ui::GridStyle::None;
    default:
      qCritical() << "Unhandled value in GridStyle conversion.";
      return ui::GridStyle::None;
  }
}

Theme::GridStyle s2l(ui::GridStyle v) noexcept {
  switch (v) {
    case ui::GridStyle::Lines:
      return Theme::GridStyle::Lines;
    case ui::GridStyle::Dots:
      return Theme::GridStyle::Dots;
    case ui::GridStyle::None:
      return Theme::GridStyle::None;
    default:
      qCritical() << "Unhandled value in GridStyle conversion.";
      return Theme::GridStyle::None;
  }
}

ui::LengthUnit l2s(const LengthUnit& v) noexcept {
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

LengthUnit s2l(ui::LengthUnit v) noexcept {
  switch (v) {
    case ui::LengthUnit::Millimeters:
      return LengthUnit::millimeters();
    case ui::LengthUnit::Micrometers:
      return LengthUnit::micrometers();
    case ui::LengthUnit::Nanometers:
      return LengthUnit::nanometers();
    case ui::LengthUnit::Inches:
      return LengthUnit::inches();
    case ui::LengthUnit::Mils:
      return LengthUnit::mils();
    default:
      qCritical() << "Unhandled value in LengthUnit conversion.";
      return LengthUnit::millimeters();
  }
}

ui::NotificationType l2s(RuleCheckMessage::Severity v) noexcept {
  switch (v) {
    case RuleCheckMessage::Severity::Hint:
      return ui::NotificationType::Info;
    case RuleCheckMessage::Severity::Warning:
      return ui::NotificationType::Warning;
    case RuleCheckMessage::Severity::Error:
      return ui::NotificationType::Critical;
    default:
      qCritical()
          << "Unhandled value in RuleCheckMessage::Severity conversion.";
      return ui::NotificationType::Critical;
  }
}

ui::EditorCommand l2s(const EditorCommand& cmd, ui::EditorCommand in) noexcept {
  QString text = cmd.getDisplayText();
  if (cmd.getFlags().testFlag(EditorCommand::Flag::OpensPopup)) {
    text += "...";
  }
  in.text = q2s(text);
  in.status_tip = q2s(cmd.getDescription());
  const QKeySequence shortcut = cmd.getKeySequences().value(0);
  if (shortcut.count() == 1) {
    in.shortcut = q2s(shortcut.toString());
    in.modifiers = q2s(shortcut[0].keyboardModifiers());
    in.key = q2s(shortcut[0].key());
  } else {
    // Multi-combination shortcuts are not supported yet.
    in.shortcut = slint::SharedString();
    in.modifiers = slint::private_api::KeyboardModifiers{};
    in.key = slint::SharedString();
  }
  return in;
}

ui::FeatureState toFs(bool enabled) noexcept {
  return enabled ? ui::FeatureState::Enabled : ui::FeatureState::Disabled;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
