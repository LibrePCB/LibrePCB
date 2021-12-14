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
#ifndef LIBREPCB_MARKDOWN_MARKDOWNCONVERTER_H
#define LIBREPCB_MARKDOWN_MARKDOWNCONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Class MarkdownConverter
 ******************************************************************************/

/**
 * @brief The MarkdownConverter class
 */
class MarkdownConverter final {
public:
  // Static Methods
  static QString convertMarkdownToHtml(const FilePath& markdownFile) noexcept;
  static QString convertMarkdownToHtml(const QString& markdown) noexcept;

private:
  // Constructors / Destructor
  MarkdownConverter() = delete;
  MarkdownConverter(const MarkdownConverter& other) = delete;
  ~MarkdownConverter() = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif
