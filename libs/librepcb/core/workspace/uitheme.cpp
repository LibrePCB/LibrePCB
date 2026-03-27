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
#include "uitheme.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

QString UiTheme::getNameTr() const noexcept {
  return QCoreApplication::translate("UiTheme", name);
}

// Convert Slint Theme to C++ UiTheme with Python:
// print("\n".join([f"\"{b[:-1]}\",  // {a[:-1]}" for l in INPUT.splitlines()
//       for a, b, *_ in [l.strip().split(" ")]]))

const UiTheme& UiTheme::light() noexcept {
  auto createQPalette = []() {
    QPalette p(QColor("#fafafa"), QColor("#ffffff"));
    p.setColor(QPalette::Window, "#e0e0e0");
    p.setColor(QPalette::WindowText, "#292929");
    p.setColor(QPalette::Base, "#f0f0f0");
    p.setColor(QPalette::AlternateBase, "#f0f0f0");
    p.setColor(QPalette::ToolTipBase, "#fffbc5");
    p.setColor(QPalette::ToolTipText, "#3a3a3a");
    p.setColor(QPalette::Text, "#292929");
    p.setColor(QPalette::PlaceholderText, "#808080");
    p.setColor(QPalette::Button, "#ffffff");
    p.setColor(QPalette::ButtonText, "#292929");
    p.setColor(QPalette::Link, "#1a73e8");
    p.setColor(QPalette::LinkVisited, "#1a73e8");
    p.setColor(QPalette::Highlight, "#29d682");
    p.setColor(QPalette::HighlightedText, "#161616");
    p.setColor(QPalette::Light, "#e0e0e0");
    p.setColor(QPalette::Dark, "#c0c0c0");
    p.setColor(QPalette::Disabled, QPalette::Button, "#d0d0d0");
    p.setColor(QPalette::Disabled, QPalette::ButtonText, "#b0b0b0");
    p.setColor(QPalette::Disabled, QPalette::WindowText, "#b0b0b0");
    p.setColor(QPalette::Disabled, QPalette::Text, "#b0b0b0");
    p.setColor(QPalette::Disabled, QPalette::Light, "#b0b0b0");
    return p;
  };
  static const UiTheme theme = {
      "light",  // theme id
      QT_TR_NOOP("Light"),  // theme name
      createQPalette(),  // QPalette
      "#f0f0f0",  // window
      "#ffffff",  // base
      "#d0d0d0",  // base-border
      "#9e9e9e",  // base-text-disabled
      "#7c7c7c",  // base-text-muted
      "#292929",  // base-text
      "#000000",  // base-text-hovered
      "#0059ff",  // base-text-info
      "#008f00",  // base-text-success
      "#e24800",  // base-text-warning
      "#ff0000",  // base-text-error
      "#fafafa",  // control
      "#e4e4e4",  // control-disabled
      "#e8e8e8",  // control-hovered
      "#d0d0d0",  // control-checked
      "#c0c0c0",  // control-border
      "#dddddd",  // control-border-disabled
      "#9e9e9e",  // control-text-disabled
      "#7c7c7c",  // control-text-muted
      "#000000",  // control-text
      "#29d682",  // selection
      "#000000",  // selection-text
      "#fffbc5",  // tooltip
      "#b9b9b9",  // tooltip-border
      "#3a3a3a",  // tooltip-text
      "#29d682",  // accent
      "#000000",  // accent-text
      "#00ccff",  // info
      "#000000",  // info-text
      "#ffe659",  // warning
      "#000000",  // warning-text
      "#ff3b3b",  // error
      "#000000",  // error-text
  };
  return theme;
}

const UiTheme& UiTheme::dark() noexcept {
  auto createQPalette = []() {
    QPalette p(QColor("#202020"), QColor("#2a2a2a"));
    p.setColor(QPalette::Window, "#2a2a2a");
    p.setColor(QPalette::WindowText, "#c4c4c4");
    p.setColor(QPalette::Base, "#353535");
    p.setColor(QPalette::AlternateBase, "#2e2e2e");
    p.setColor(QPalette::ToolTipBase, "#1a1a1a");
    p.setColor(QPalette::ToolTipText, "#d8d8d8");
    p.setColor(QPalette::Text, "#c4c4c4");
    p.setColor(QPalette::PlaceholderText, "#909090");
    p.setColor(QPalette::Button, "#202020");
    p.setColor(QPalette::ButtonText, "#c4c4c4");
    p.setColor(QPalette::Link, "#29d682");
    p.setColor(QPalette::LinkVisited, "#29d682");
    p.setColor(QPalette::Highlight, "#29d682");
    p.setColor(QPalette::HighlightedText, "#161616");
    p.setColor(QPalette::Light, "#505050");
    p.setColor(QPalette::Dark, "#000000");
    p.setColor(QPalette::Disabled, QPalette::Button, "#1a1a1a");
    p.setColor(QPalette::Disabled, QPalette::ButtonText, "#707070");
    p.setColor(QPalette::Disabled, QPalette::WindowText, "#707070");
    p.setColor(QPalette::Disabled, QPalette::Text, "#707070");
    p.setColor(QPalette::Disabled, QPalette::Light, "#707070");
    return p;
  };
  static const UiTheme theme = {
      "dark",  // theme id
      QT_TR_NOOP("Dark"),  // theme name
      createQPalette(),  // QPalette
      "#353535",  // window
      "#2a2a2a",  // base
      "#505050",  // base-border
      "#808080",  // base-text-disabled
      "#adadad",  // base-text-muted
      "#c4c4c4",  // base-text
      "#e0e0e0",  // base-text-hovered
      "#00ccff",  // base-text-info
      "#00ff00",  // base-text-success
      "#ffff00",  // base-text-warning
      "#ff2020",  // base-text-error
      "#303030",  // control
      "#1a1a1a",  // control-disabled
      "#404040",  // control-hovered
      "#505050",  // control-checked
      "#606060",  // control-border
      "#414141",  // control-border-disabled
      "#707070",  // control-text-disabled
      "#909090",  // control-text-muted
      "#c4c4c4",  // control-text
      "#29d682",  // selection
      "#000000",  // selection-text
      "#fffbc5",  // tooltip
      "#535353",  // tooltip-border
      "#3a3a3a",  // tooltip-text
      "#29d682",  // accent
      "#000000",  // accent-text
      "#00ccff",  // info
      "#000000",  // info-text
      "#ffe659",  // warning
      "#000000",  // warning-text
      "#ff4343",  // error
      "#000000",  // error-text
  };
  return theme;
}

const UiTheme* UiTheme::find(const QString& id) noexcept {
  for (auto theme : all()) {
    if (theme->id == id) {
      return theme;
    }
  }
  return nullptr;
}

QVector<const UiTheme*> UiTheme::all() noexcept {
  return {
      &light(),
      &dark(),
  };
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
