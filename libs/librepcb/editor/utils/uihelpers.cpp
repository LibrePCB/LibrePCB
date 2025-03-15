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
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

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
    error =
        q2s(QCoreApplication::translate("FileProofName", "Suffix '%1' missing")
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

}  // namespace editor
}  // namespace librepcb
