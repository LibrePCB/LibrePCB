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

#ifndef LIBREPCB_CORE_OUTPUTJOBRUNNER_H
#define LIBREPCB_CORE_OUTPUTJOBRUNNER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../export/graphicsexport.h"
#include "../fileio/filepath.h"
#include "../job/outputjob.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ArchiveOutputJob;
class AssemblyVariant;
class Board3DOutputJob;
class Board;
class BomOutputJob;
class CopyOutputJob;
class GerberExcellonOutputJob;
class GerberX3OutputJob;
class GraphicsOutputJob;
class InteractiveHtmlBomOutputJob;
class LppzOutputJob;
class NetlistOutputJob;
class OutputDirectoryWriter;
class OutputJob;
class PickPlaceOutputJob;
class Project;
class ProjectJsonOutputJob;

/*******************************************************************************
 *  Class OutputJobRunner
 ******************************************************************************/

/**
 * @brief The OutputJobRunner class
 */
class OutputJobRunner final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  OutputJobRunner() = delete;
  OutputJobRunner(const OutputJobRunner& other) = delete;
  explicit OutputJobRunner(Project& project) noexcept;
  ~OutputJobRunner() noexcept;

  // Getters
  const FilePath& getOutputDirectory() const noexcept;
  const QMultiHash<Uuid, FilePath>& getWrittenFiles() const noexcept;

  // Setters
  void setOutputDirectory(const FilePath& fp) noexcept;

  // General Methods
  void run(const QVector<std::shared_ptr<OutputJob>>& jobs);
  QList<FilePath> findUnknownFiles(const QSet<Uuid>& knownJobs) const;
  void removeUnknownFiles(const QList<FilePath>& files);
  GraphicsExport::Pages buildPages(const GraphicsOutputJob& job,
                                   bool rebuildPlanes,
                                   QStringList* errors = nullptr);

  // Operator Overloadings
  OutputJobRunner& operator=(const OutputJobRunner& rhs) = delete;

signals:
  void jobStarted(std::shared_ptr<const OutputJob> job);
  void aboutToWriteFile(const FilePath& fp);
  void aboutToRemoveFile(const FilePath& fp);
  void warning(const QString& msg);
  void previewReady(int index, const QSize& pageSize, const QRectF margins,
                    std::shared_ptr<QPicture> picture);

private:  // Methods
  void run(const OutputJob& job);
  void runImpl(const GraphicsOutputJob& job);
  void runImpl(const GerberExcellonOutputJob& job);
  void runImpl(const PickPlaceOutputJob& job);
  void runImpl(const GerberX3OutputJob& job);
  void runImpl(const NetlistOutputJob& job);
  void runImpl(const BomOutputJob& job);
  void runImpl(const InteractiveHtmlBomOutputJob& job);
  void runImpl(const Board3DOutputJob& job);
  void runImpl(const ProjectJsonOutputJob& job);
  void runImpl(const LppzOutputJob& job);
  void runImpl(const CopyOutputJob& job);
  void runImpl(const ArchiveOutputJob& job);
  QList<Board*> getBoards(const OutputJob::ObjectSet<std::optional<Uuid>>& set,
                          bool includeNullInAll) const;
  QList<Board*> getBoards(const OutputJob::ObjectSet<Uuid>& set) const;
  QVector<std::shared_ptr<AssemblyVariant>> getAssemblyVariants(
      const OutputJob::ObjectSet<std::optional<Uuid>>& set,
      bool includeNullInAll) const;
  QVector<std::shared_ptr<AssemblyVariant>> getAssemblyVariants(
      const OutputJob::ObjectSet<Uuid>& set) const;
  static void rebuildOutdatedPlanes(Board& board);

private:  // Data
  Project& mProject;
  QScopedPointer<OutputDirectoryWriter> mWriter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
