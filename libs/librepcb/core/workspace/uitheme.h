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

#ifndef LIBREPCB_CORE_UITHEME_H
#define LIBREPCB_CORE_UITHEME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Struct UiTheme
 ******************************************************************************/

/**
 * @brief Theme settings (e.g. colors) for the UI
 */
struct UiTheme {
  QString id;  ///< Constant, unique identifier (DO NOT CHANGE!!!)
  const char* name;  ///< Name (en_US, untranslated)
  QPalette qpalette;  ///< Colors for (legacy) Qt widgets

  // Menu bar, side bar, status bar
  QColor window;

  // Tab content
  QColor base;
  QColor baseBorder;
  QColor baseTextDisabled;
  QColor baseTextMuted;
  QColor baseText;
  QColor baseTextHovered;
  QColor baseTextInfo;
  QColor baseTextSuccess;
  QColor baseTextWarning;
  QColor baseTextError;

  // Buttons, text edits, dropdowns, lists, ...
  QColor control;
  QColor controlDisabled;
  QColor controlHovered;
  QColor controlChecked;
  QColor controlBorder;
  QColor controlBorderDisabled;
  QColor controlTextDisabled;
  QColor controlTextMuted;
  QColor controlText;

  // Selected text
  QColor selection;
  QColor selectionText;

  // Tooltips
  QColor tooltip;
  QColor tooltipBorder;
  QColor tooltipText;

  // Various
  QColor accent;
  QColor accentText;
  QColor info;
  QColor infoText;
  QColor warning;
  QColor warningText;
  QColor error;
  QColor errorText;

  // Methods
  QString getNameTr() const noexcept;

  // Themes
  static const UiTheme& light() noexcept;
  static const UiTheme& dark() noexcept;
  static const UiTheme* find(const QString& id) noexcept;
  static QVector<const UiTheme*> all() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
