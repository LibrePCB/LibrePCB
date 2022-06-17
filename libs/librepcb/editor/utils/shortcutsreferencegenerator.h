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

#ifndef LIBREPCB_EDITOR_SHORTCUTSREFERENCEGENERATOR_H
#define LIBREPCB_EDITOR_SHORTCUTSREFERENCEGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace editor {

class EditorCommand;
class EditorCommandCategory;
class EditorCommandSet;

/*******************************************************************************
 *  Class ShortcutsReferenceGenerator
 ******************************************************************************/

/**
 * @brief Helper to generate a keyboard shortcuts reference PDF
 *
 * Dynamically creates a PDF using the configured keyboard shortcuts (via
 * workspace settings) from ::librepcb::editor::EditorCommandSet.
 *
 * @attention The exported PDF shall be locale-independent, i.e. always in
 *            english to avoid unexpected formatting in other languages!
 */
class ShortcutsReferenceGenerator final {
public:
  enum class Flag {
    Bold = (1 << 0),
    Italic = (1 << 1),
    AlignCenter = (1 << 8),
    AlignRight = (1 << 9),
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  // Constructors / Destructor
  ShortcutsReferenceGenerator() = delete;
  ShortcutsReferenceGenerator(const ShortcutsReferenceGenerator& other) =
      delete;
  explicit ShortcutsReferenceGenerator(EditorCommandSet& commands) noexcept;
  ~ShortcutsReferenceGenerator() noexcept;

  // General Methods
  void generatePdf(const FilePath& fp);

  // Operator Overloadings
  ShortcutsReferenceGenerator& operator=(
      const ShortcutsReferenceGenerator& rhs) = delete;

private:  // Methods
  void drawSectionTitle(QPdfWriter& writer, QPainter& painter, qreal x1,
                        qreal x2, qreal y, const QString& text) const noexcept;
  void drawCommandCategory(QPdfWriter& writer, QPainter& painter, qreal x,
                           qreal y, EditorCommandCategory& cat) const noexcept;
  void drawRow(QPdfWriter& writer, QPainter& painter, qreal x, qreal y,
               qreal totalWidth, qreal shortcutsWidth, const QString& text,
               const QString& shortcuts, bool gray) const noexcept;
  int drawText(QPdfWriter& writer, QPainter& painter, qreal x, qreal y,
               qreal size, qreal maxLength, const QString& text,
               Flags flags = Flags()) const noexcept;
  int mmToPx(QPdfWriter& writer, qreal mm) const noexcept;

private:  // Data
  EditorCommandSet& mCommands;

  static constexpr qreal sPageWidth = 270;
  static constexpr qreal sPageHeight = 190;
  static constexpr qreal sCategoryTextSize = 3;
  static constexpr qreal sRowTextSize = 2.5;
  static constexpr qreal sRowHeight = 3;
  static constexpr qreal sCategorySpacing = 6;
  static constexpr qreal sColumnSpacing = 3.5;
  static constexpr qreal sColumnWidth = (sPageWidth - 3 * sColumnSpacing) / 4;
  static constexpr qreal sShortcutsWidth = 28;
};

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(
    librepcb::editor::ShortcutsReferenceGenerator::Flags)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
