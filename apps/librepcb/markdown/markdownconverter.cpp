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
#include "markdownconverter.h"

#include <QtCore>

extern "C" {
#include <hoedown/src/html.h>
}

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QString MarkdownConverter::convertMarkdownToHtml(
    const FilePath& markdownFile) noexcept {
  QFile file(markdownFile.toStr());
  if (file.open(QFile::ReadOnly)) {
    return convertMarkdownToHtml(file.readAll());
  } else {
    return QString();
  }
}

QString MarkdownConverter::convertMarkdownToHtml(
    const QString& markdown) noexcept {
  // create HTML renderer
  hoedown_html_flags flags    = static_cast<hoedown_html_flags>(0);
  hoedown_renderer*  renderer = hoedown_html_renderer_new(flags, 0);

  // create document parser
  hoedown_extensions extensions = static_cast<hoedown_extensions>(
      HOEDOWN_EXT_TABLES | HOEDOWN_EXT_FENCED_CODE | HOEDOWN_EXT_AUTOLINK |
      HOEDOWN_EXT_STRIKETHROUGH | HOEDOWN_EXT_NO_INTRA_EMPHASIS);
  hoedown_document* document = hoedown_document_new(renderer, extensions, 16);

  // render markdown
  QByteArray   markdownUtf8 = markdown.toUtf8();
  const uchar* markdownData =
      reinterpret_cast<const uchar*>(markdownUtf8.constData());
  hoedown_buffer* htmlBuffer = hoedown_buffer_new(64);
  hoedown_document_render(document, htmlBuffer, markdownData,
                          markdownUtf8.size());

  // get HTML output
  QString html = QString::fromUtf8(hoedown_buffer_cstr(htmlBuffer));

  // clean up
  hoedown_buffer_free(htmlBuffer);
  hoedown_document_free(document);
  hoedown_html_renderer_free(renderer);

  return html;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
