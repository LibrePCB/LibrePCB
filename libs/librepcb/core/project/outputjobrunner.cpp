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
#include "outputjobrunner.h"

#include "../3d/stepexport.h"
#include "../application.h"
#include "../attribute/attributesubstitutor.h"
#include "../export/bomcsvwriter.h"
#include "../export/graphicsexport.h"
#include "../export/graphicsexportsettings.h"
#include "../export/interactivehtmlbom.h"
#include "../export/pickplacecsvwriter.h"
#include "../fileio/csvfile.h"
#include "../fileio/fileutils.h"
#include "../fileio/outputdirectorywriter.h"
#include "../fileio/transactionalfilesystem.h"
#include "../job/archiveoutputjob.h"
#include "../job/board3doutputjob.h"
#include "../job/bomoutputjob.h"
#include "../job/copyoutputjob.h"
#include "../job/gerberexcellonoutputjob.h"
#include "../job/gerberx3outputjob.h"
#include "../job/graphicsoutputjob.h"
#include "../job/interactivehtmlbomoutputjob.h"
#include "../job/lppzoutputjob.h"
#include "../job/netlistoutputjob.h"
#include "../job/pickplaceoutputjob.h"
#include "../job/projectjsonoutputjob.h"
#include "../types/layer.h"
#include "../utils/toolbox.h"
#include "board/board.h"
#include "board/boardd356netlistexport.h"
#include "board/boardfabricationoutputsettings.h"
#include "board/boardgerberexport.h"
#include "board/boardinteractivehtmlbomgenerator.h"
#include "board/boardpainter.h"
#include "board/boardpickplacegenerator.h"
#include "board/boardplanefragmentsbuilder.h"
#include "board/realisticboardpainter.h"
#include "bomgenerator.h"
#include "circuit/circuit.h"
#include "project.h"
#include "projectattributelookup.h"
#include "projectjsonexport.h"
#include "schematic/schematicpainter.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OutputJobRunner::OutputJobRunner(Project& project) noexcept
  : QObject(nullptr), mProject(project), mWriter() {
  setOutputDirectory(mProject.getCurrentOutputDir());
}

OutputJobRunner::~OutputJobRunner() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const FilePath& OutputJobRunner::getOutputDirectory() const noexcept {
  return mWriter->getDirectoryPath();
}

const QMultiHash<Uuid, FilePath>& OutputJobRunner::getWrittenFiles()
    const noexcept {
  return mWriter->getWrittenFiles();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void OutputJobRunner::setOutputDirectory(const FilePath& fp) noexcept {
  mWriter.reset(new OutputDirectoryWriter(fp));
  connect(mWriter.data(), &OutputDirectoryWriter::aboutToWriteFile, this,
          &OutputJobRunner::aboutToWriteFile);
  connect(mWriter.data(), &OutputDirectoryWriter::aboutToRemoveFile, this,
          &OutputJobRunner::aboutToRemoveFile);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OutputJobRunner::run(const QVector<std::shared_ptr<OutputJob>>& jobs) {
  mWriter->loadIndex();  // can throw
  foreach (const auto& job, jobs) {
    emit jobStarted(job);
    run(*job);  // can throw
    qApp->processEvents();  // Avoid freeze due to blocking loop.
  }
  mWriter->storeIndex();  // can throw
}

QList<FilePath> OutputJobRunner::findUnknownFiles(
    const QSet<Uuid>& knownJobs) const {
  return mWriter->findUnknownFiles(knownJobs);
}

void OutputJobRunner::removeUnknownFiles(const QList<FilePath>& files) {
  mWriter->removeUnknownFiles(files);
}

GraphicsExport::Pages OutputJobRunner::buildPages(const GraphicsOutputJob& job,
                                                  bool rebuildPlanes,
                                                  QStringList* errors) {
  using Type = GraphicsOutputJob::Content::Type;

  GraphicsExport::Pages pages;
  foreach (const GraphicsOutputJob::Content& content, job.getContent()) {
    std::shared_ptr<GraphicsExportSettings> settings =
        std::make_shared<GraphicsExportSettings>();
    std::optional<QPageSize> pageSize;
    if (content.pageSizeKey) {
      for (int i = 0; i < QPageSize::LastPageSize; ++i) {
        const QPageSize::PageSizeId id = static_cast<QPageSize::PageSizeId>(i);
        if (QPageSize::key(id) == content.pageSizeKey) {
          pageSize = QPageSize(id);
          break;
        }
      }
      if (!pageSize) {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Unsupported page size: '%1'").arg(*content.pageSizeKey));
      }
    }
    settings->setPageSize(pageSize);
    settings->setOrientation(content.orientation);
    settings->setMarginLeft(content.marginLeft);
    settings->setMarginTop(content.marginTop);
    settings->setMarginRight(content.marginRight);
    settings->setMarginBottom(content.marginBottom);
    settings->setRotate(content.rotate);
    settings->setMirror(content.mirror);
    settings->setScale(content.scale);
    settings->setPixmapDpi(static_cast<int>(content.pixmapDpi));
    settings->setBlackWhite(content.monochrome);
    settings->setBackgroundColor(content.backgroundColor);
    settings->setMinLineWidth(content.minLineWidth);
    QList<std::pair<QString, QColor>> layers;
    if (content.type == Type::BoardRendering) {
      settings->loadBoardRenderingColors(Layer::innerCopperCount());
    }
    foreach (const auto& pair, settings->getColors()) {
      if (content.layers.contains(pair.first)) {
        layers.append(std::make_pair(pair.first, content.layers[pair.first]));
      }
    }
    settings->setColors(layers);
    const QList<Board*> boards = getBoards(content.boards, false);
    const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
        getAssemblyVariants(content.assemblyVariants, false);
    if (content.type == Type::Schematic) {
      foreach (auto av, assemblyVariants) {
        Q_UNUSED(av);  // TODO
        foreach (const Board* board, boards) {
          Q_UNUSED(board);  // TODO
          foreach (const Schematic* schematic, mProject.getSchematics()) {
            std::shared_ptr<GraphicsPagePainter> painter =
                std::make_shared<SchematicPainter>(*schematic, errors);
            pages.append(std::make_pair(painter, settings));
          }
        }
      }
    } else if ((content.type == Type::Board) ||
               (content.type == Type::BoardRendering)) {
      foreach (Board* board, boards) {
        if (rebuildPlanes) {
          rebuildOutdatedPlanes(*board);  // can throw
        }

        foreach (auto av, assemblyVariants) {
          Q_UNUSED(av);  // TODO
          std::shared_ptr<GraphicsPagePainter> painter;
          if (content.type == Type::BoardRendering) {
            painter = std::make_shared<RealisticBoardPainter>(
                board->buildScene3D(std::nullopt));
          } else {
            painter = std::make_shared<BoardPainter>(*board);
          }
          pages.append(std::make_pair(painter, settings));
        }
      }
    } else if (content.type == Type::AssemblyGuide) {
      throw RuntimeError(__FILE__, __LINE__,
                         "Assembly guide output jobs are not supported yet, "
                         "you need to use a more recent release of LibrePCB.");
    } else {
      throw LogicError(__FILE__, __LINE__,
                       "Unknown graphics output job content.");
    }
  }
  return pages;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OutputJobRunner::run(const OutputJob& job) {
  const int countBefore = mWriter->getWrittenFiles().count(job.getUuid());
  if (auto ptr = dynamic_cast<const GraphicsOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const GerberExcellonOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const PickPlaceOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const GerberX3OutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const NetlistOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const BomOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr =
                 dynamic_cast<const InteractiveHtmlBomOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const Board3DOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const ProjectJsonOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const LppzOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const CopyOutputJob*>(&job)) {
    runImpl(*ptr);
  } else if (auto ptr = dynamic_cast<const ArchiveOutputJob*>(&job)) {
    runImpl(*ptr);
  } else {
    throw LogicError(
        __FILE__, __LINE__,
        tr("Unknown output job type '%1'.").arg(job.getType()) % " " %
            tr("You may need a more recent LibrePCB version to run this job."));
  }
  const int countAfter = mWriter->getWrittenFiles().count(job.getUuid());
  mWriter->removeObsoleteFiles(job.getUuid());  // can throw
  if (countAfter <= countBefore) {
    emit warning(
        tr("No output files were generated, check the job configuration."));
  }
}

void OutputJobRunner::runImpl(const GraphicsOutputJob& job) {
  // Build pages.
  QStringList errors;
  const GraphicsExport::Pages pages =
      buildPages(job, true, &errors);  // can throw

  // Determine lookup objects.
  QSet<Board*> allBoards;
  QSet<std::shared_ptr<AssemblyVariant>> allAssemblyVariants;
  foreach (const GraphicsOutputJob::Content& content, job.getContent()) {
    allBoards |= Toolbox::toSet(getBoards(content.boards, false));
    allAssemblyVariants |=
        Toolbox::toSet(getAssemblyVariants(content.assemblyVariants, false));
  }

  // Determine output path.
  std::shared_ptr<AssemblyVariant> av = (allAssemblyVariants.count() == 1)
      ? (*allAssemblyVariants.begin())
      : nullptr;
  const ProjectAttributeLookup lookup =
      ((allBoards.count() == 1) && (*allBoards.begin()))
      ? ProjectAttributeLookup(**allBoards.begin(), av)
      : ProjectAttributeLookup(mProject, av);
  const FilePath fp = mWriter->beginWritingFile(
      job.getUuid(),
      AttributeSubstitutor::substitute(
          job.getOutputPath(), lookup, [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          }));  // can throw

  // Determine document name.
  QString docName = *job.getDocumentTitle();
  if (docName.isEmpty()) {
    docName = "{{PROJECT}} {{VERSION}}";
  }
  docName = AttributeSubstitutor::substitute(docName, lookup).simplified();

  // Perform the export.
  GraphicsExport graphicsExport;
  graphicsExport.setDocumentName(docName);
  graphicsExport.startExport(pages, fp);
  const GraphicsExport::Result result = graphicsExport.waitForFinished();
  foreach (const FilePath& writtenFile, result.writtenFiles) {
    if (writtenFile != fp) {
      // Track additional files.
      mWriter->beginWritingFile(
          job.getUuid(),
          writtenFile.toRelative(mWriter->getDirectoryPath()));  // can throw
    }
  }
  if (!result.errorMsg.isEmpty()) {
    errors.append(result.errorMsg);
  }
  if (!errors.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__, errors.join("; "));
  }
}

void OutputJobRunner::runImpl(const GerberExcellonOutputJob& job) {
  // Build settings.
  BoardFabricationOutputSettings settings;
  settings.setOutputBasePath(mWriter->getDirectoryPath().toStr() % "/" %
                             job.getOutputPath());
  settings.setSuffixDrills(job.getSuffixDrills());
  settings.setSuffixDrillsNpth(job.getSuffixDrillsNpth());
  settings.setSuffixDrillsPth(job.getSuffixDrillsPth());
  settings.setSuffixDrillsBlindBuried(job.getSuffixDrillsBlindBuried());
  settings.setSuffixOutlines(job.getSuffixOutlines());
  settings.setSuffixCopperTop(job.getSuffixCopperTop());
  settings.setSuffixCopperInner(job.getSuffixCopperInner());
  settings.setSuffixCopperBot(job.getSuffixCopperBot());
  settings.setSuffixSolderMaskTop(job.getSuffixSolderMaskTop());
  settings.setSuffixSolderMaskBot(job.getSuffixSolderMaskBot());
  settings.setSuffixSilkscreenTop(job.getSuffixSilkscreenTop());
  settings.setSuffixSilkscreenBot(job.getSuffixSilkscreenBot());
  settings.setSuffixSolderPasteTop(job.getSuffixSolderPasteTop());
  settings.setSuffixSolderPasteBot(job.getSuffixSolderPasteBot());
  settings.setMergeDrillFiles(job.getMergeDrillFiles());
  settings.setUseG85SlotCommand(job.getUseG85SlotCommand());
  settings.setEnableSolderPasteTop(job.getEnableSolderPasteTop());
  settings.setEnableSolderPasteBot(job.getEnableSolderPasteBot());

  // Determine boards.
  const QList<Board*> boards = getBoards(job.getBoards());

  // Perform export.
  foreach (Board* board, boards) {
    // Rebuild planes to be sure no outdated planes are exported!
    rebuildOutdatedPlanes(*board);  // can throw

    // Now actually export Gerber/Excellon.
    BoardGerberExport grbExport(*board);
    grbExport.setRemoveObsoleteFiles(false);  // must be done by this runner!
    grbExport.setBeforeWriteCallback([this, &job](const FilePath& fp) {
      mWriter->beginWritingFile(job.getUuid(),
                                fp.toRelative(mWriter->getDirectoryPath()));
    });
    grbExport.exportPcbLayers(settings);  // can throw
  }
}

void OutputJobRunner::runImpl(const PickPlaceOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards());
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants());

  QVector<std::pair<PickPlaceCsvWriter::BoardSide, QString>> sides;
  if (job.getCreateTop()) {
    sides.append(std::make_pair(PickPlaceCsvWriter::BoardSide::Top,
                                job.getOutputPathTop()));
  }
  if (job.getCreateBottom()) {
    sides.append(std::make_pair(PickPlaceCsvWriter::BoardSide::Bottom,
                                job.getOutputPathBottom()));
  }
  if (job.getCreateBoth()) {
    sides.append(std::make_pair(PickPlaceCsvWriter::BoardSide::Both,
                                job.getOutputPathBoth()));
  }

  QSet<PickPlaceDataItem::Type> typeFilter;
  if (job.getTechnologies().testFlag(PickPlaceOutputJob::Technology::Tht)) {
    typeFilter.insert(PickPlaceDataItem::Type::Tht);
  }
  if (job.getTechnologies().testFlag(PickPlaceOutputJob::Technology::Smt)) {
    typeFilter.insert(PickPlaceDataItem::Type::Smt);
  }
  if (job.getTechnologies().testFlag(PickPlaceOutputJob::Technology::Mixed)) {
    typeFilter.insert(PickPlaceDataItem::Type::Mixed);
  }
  if (job.getTechnologies().testFlag(
          PickPlaceOutputJob::Technology::Fiducial)) {
    typeFilter.insert(PickPlaceDataItem::Type::Fiducial);
  }
  if (job.getTechnologies().testFlag(PickPlaceOutputJob::Technology::Other)) {
    typeFilter.insert(PickPlaceDataItem::Type::Other);
  }
  if (typeFilter.isEmpty()) {
    emit warning(
        tr("No technologies selected, thus the output files won't "
           "contain any entries."));
  }

  foreach (const Board* board, boards) {
    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      BoardPickPlaceGenerator gen(*board, av->getUuid());
      std::shared_ptr<PickPlaceData> data = gen.generate();
      foreach (const auto& pair, sides) {
        const FilePath fp = mWriter->beginWritingFile(
            job.getUuid(),
            AttributeSubstitutor::substitute(
                pair.second, ProjectAttributeLookup(*board, av),
                [&](const QString& str) {
                  return FilePath::cleanFileName(
                      str, FilePath::ReplaceSpaces | FilePath::KeepCase);
                }));  // can throw

        if (fp.getSuffix().toLower() == "csv") {
          PickPlaceCsvWriter writer(*data);
          writer.setIncludeMetadataComment(job.getIncludeComment());
          writer.setBoardSide(pair.first);
          writer.setTypeFilter(typeFilter);
          std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
          csv->saveToFile(fp);  // can throw
        } else {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Unsupported pick&place format: '%1'")
                                 .arg(fp.getSuffix()));
        }
      }
    }
  }
}

void OutputJobRunner::runImpl(const GerberX3OutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards());
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants());

  QVector<std::pair<BoardGerberExport::BoardSide, QString>> sides;
  if (job.getCreateTop()) {
    sides.append(std::make_pair(BoardGerberExport::BoardSide::Top,
                                job.getOutputPathTop()));
  }
  if (job.getCreateBottom()) {
    sides.append(std::make_pair(BoardGerberExport::BoardSide::Bottom,
                                job.getOutputPathBottom()));
  }

  foreach (const Board* board, boards) {
    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      foreach (const auto& pair, sides) {
        const FilePath fp = mWriter->beginWritingFile(
            job.getUuid(),
            AttributeSubstitutor::substitute(
                pair.second, ProjectAttributeLookup(*board, av),
                [&](const QString& str) {
                  return FilePath::cleanFileName(
                      str, FilePath::ReplaceSpaces | FilePath::KeepCase);
                }));  // can throw

        BoardGerberExport gen(*board);
        gen.exportComponentLayer(pair.first, av->getUuid(), fp);  // can throw
      }
    }
  }
}

void OutputJobRunner::runImpl(const NetlistOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards());
  foreach (const Board* board, boards) {
    const FilePath fp = mWriter->beginWritingFile(
        job.getUuid(),
        AttributeSubstitutor::substitute(
            job.getOutputPath(), ProjectAttributeLookup(*board, nullptr),
            [&](const QString& str) {
              return FilePath::cleanFileName(
                  str, FilePath::ReplaceSpaces | FilePath::KeepCase);
            }));  // can throw

    if (fp.getSuffix().toLower() == "d356") {
      BoardD356NetlistExport exp(*board);
      FileUtils::writeFile(fp, exp.generate());  // can throw
    } else {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Unsupported netlist format: '%1'").arg(fp.getSuffix()));
    }
  }
}

void OutputJobRunner::runImpl(const BomOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards(), false);
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants());

  foreach (const Board* board, boards) {
    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      const ProjectAttributeLookup lookup = board
          ? ProjectAttributeLookup(*board, av)
          : ProjectAttributeLookup(mProject, av);
      const FilePath fp = mWriter->beginWritingFile(
          job.getUuid(),
          AttributeSubstitutor::substitute(
              job.getOutputPath(), lookup, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              }));  // can throw

      BomGenerator gen(mProject);
      gen.setAdditionalAttributes(job.getCustomAttributes());
      std::shared_ptr<Bom> bom = gen.generate(board, av->getUuid());
      if (fp.getSuffix().toLower() == "csv") {
        BomCsvWriter writer(*bom);
        std::shared_ptr<CsvFile> csv = writer.generateCsv();
        csv->saveToFile(fp);
      } else {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Unsupported BOM format: '%1'").arg(fp.getSuffix()));
      }
    }
  }
}

void OutputJobRunner::runImpl(const InteractiveHtmlBomOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards());
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants());

  foreach (Board* board, boards) {
    // Rebuild planes to be sure no outdated planes are exported!
    rebuildOutdatedPlanes(*board);  // can throw

    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      const ProjectAttributeLookup lookup = board
          ? ProjectAttributeLookup(*board, av)
          : ProjectAttributeLookup(mProject, av);
      const FilePath fp = mWriter->beginWritingFile(
          job.getUuid(),
          AttributeSubstitutor::substitute(
              job.getOutputPath(), lookup, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              }));  // can throw

      if ((fp.getSuffix().toLower() == "html") ||
          (fp.getSuffix().toLower() == "htm") ||
          (fp.getSuffix().toLower() == "xhtml")) {
        BoardInteractiveHtmlBomGenerator gen(*board, av);
        gen.setCustomAttributes(job.getCustomAttributes());
        gen.setComponentOrder(job.getComponentOrder());
        std::shared_ptr<InteractiveHtmlBom> ibom =
            gen.generate(QDateTime::currentDateTime());  // can throw
        ibom->setViewConfig(job.getViewMode(), job.getHighlightPin1(),
                            job.getDarkMode());
        ibom->setBoardRotation(job.getBoardRotation(),
                               job.getOffsetBackRotation());
        ibom->setShowSilkscreen(job.getShowSilkscreen());
        ibom->setShowFabrication(job.getShowFabrication());
        ibom->setShowPads(job.getShowPads());
        ibom->setCheckBoxes(job.getCheckBoxes());
        const QString html = ibom->generateHtml();  // can throw
        FileUtils::writeFile(fp, html.toUtf8());  // can throw
      } else {
        throw RuntimeError(__FILE__, __LINE__,
                           QString("Unsupported interactive BOM format: '%1'")
                               .arg(fp.getSuffix()));
      }
    }
  }
}

void OutputJobRunner::runImpl(const Board3DOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards());
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants(), false);

  foreach (Board* board, boards) {
    // Rebuild planes to be sure no outdated planes are exported!
    rebuildOutdatedPlanes(*board);  // can throw

    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      const FilePath fp = mWriter->beginWritingFile(
          job.getUuid(),
          AttributeSubstitutor::substitute(
              job.getOutputPath(), ProjectAttributeLookup(*board, av),
              [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              }));  // can throw

      std::shared_ptr<SceneData3D> data = board->buildScene3D(
          av ? std::make_optional(av->getUuid()) : std::nullopt);

      if ((fp.getSuffix().toLower() == "step") ||
          (fp.getSuffix().toLower() == "stp")) {
        StepExport stepExport;
        stepExport.start(data, fp);
        const QString errorMsg = stepExport.waitForFinished();
        if (!errorMsg.isEmpty()) {
          throw RuntimeError(__FILE__, __LINE__, errorMsg);
        }
      } else {
        throw RuntimeError(
            __FILE__, __LINE__,
            QString("Unsupported netlist format: '%1'").arg(fp.getSuffix()));
      }
    }
  }
}

void OutputJobRunner::runImpl(const ProjectJsonOutputJob& job) {
  // Determine output file.
  const FilePath fp = mWriter->beginWritingFile(
      job.getUuid(),
      AttributeSubstitutor::substitute(
          job.getOutputPath(), ProjectAttributeLookup(mProject, nullptr),
          [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          }));  // can throw

  // Export JSON.
  ProjectJsonExport jsonExport;
  FileUtils::writeFile(fp, jsonExport.toUtf8(mProject));  // can throw
}

void OutputJobRunner::runImpl(const LppzOutputJob& job) {
  // Determine output file.
  const FilePath fp = mWriter->beginWritingFile(
      job.getUuid(),
      AttributeSubstitutor::substitute(
          job.getOutputPath(), ProjectAttributeLookup(mProject, nullptr),
          [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          }));  // can throw

  // Usually we save the project to the transactional file system (but not to
  // the disk!) before exporting the *.lppz since the user probably expects
  // that the current state of the project gets exported. However, if the
  // file format is unstable (i.e. on development branches), this would lead
  // in a *.lppz of an unstable file format, which is not really useful (most
  // *.lppz readers will not support an unstable file format). Therefore we
  // don't save the project on development branches. Note that unfortunately
  // this doesn't work if there are any changes in the project and an autosave
  // was already performed, but it is almost impossible to fix this issue :-(
  if (Application::isFileFormatStable()) {
    mProject.save();  // can throw
  }

  // Export project to ZIP, but without the output directory since this can
  // be quite large and usually does not make sense, especially since *.lppz
  // files might even be stored in this directory as well because they are
  // output files.
  auto filter = [](const QString& filePath) {
    return !filePath.startsWith("output/");
  };
  mProject.getDirectory().getFileSystem()->exportToZip(fp,
                                                       filter);  // can throw
}

void OutputJobRunner::runImpl(const CopyOutputJob& job) {
  const QList<Board*> boards = getBoards(job.getBoards(), false);
  const QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants =
      getAssemblyVariants(job.getAssemblyVariants(), false);

  foreach (const Board* board, boards) {
    foreach (const std::shared_ptr<AssemblyVariant>& av, assemblyVariants) {
      const ProjectAttributeLookup lookup = board
          ? ProjectAttributeLookup(*board, av)
          : ProjectAttributeLookup(mProject, av);
      QString inputPath = AttributeSubstitutor::substitute(
          job.getInputPath(), lookup, [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          });
      const FilePath outputFp = mWriter->beginWritingFile(
          job.getUuid(),
          AttributeSubstitutor::substitute(
              job.getOutputPath(), lookup, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              }));  // can throw

      // The input file must be located within the project to keep the project
      // self-contained, thus we can load it from the transactional filesystem.
      // This also ensures that the job works for *.lppz projects.
      // For compatibility, we need to normalize the specified file path.
      const FilePath inputFp = mProject.getPath().getPathTo(inputPath);
      if ((!QDir::isRelativePath(inputPath)) ||
          (!inputFp.isLocatedInDir(mProject.getPath()))) {
        throw RuntimeError(
            __FILE__, __LINE__,
            tr("The input file must be located within the project directory, "
               "specified by a relative file path."));
      }
      inputPath = inputFp.toRelative(mProject.getPath());

      // Copy file.
      QByteArray content =
          mProject.getDirectory().read(inputPath);  // can throw
      if (job.getSubstituteVariables()) {
        content = AttributeSubstitutor::substitute(content, lookup).toUtf8();
      }
      FileUtils::writeFile(outputFp, content);  // can throw
    }
  }
}

void OutputJobRunner::runImpl(const ArchiveOutputJob& job) {
  // Determine output file.
  const FilePath fp = mWriter->beginWritingFile(
      job.getUuid(),
      AttributeSubstitutor::substitute(
          job.getOutputPath(), ProjectAttributeLookup(mProject, nullptr),
          [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          }));  // can throw

  // Collect input files.
  std::shared_ptr<TransactionalFileSystem> fs =
      TransactionalFileSystem::openRW(FilePath::getRandomTempPath());
  for (auto it = job.getInputJobs().begin(); it != job.getInputJobs().end();
       ++it) {
    if (!mWriter->getWrittenFiles().contains(it.key())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The archive job depends on files from another job which was not "
             "run yet. Note that archive jobs can only depend on jobs further "
             "ahead in the list so you might need to reorder them."));
    }
    foreach (const FilePath& inputFp,
             mWriter->getWrittenFiles().values(it.key())) {
      fs->write(it.value() % "/" % inputFp.getFilename(),
                FileUtils::readFile(inputFp));  // can throw
    }
  }
  if (job.getInputJobs().isEmpty()) {
    emit warning(
        tr("No input jobs selected, thus the resulting archive will "
           "be empty."));
  }

  // Export depending on file extension.
  if (fp.getSuffix().toLower() == "zip") {
    fs->exportToZip(fp);  // can throw
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Unsupported archive format: '%1'").arg(fp.getSuffix()));
  }
}

QList<Board*> OutputJobRunner::getBoards(
    const OutputJob::ObjectSet<std::optional<Uuid>>& set,
    bool includeNullInAll) const {
  QList<Board*> result;
  if (set.isAll()) {
    if (includeNullInAll) {
      result.append(nullptr);
    }
    result += mProject.getBoards();
  } else if (set.isDefault()) {
    result.append(mProject.getBoardByIndex(0));
  } else {
    QSet<std::optional<Uuid>> remainingUuids = set.getSet();
    if (set.getSet().contains(std::nullopt)) {
      result.append(nullptr);
      remainingUuids.remove(std::nullopt);
    }
    foreach (auto board, mProject.getBoards()) {
      if (remainingUuids.contains(board->getUuid())) {
        result.append(board);
        remainingUuids.remove(board->getUuid());
      }
    }
    foreach (const auto& uuid, remainingUuids) {
      Q_ASSERT(uuid);
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Board does not exist: %1").arg(uuid->toStr()));
    }
  }
  return result;
}

QList<Board*> OutputJobRunner::getBoards(
    const OutputJob::ObjectSet<Uuid>& set) const {
  QList<Board*> result;
  if (set.isAll()) {
    result = mProject.getBoards();
  } else if (set.isDefault()) {
    if (auto board = mProject.getBoardByIndex(0)) {
      result.append(board);
    }
  } else {
    QSet<Uuid> remainingUuids = set.getSet();
    foreach (auto board, mProject.getBoards()) {
      if (remainingUuids.contains(board->getUuid())) {
        result.append(board);
        remainingUuids.remove(board->getUuid());
      }
    }
    foreach (const auto& uuid, remainingUuids) {
      throw RuntimeError(__FILE__, __LINE__,
                         QString("Board does not exist: %1").arg(uuid.toStr()));
    }
  }
  return result;
}

QVector<std::shared_ptr<AssemblyVariant>> OutputJobRunner::getAssemblyVariants(
    const OutputJob::ObjectSet<std::optional<Uuid>>& set,
    bool includeNullInAll) const {
  QVector<std::shared_ptr<AssemblyVariant>> result;
  if (set.isAll()) {
    if (includeNullInAll) {
      result.append(nullptr);
    }
    result += mProject.getCircuit().getAssemblyVariants().values();
  } else if (set.isDefault()) {
    result.append(mProject.getCircuit().getAssemblyVariants().value(0));
  } else {
    QSet<std::optional<Uuid>> remainingUuids = set.getSet();
    if (set.getSet().contains(std::nullopt)) {
      result.append(nullptr);
      remainingUuids.remove(std::nullopt);
    }
    foreach (auto av, mProject.getCircuit().getAssemblyVariants().values()) {
      if (remainingUuids.contains(av->getUuid())) {
        result.append(av);
        remainingUuids.remove(av->getUuid());
      }
    }
    foreach (const auto& uuid, remainingUuids) {
      Q_ASSERT(uuid);
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Assembly variant does not exist: %1").arg(uuid->toStr()));
    }
  }
  return result;
}

QVector<std::shared_ptr<AssemblyVariant>> OutputJobRunner::getAssemblyVariants(
    const OutputJob::ObjectSet<Uuid>& set) const {
  QVector<std::shared_ptr<AssemblyVariant>> result;
  if (set.isAll()) {
    result = mProject.getCircuit().getAssemblyVariants().values();
  } else if (set.isDefault()) {
    if (auto av = mProject.getCircuit().getAssemblyVariants().value(0)) {
      result.append(av);
    }
  } else {
    QSet<Uuid> remainingUuids = set.getSet();
    foreach (auto av, mProject.getCircuit().getAssemblyVariants().values()) {
      if (remainingUuids.contains(av->getUuid())) {
        result.append(av);
        remainingUuids.remove(av->getUuid());
      }
    }
    foreach (const auto& uuid, remainingUuids) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("Assembly variant does not exist: %1").arg(uuid.toStr()));
    }
  }
  return result;
}

void OutputJobRunner::rebuildOutdatedPlanes(Board& board) {
  const auto layers = board.getCopperLayers();
  BoardPlaneFragmentsBuilder builder;
  if (builder.start(board, &layers)) {
    BoardPlaneFragmentsBuilder::Result result = builder.waitForFinished();
    result.throwOnError();
    result.applyToBoard();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
