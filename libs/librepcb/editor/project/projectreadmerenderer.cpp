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
#include "projectreadmerenderer.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/ziparchive.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectReadmeRenderer::ProjectReadmeRenderer(QObject* parent) noexcept
  : QObject(parent), mPath(), mWidth(0) {
  mDelayTimer.setSingleShot(true);
  connect(&mDelayTimer, &QTimer::timeout, this, &ProjectReadmeRenderer::start);
}

ProjectReadmeRenderer::~ProjectReadmeRenderer() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectReadmeRenderer::request(const FilePath& fp, int width) noexcept {
  // Abort current watching, we're no longer interested in its result.
  if (mWatcher) {
    mWatcher->cancel();
    mWatcher.reset();
    emit runningChanged(false);
  }

  // Schedule new run.
  mPath = fp;
  mWidth = width;
  mDelayTimer.start(200);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectReadmeRenderer::start() noexcept {
  // Do not start if input is invalid.
  if ((!mPath.isValid()) || (mWidth <= 0)) {
    emit finished(QPixmap());
    return;
  }

  // Check if README.md exists.
  if (mPath.getSuffix() == "lpp") {
    mPath = mPath.getParentDir().getPathTo("README.md");
  } else if (mPath.isExistingDir()) {
    mPath = mPath.getPathTo("README.md");
  } else if ((mPath.getSuffix() != "lppz") && (mPath.getSuffix() != "md") &&
             (mPath.getSuffix() != "txt")) {
    emit finished(QPixmap());
    return;
  }
  if (!mPath.isExistingFile()) {
    emit finished(QPixmap());
    return;
  }

  // Start rendering in thread.
  emit runningChanged(true);
  mWatcher.reset(new QFutureWatcher<QPixmap>());
  connect(mWatcher.get(), &QFutureWatcher<QPixmap>::finished, this, [this]() {
    emit runningChanged(false);
    emit finished(mWatcher->result());
  });
  mWatcher->setFuture(
      QtConcurrent::run(&ProjectReadmeRenderer::render, mPath, mWidth));
}

QPixmap ProjectReadmeRenderer::render(const FilePath& fp, int width) noexcept {
  try {
    // Create temporary directory.
    const FilePath tmpDir = FilePath::getRandomTempPath();
    auto sg =
        scopeGuard([tmpDir]() { QDir(tmpDir.toStr()).removeRecursively(); });

    // Load the markdown file. If a *.lppz was specified, look for a README.md
    // in the ZIP file.
    QString md;
    std::unique_ptr<ZipArchive> zip;
    if (fp.getSuffix() == "lppz") {
      zip.reset(new ZipArchive(fp));
      if (auto content = zip->tryReadFile("README.md")) {
        md = *content;
      }
    } else {
      md = FileUtils::readFile(fp);
    }

    // Abort if there was no valid markdown file.
    if (md.isEmpty()) return QPixmap();

    // Parse Markdown.
    QTextDocument document;
    document.setBaseUrl(tmpDir.getPathTo(".dummy.md").toQUrl());
    document.setTextWidth(width);
    document.setMarkdown(md, QTextDocument::MarkdownDialectGitHub);

    // Copy referenced images to temporary directory, and shrink them to page
    // width. Otherwise they appear in the original resolution, which can be
    // way too large for the window where it is displayed.
    QTextBlock b = document.begin();
    while (b.isValid()) {
      for (QTextBlock::iterator i = b.begin(); !i.atEnd(); ++i) {
        QTextCharFormat format = i.fragment().charFormat();
        QTextImageFormat imageFormat = format.toImageFormat();
        if (imageFormat.isValid()) {
          const FilePath newFp = tmpDir.getPathTo(imageFormat.name());
          QImage img;
          if (zip) {
            if (auto content = zip->tryReadFile(imageFormat.name())) {
              img.loadFromData(*content);
            }
          } else {
            img.load(fp.getParentDir().getPathTo(imageFormat.name()).toStr());
          }
          if (img.width() > width) {
            img = img.scaledToWidth(width, Qt::SmoothTransformation);
          }
          if (!img.isNull()) {
            FileUtils::makePath(newFp.getParentDir());
            img.save(newFp.toStr());
          }
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
