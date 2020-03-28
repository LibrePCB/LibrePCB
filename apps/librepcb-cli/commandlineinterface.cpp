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
#include "commandlineinterface.h"

#include <librepcb/common/application.h>
#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/common/bom/bom.h>
#include <librepcb/common/bom/bomcsvwriter.h>
#include <librepcb/common/debug.h>
#include <librepcb/common/fileio/csvfile.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/library/elements.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardfabricationoutputsettings.h>
#include <librepcb/project/boards/boardgerberexport.h>
#include <librepcb/project/bomgenerator.h>
#include <librepcb/project/erc/ercmsg.h>
#include <librepcb/project/erc/ercmsglist.h>
#include <librepcb/project/project.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace cli {

using namespace librepcb::library;
using namespace librepcb::project;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CommandLineInterface::CommandLineInterface(const Application& app) noexcept
  : mApp(app) {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int CommandLineInterface::execute() noexcept {
  QMap<QString, QPair<QString, QString>> commands = {
      {"open-project",
       {tr("Open a project to execute project-related tasks."),
        tr("open-project [command_options]")}},
      {"open-library",
       {tr("Open a library to execute library-related tasks."),
        tr("open-library [command_options]")}},
  };

  // Add global options
  QCommandLineParser parser;
  parser.setApplicationDescription(tr("LibrePCB Command Line Interface"));
  const QCommandLineOption helpOption    = parser.addHelpOption();
  const QCommandLineOption versionOption = parser.addVersionOption();
  QCommandLineOption       verboseOption("verbose", tr("Verbose output."));
  parser.addOption(verboseOption);
  parser.addPositionalArgument("command", tr("The command to execute."));

  // Define options for "open-project"
  QCommandLineOption ercOption(
      "erc",
      tr("Run the electrical rule check, print all non-approved "
         "warnings/errors and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption exportSchematicsOption(
      "export-schematics",
      QString(tr("Export schematics to given file(s). Existing files will be "
                 "overwritten. Supported file extensions: %1"))
          .arg("pdf"),
      tr("file"));
  QCommandLineOption exportBomOption(
      "export-bom",
      QString(tr("Export generic BOM to given file(s). Existing files will be "
                 "overwritten. Supported file extensions: %1"))
          .arg("csv"),
      tr("file"));
  QCommandLineOption exportBoardBomOption(
      "export-board-bom",
      QString(tr("Export board-specific BOM to given file(s). Existing files "
                 "will be overwritten. Supported file extensions: %1"))
          .arg("csv"),
      tr("file"));
  QCommandLineOption bomAttributesOption(
      "bom-attributes",
      QString(tr("Comma-separated list of additional attributes to be exported "
                 "to the BOM. Example: \"%1\""))
          .arg("MANUFACTURER, MPN"),
      tr("attributes"));
  QCommandLineOption exportPcbFabricationDataOption(
      "export-pcb-fabrication-data",
      tr("Export PCB fabrication data (Gerber/Excellon) according the "
         "fabrication "
         "output settings of boards. Existing files will be overwritten."));
  QCommandLineOption pcbFabricationSettingsOption(
      "pcb-fabrication-settings",
      tr("Override PCB fabrication output settings by providing a *.lp file "
         "containing custom settings. If not set, the settings from the boards "
         "will be used instead."),
      tr("file"));
  QCommandLineOption boardOption("board",
                                 tr("The name of the board(s) to export. Can "
                                    "be given multiple times. If not set, "
                                    "all boards are exported."),
                                 tr("name"));
  QCommandLineOption saveOption(
      "save",
      tr("Save project before closing it (useful to upgrade file format)."));
  QCommandLineOption prjStrictOption(
      "strict", tr("Fail if the project files are not strictly canonical, i.e. "
                   "there would be changes when saving the project. Note that "
                   "this option is not available for *.lppz files."));

  // Define options for "open-library"
  QCommandLineOption libAllOption(
      "all", tr("Perform the selected action(s) on all elements contained in "
                "the opened library."));
  QCommandLineOption libSaveOption(
      "save", tr("Save library (and contained elements if '--all' is given) "
                 "before closing them (useful to upgrade file format)."));
  QCommandLineOption libStrictOption(
      "strict", tr("Fail if the opened files are not strictly canonical, i.e. "
                   "there would be changes when saving the library elements."));

  // First parse to get the supplied command (ignoring errors because the parser
  // does not yet know the command-dependent options).
  parser.parse(mApp.arguments());

  // Add command-dependent options
  QStringList positionalArgs = parser.positionalArguments();
  QString     command =
      positionalArgs.isEmpty() ? QString() : positionalArgs.first();
  if (positionalArgs.count() > 0) {
    positionalArgs.removeFirst();  // command is now stored in separate variable
  }
  if (command == "open-project") {
    parser.clearPositionalArguments();
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("project",
                                 tr("Path to project file (*.lpp[z])."));
    parser.addOption(ercOption);
    parser.addOption(exportSchematicsOption);
    parser.addOption(exportBomOption);
    parser.addOption(exportBoardBomOption);
    parser.addOption(bomAttributesOption);
    parser.addOption(exportPcbFabricationDataOption);
    parser.addOption(pcbFabricationSettingsOption);
    parser.addOption(boardOption);
    parser.addOption(saveOption);
    parser.addOption(prjStrictOption);
  } else if (command == "open-library") {
    parser.clearPositionalArguments();
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("library",
                                 tr("Path to library directory (*.lplib)."));
    parser.addOption(libAllOption);
    parser.addOption(libSaveOption);
    parser.addOption(libStrictOption);
  } else if (!command.isEmpty()) {
    printErr(QString(tr("Unknown command '%1'.")).arg(command), 2);
    print(parser.helpText(), 0);
    return 1;
  }

  // Parse the actual command line arguments given by the user
  if (!parser.parse(mApp.arguments())) {
    printErr(parser.errorText(), 2);
    print(parser.helpText(), 0);
    return 1;
  }

  // --version
  if (parser.isSet(versionOption)) {
    print(
        QString(tr("LibrePCB CLI Version %1")).arg(mApp.applicationVersion()));
    print(QString(tr("Git Revision %1")).arg(mApp.getGitRevision()));
    print(QString(tr("Qt Version %1 (compiled against %2)"))
              .arg(qVersion(), QT_VERSION_STR));
    print(QString(tr("Built at %1"))
              .arg(mApp.getBuildDate().toString(Qt::LocalDate)));
    return 0;
  }

  // --help (also shown if no command supplied)
  if (parser.isSet(helpOption) || command.isEmpty()) {
    print(parser.helpText(), 0);
    if (command.isEmpty()) {
      print("\n" % tr("Commands:"));
      for (auto it = commands.constBegin(); it != commands.constEnd(); ++it) {
        print("  " % it.key().leftJustified(15) % it.value().first);
      }
    }
    return 0;
  }

  // --verbose
  if (parser.isSet(verboseOption)) {
    Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::All);
  }

  // Execute command
  bool cmdSuccess = false;
  if (command == "open-project") {
    if (positionalArgs.count() != 1) {
      printErr(tr("Wrong argument count."), 2);
      print(parser.helpText(), 0);
      return 1;
    }
    cmdSuccess = openProject(
        positionalArgs.value(0),                       // project filepath
        parser.isSet(ercOption),                       // run ERC
        parser.values(exportSchematicsOption),         // export schematics
        parser.values(exportBomOption),                // export generic BOM
        parser.values(exportBoardBomOption),           // export board BOM
        parser.value(bomAttributesOption),             // BOM attributes
        parser.isSet(exportPcbFabricationDataOption),  // export PCB fab. data
        parser.value(pcbFabricationSettingsOption),    // PCB fab. settings
        parser.values(boardOption),                    // boards
        parser.isSet(saveOption),                      // save project
        parser.isSet(prjStrictOption)                  // strict mode
    );
  } else if (command == "open-library") {
    if (positionalArgs.count() != 1) {
      printErr(tr("Wrong argument count."), 2);
      print(parser.helpText(), 0);
      return 1;
    }
    cmdSuccess = openLibrary(positionalArgs.value(0),       // library directory
                             parser.isSet(libAllOption),    // all elements
                             parser.isSet(libSaveOption),   // save
                             parser.isSet(libStrictOption)  // strict mode
    );
  } else {
    printErr(tr("Internal failure."));
  }
  if (cmdSuccess) {
    print(tr("SUCCESS"));
    return 0;
  } else {
    print(tr("Finished with errors!"));
    return 1;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool CommandLineInterface::openProject(
    const QString& projectFile, bool runErc,
    const QStringList& exportSchematicsFiles, const QStringList& exportBomFiles,
    const QStringList& exportBoardBomFiles, const QString& bomAttributes,
    bool exportPcbFabricationData, const QString& pcbFabricationSettingsPath,
    const QStringList& boards, bool save, bool strict) const noexcept {
  try {
    bool                success = true;
    QMap<FilePath, int> writtenFilesCounter;

    // Open project
    FilePath projectFp(QFileInfo(projectFile).absoluteFilePath());
    print(QString(tr("Open project '%1'..."))
              .arg(prettyPath(projectFp, projectFile)));
    std::shared_ptr<TransactionalFileSystem> projectFs;
    QString                                  projectFileName;
    if (projectFp.getSuffix() == "lppz") {
      projectFs = TransactionalFileSystem::openRO(projectFp.getParentDir());
      projectFs->removeDirRecursively();  // 1) get a clean initial state
      projectFs->loadFromZip(projectFp);  // 2) load files from ZIP
      foreach (const QString& fn, projectFs->getFiles()) {
        if (fn.endsWith(".lpp")) {
          projectFileName = fn;
        }
      }
    } else {
      projectFs = TransactionalFileSystem::open(projectFp.getParentDir(), save);
      projectFileName = projectFp.getFilename();
    }
    Project project(std::unique_ptr<TransactionalDirectory>(
                        new TransactionalDirectory(projectFs)),
                    projectFileName);  // can throw

    // Check for non-canonical files (strict mode)
    if (strict) {
      print(tr("Check for non-canonical files..."));
      if (projectFp.getSuffix() == "lppz") {
        printErr("  " % tr("ERROR: The option '--strict' is not available for "
                           "*.lppz files!"));
        success = false;
      } else {
        project.save();                                          // can throw
        QStringList paths = projectFs->checkForModifications();  // can throw
        // ignore user config files
        paths = paths.filter(QRegularExpression("^((?!\\.user\\.lp).)*$"));
        // sort file paths to increases readability of console output
        std::sort(paths.begin(), paths.end());
        foreach (const QString& path, paths) {
          printErr(
              QString("    - Non-canonical file: %1")
                  .arg(prettyPath(projectFs->getAbsPath(path), projectFile)));
        }
        if (paths.count() > 0) {
          success = false;
        }
      }
    }

    // ERC
    if (runErc) {
      print(tr("Run ERC..."));
      QStringList messages;
      int         approvedMsgCount = 0;
      foreach (const ErcMsg* msg, project.getErcMsgList().getItems()) {
        if (!msg->isVisible()) continue;
        if (msg->isIgnored()) {
          ++approvedMsgCount;
        } else {
          QString severity;
          switch (msg->getMsgType()) {
            case ErcMsg::ErcMsgType_t::CircuitWarning:
            case ErcMsg::ErcMsgType_t::SchematicWarning:
            case ErcMsg::ErcMsgType_t::BoardWarning:
              severity = tr("WARNING");
              break;
            default:
              severity = tr("ERROR");
              break;
          }
          messages.append(
              QString("    - [%1] %2").arg(severity, msg->getMsg()));
        }
      }
      print("  " % QString(tr("Approved messages: %1")).arg(approvedMsgCount));
      print("  " %
            QString(tr("Non-approved messages: %1")).arg(messages.count()));
      // sort messages to increases readability of console output
      std::sort(messages.begin(), messages.end());
      foreach (const QString& msg, messages) { printErr(msg); }
      if (messages.count() > 0) {
        success = false;
      }
    }

    // Export schematics
    foreach (const QString& destStr, exportSchematicsFiles) {
      print(QString(tr("Export schematics to '%1'...")).arg(destStr));
      QString suffix = destStr.split('.').last().toLower();
      if (suffix == "pdf") {
        QString destPathStr = AttributeSubstitutor::substitute(
            destStr, &project, [&](const QString& str) {
              return FilePath::cleanFileName(
                  str, FilePath::ReplaceSpaces | FilePath::KeepCase);
            });
        FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());
        project.exportSchematicsAsPdf(destPath);  // can throw
        print(QString("  => '%1'").arg(prettyPath(destPath, destPathStr)));
        writtenFilesCounter[destPath]++;
      } else {
        printErr("  " %
                 QString(tr("ERROR: Unknown extension '%1'.")).arg(suffix));
        success = false;
      }
    }

    // Parse list of boards
    QList<Board*> boardList;
    if (boards.isEmpty()) {
      // export all boards
      boardList = project.getBoards();
    } else {
      // export specified boards
      foreach (const QString& boardName, boards) {
        Board* board = project.getBoardByName(boardName);
        if (board) {
          boardList.append(board);
        } else {
          printErr(QString(tr("ERROR: No board with the name '%1' found."))
                       .arg(boardName));
          success = false;
        }
      }
    }

    // Export BOM
    if (exportBomFiles.count() + exportBoardBomFiles.count() > 0) {
      QList<QPair<QString, bool>> jobs;  // <OutputPath, BoardSpecific>
      foreach (const QString& fp, exportBomFiles) {
        jobs.append(qMakePair(fp, false));
      }
      foreach (const QString& fp, exportBoardBomFiles) {
        jobs.append(qMakePair(fp, true));
      }
      QStringList attributes;
      foreach (const QString str,
               bomAttributes.simplified().split(',', QString::SkipEmptyParts)) {
        attributes.append(str.trimmed());
      }
      foreach (const auto& job, jobs) {
        const QString& destStr       = job.first;
        bool           boardSpecific = job.second;
        if (boardSpecific) {
          print(
              QString(tr("Export board-specific BOM to '%1'...")).arg(destStr));
        } else {
          print(QString(tr("Export generic BOM to '%1'...")).arg(destStr));
        }
        QList<Board*> boards =
            boardSpecific ? boardList : QList<Board*>{nullptr};
        foreach (const Board* board, boards) {
          const AttributeProvider* attrProvider = board;
          if (!board) {
            attrProvider = &project;
          }
          QString destPathStr = AttributeSubstitutor::substitute(
              destStr, attrProvider, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              });
          FilePath     fp(QFileInfo(destPathStr).absoluteFilePath());
          BomGenerator gen(project);
          gen.setAdditionalAttributes(attributes);
          std::shared_ptr<Bom> bom = gen.generate(board);
          if (board) {
            print(QString("  - '%1' => '%2'")
                      .arg(*board->getName(), prettyPath(fp, destPathStr)));
          } else {
            print(QString("  => '%1'").arg(prettyPath(fp, destPathStr)));
          }
          QString suffix = destStr.split('.').last().toLower();
          if (suffix == "csv") {
            BomCsvWriter             writer(*bom);
            std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
            csv->saveToFile(fp);                                  // can throw
            writtenFilesCounter[fp]++;
          } else {
            printErr("  " %
                     QString(tr("ERROR: Unknown extension '%1'.")).arg(suffix));
            success = false;
          }
        }
      }
    }

    // Export PCB fabrication data
    if (exportPcbFabricationData) {
      print(tr("Export PCB fabrication data..."));
      tl::optional<BoardFabricationOutputSettings> customSettings;
      if (!pcbFabricationSettingsPath.isEmpty()) {
        try {
          qDebug() << "Load custom fabrication output settings:"
                   << pcbFabricationSettingsPath;
          FilePath fp(QFileInfo(pcbFabricationSettingsPath).absoluteFilePath());
          customSettings = BoardFabricationOutputSettings(
              SExpression::parse(FileUtils::readFile(fp), fp));  // can throw
        } catch (const Exception& e) {
          printErr(QString(tr("ERROR: Failed to load custom settings: %1"))
                       .arg(e.getMsg()));
          success = false;
          boardList.clear();  // avoid exporting any boards
        }
      }
      foreach (const Board* board, boardList) {
        print("  " % QString(tr("Board '%1':")).arg(*board->getName()));
        BoardGerberExport grbExport(
            *board, customSettings ? *customSettings
                                   : board->getFabricationOutputSettings());
        grbExport.exportAllLayers();  // can throw
        foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
          print(QString("    => '%1'").arg(prettyPath(fp, projectFile)));
          writtenFilesCounter[fp]++;
        }
      }
    }

    // Save project
    if (save) {
      print(tr("Save project..."));
      project.save();  // can throw
      if (projectFp.getSuffix() == "lppz") {
        projectFs->exportToZip(projectFp);  // can throw
      } else {
        projectFs->save();  // can throw
      }
    }

    // Fail if some files were written multiple times
    bool                        filesOverwritten = false;
    QMapIterator<FilePath, int> writtenFilesIterator(writtenFilesCounter);
    while (writtenFilesIterator.hasNext()) {
      writtenFilesIterator.next();
      if (writtenFilesIterator.value() > 1) {
        filesOverwritten = true;
        printErr(QString(tr("ERROR: The file %1 was written multiple times!"))
                     .arg(prettyPath(writtenFilesIterator.key(), projectFile)));
      }
    }
    if (filesOverwritten) {
      printErr(QString(tr("NOTE: To avoid writing files multiple times, make "
                          "sure to pass unique filepaths to all export "
                          "functions. For board output files, you could either "
                          "add the placeholder '%1' to the path or specify the "
                          "boards to export with the '%2' argument."))
                   .arg("'{{BOARD}}'", "--board"));
      success = false;
    }

    return success;
  } catch (const Exception& e) {
    printErr(QString(tr("ERROR: %1")).arg(e.getMsg()));
    return false;
  }
}

bool CommandLineInterface::openLibrary(const QString& libDir, bool all,
                                       bool save, bool strict) const noexcept {
  try {
    bool success = true;

    // Open library
    FilePath libFp(QFileInfo(libDir).absoluteFilePath());
    print(QString(tr("Open library '%1'...")).arg(prettyPath(libFp, libDir)));

    std::shared_ptr<TransactionalFileSystem> libFs =
        TransactionalFileSystem::open(libFp, save);  // can throw
    Library lib(std::unique_ptr<TransactionalDirectory>(
        new TransactionalDirectory(libFs)));  // can throw
    processLibraryElement(libDir, *libFs, lib, save, strict,
                          success);  // can throw

    // Open all component categories
    if (all) {
      QStringList elements = lib.searchForElements<ComponentCategory>();
      print(QString(tr("Process %1 component categories..."))
                .arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        ComponentCategory element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    // Open all package categories
    if (all) {
      QStringList elements = lib.searchForElements<PackageCategory>();
      print(QString(tr("Process %1 package categories..."))
                .arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        PackageCategory element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    // Open all symbols
    if (all) {
      QStringList elements = lib.searchForElements<Symbol>();
      print(QString(tr("Process %1 symbols...")).arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        Symbol element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    // Open all packages
    if (all) {
      QStringList elements = lib.searchForElements<Package>();
      print(QString(tr("Process %1 packages...")).arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        Package element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    // Open all components
    if (all) {
      QStringList elements = lib.searchForElements<Component>();
      print(QString(tr("Process %1 components...")).arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        Component element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    // Open all devices
    if (all) {
      QStringList elements = lib.searchForElements<Device>();
      print(QString(tr("Process %1 devices...")).arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo() << QString(tr("Open '%1'...")).arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        Device element(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, element, save, strict,
                              success);  // can throw
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(QString(tr("ERROR: %1")).arg(e.getMsg()));
    return false;
  }
}

void CommandLineInterface::processLibraryElement(const QString& libDir,
                                                 TransactionalFileSystem& fs,
                                                 LibraryBaseElement& element,
                                                 bool save, bool strict,
                                                 bool& success) const {
  // Save element to transactional file system, if needed
  if (strict || save) {
    element.save();  // can throw
  }

  // Check for non-canonical files (strict mode)
  if (strict) {
    qInfo() << QString(tr("Check '%1' for non-canonical files..."))
                   .arg(prettyPath(fs.getPath(), libDir));

    QStringList paths = fs.checkForModifications();  // can throw
    // sort file paths to increases readability of console output
    std::sort(paths.begin(), paths.end());
    foreach (const QString& path, paths) {
      printErr(QString("    - Non-canonical file: %1")
                   .arg(prettyPath(fs.getAbsPath(path), libDir)));
    }
    if (paths.count() > 0) {
      success = false;
    }
  }

  // Save element to file system, if needed
  if (save) {
    qInfo()
        << QString(tr("Save '%1'...")).arg(prettyPath(fs.getPath(), libDir));
    fs.save();  // can throw
  }

  // Do not propagate changes in the transactional file system to the
  // following checks
  fs.discardChanges();
}

QString CommandLineInterface::prettyPath(const FilePath& path,
                                         const QString&  style) noexcept {
  if (QFileInfo(style).isAbsolute()) {
    return path.toStr();  // absolute path
  } else if (path == FilePath(QDir::currentPath())) {
    return path.getFilename();  // name of current directory
  } else {
    return path.toRelative(FilePath(QDir::currentPath()));  // relative path
  }
}

void CommandLineInterface::print(const QString& str, int newlines) noexcept {
  QTextStream s(stdout);
  s << str;
  for (int i = 0; i < newlines; ++i) {
    s << endl;
  }
}

void CommandLineInterface::printErr(const QString& str, int newlines) noexcept {
  QTextStream s(stderr);
  s << str;
  for (int i = 0; i < newlines; ++i) {
    s << endl;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace cli
}  // namespace librepcb
