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
#include "outputdirectorywriter.h"

#include "../exceptions.h"
#include "../types/uuid.h"
#include "fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OutputDirectoryWriter::OutputDirectoryWriter(const FilePath& dirPath) noexcept
  : QObject(),
    mDirPath(dirPath),
    mIndexFilePath(dirPath.getPathTo(".librepcb-output")),
    mIndex(),
    mIndexLoaded(false),
    mIndexModified(false) {
}

OutputDirectoryWriter::~OutputDirectoryWriter() noexcept {
  if (mIndexModified) {
    try {
      storeIndex();  // can throw
    } catch (const Exception& e) {
      qCritical() << "Failed to automatically store output directory index:"
                  << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool OutputDirectoryWriter::loadIndex() {
  bool success = false;
  try {
    mIndex.clear();
    if (mIndexFilePath.isExistingFile()) {
      const QString content = FileUtils::readFile(mIndexFilePath);  // can throw
      const QStringList lines = content.split("\n", Qt::SkipEmptyParts);
      foreach (const QString& line, lines) {
        const QStringList values = line.split(" | ", Qt::KeepEmptyParts);
        if (values.count() >= 2) {
          const QString file = values.first();
          const Uuid uuid = Uuid::fromString(values.value(1));
          mIndex.insert(mDirPath.getPathTo(file), uuid);
        }
      }
    }
    success = true;
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
  mIndexLoaded = true;
  mIndexModified = false;
  return success;
}

void OutputDirectoryWriter::storeIndex() {
  QStringList lines;
  for (auto it = mIndex.begin(); it != mIndex.end(); ++it) {
    if (it.key().isExistingFile()) {
      lines.append(QString("%1 | %2")
                       .arg(it.key().toRelative(mDirPath))
                       .arg(it.value().toStr()));
    }
  }
  std::sort(lines.begin(), lines.end());
  const QString content = lines.join("\n") % "\n";
  FileUtils::writeFile(mIndexFilePath, content.toUtf8());  // can throw
  mIndexModified = false;
}

FilePath OutputDirectoryWriter::beginWritingFile(const Uuid& job,
                                                 const QString& relPath) {
  const FilePath fp = mDirPath.getPathTo(relPath);
  emit aboutToWriteFile(fp);

  if (!mIndexLoaded) {
    throw LogicError(__FILE__, __LINE__, "Output directory index not loaded.");
  }

  if (relPath.contains("|")) {
    throw RuntimeError(
        __FILE__, __LINE__,
        "Sorry, the character '|' cannot be used in output filenames.");
  }
  if (mWrittenFiles.values().contains(fp)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("Attempted to write the output file '%1' multiple times!")
                .arg(fp.toRelativeNative(mDirPath)) %
            " " %
            tr("Make sure to specify unique output file paths, "
               "e.g. by using placeholders like '%1' or '%2'.")
                .arg("{{BOARD}}")
                .arg("{{VARIANT}}"));
  }

  mIndex.insert(fp, job);
  mIndexModified = true;
  mWrittenFiles.insert(job, fp);
  return fp;
}

void OutputDirectoryWriter::removeObsoleteFiles(const Uuid& job) {
  const auto tmpIndex = mIndex;  // Avoid removing while iterating.
  for (auto it = tmpIndex.begin(); it != tmpIndex.end(); ++it) {
    if ((it.value() == job) &&
        (!mWrittenFiles.values(job).contains(it.key()))) {
      emit aboutToRemoveFile(it.key());
      if (it.key().isExistingFile()) {
        FileUtils::removeFile(it.key());  // can throw
      }
      mIndex.remove(it.key());
    }
  }
}

QList<FilePath> OutputDirectoryWriter::findUnknownFiles(
    const QSet<Uuid>& knownJobs) const {
  if (!mIndexLoaded) {
    throw LogicError(__FILE__, __LINE__, "Output directory index not loaded.");
  }
  QList<FilePath> result;
  if (mDirPath.isExistingDir()) {
    // Note: Ignore hidden files such as .DS_Store or Thumbs.db.
    result = FileUtils::getFilesInDirectory(mDirPath, {}, true, true);
    result.removeOne(mIndexFilePath);
    for (auto it = mIndex.begin(); it != mIndex.end(); ++it) {
      if (knownJobs.contains(it.value())) {
        result.removeOne(it.key());
      }
    }
  }
  return result;
}

void OutputDirectoryWriter::removeUnknownFiles(const QList<FilePath>& files) {
  if (!mIndexLoaded) {
    throw LogicError(__FILE__, __LINE__, "Output directory index not loaded.");
  }
  foreach (const FilePath& fp, files) {
    emit aboutToRemoveFile(fp);
    FileUtils::removeFile(fp);  // can throw
    QDir().rmpath(fp.getParentDir().toStr());  // Remove empty parents.
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
