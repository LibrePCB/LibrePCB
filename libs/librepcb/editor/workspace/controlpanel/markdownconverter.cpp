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

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

#include <qtextdocument.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

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
  // Use a temporary QTextDocument to convert markdown to HTML.
  // This is not terribly efficient (we could return a QTextDocument instead),
  // but since this method is only used in non-performance-sensitive code right
  // now (rendering README files in the project manager) this is fine, because
  // it makes the API simpler (QString in, QString out).
  QTextDocument document;
  document.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
  return document.toHtml();
}

QPixmap MarkdownConverter::convertMarkdownToPixmap(const FilePath& fp,
                                                   int width) noexcept {
  try {
    // Create temporary directory.
    const FilePath tmpDir = FilePath::getRandomTempPath();
    auto sg =
        scopeGuard([tmpDir]() { QDir(tmpDir.toStr()).removeRecursively(); });

    // Parse Markdown.
    QTextDocument document;
    document.setBaseUrl(tmpDir.getPathTo(fp.getFilename()).toQUrl());
    document.setTextWidth(width);
    document.setMarkdown(FileUtils::readFile(fp),
                         QTextDocument::MarkdownDialectGitHub);

    // Copy referenced images to temporary directory, and shrink them to page
    // width.
    QTextBlock b = document.begin();
    while (b.isValid()) {
      for (QTextBlock::iterator i = b.begin(); !i.atEnd(); ++i) {
        QTextCharFormat format = i.fragment().charFormat();
        QTextImageFormat imageFormat = format.toImageFormat();
        if (imageFormat.isValid()) {
          const FilePath oldFp =
              fp.getParentDir().getPathTo(imageFormat.name());
          const FilePath newFp = tmpDir.getPathTo(imageFormat.name());
          QImage img(oldFp.toStr());
          if (img.width() > width) {
            img = img.scaledToWidth(width, Qt::SmoothTransformation);
          }
          FileUtils::makePath(newFp.getParentDir());
          img.save(newFp.toStr());
        }
      }
      b = b.next();
    }

    // Render document.
    QPixmap pixmap(document.size().toSize());
    pixmap.fill(Qt::transparent);
    {
      QPainter painter(&pixmap);
      document.drawContents(&painter);
    }
    return pixmap;
  } catch (const Exception& e) {
    qWarning() << "Failed to render Markdown:" << e.getMsg();
    return QPixmap();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
