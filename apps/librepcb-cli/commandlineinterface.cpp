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

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/debug.h>
#include <librepcb/core/export/bom.h>
#include <librepcb/core/export/bomcsvwriter.h>
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/export/pickplacecsvwriter.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/footprintpainter.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/library/sym/symbolpainter.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardd356netlistexport.h>
#include <librepcb/core/project/board/boardfabricationoutputsettings.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/board/boardpickplacegenerator.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/bomgenerator.h>
#include <librepcb/core/project/circuit/assemblyvariant.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace cli {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CommandLineInterface::CommandLineInterface() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int CommandLineInterface::execute(const QStringList& args) noexcept {
  QStringList positionalArgNames;
  QMap<QString, QPair<QString, QString>> commands = {
      {"open-project",
       {tr("Open a project to execute project-related tasks."),
        "open-project [command_options]"}},  // no tr()!
      {"open-library",
       {tr("Open a library to execute library-related tasks."),
        "open-library [command_options]"}},  // no tr()!
      {"open-package",
       {tr("Open a package to execute package-related tasks."),
        "open-package [command_options]"}},  // no tr()!
      {"open-symbol",
       {tr("Open a symbol to execute symbol-related tasks."),
        "open-symbol [command_options]"}},  // no tr()!
      {"open-step",
       {tr("Open a STEP model to execute STEP-related tasks outside of a "
           "library."),
        "open-step [command_options]"}},  // no tr()!
  };

  // Add global options
  QCommandLineParser parser;
  parser.setApplicationDescription(tr("LibrePCB Command Line Interface"));
  // Don't use the built-in addHelpOption() since it also adds the "--help-all"
  // option which we don't need, and the OS-dependent option "-?".
  const QCommandLineOption helpOption({"h", "help"}, tr("Print this message."));
  parser.addOption(helpOption);
  const QCommandLineOption versionOption({"V", "version"},
                                         tr("Displays version information."));
  parser.addOption(versionOption);
  QCommandLineOption verboseOption({"v", "verbose"}, tr("Verbose output."));
  parser.addOption(verboseOption);
  parser.addPositionalArgument("command",
                               tr("The command to execute (see list below)."));
  positionalArgNames.append("command");

  // Define options for "open-project"
  QCommandLineOption ercOption(
      "erc",
      tr("Run the electrical rule check, print all non-approved "
         "warnings/errors and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption drcOption(
      "drc",
      tr("Run the design rule check, print all non-approved warnings/errors "
         "and report failure (exit code = 1) if there are non-approved "
         "messages."));
  QCommandLineOption drcSettingsOption(
      "drc-settings",
      tr("Override DRC settings by providing a *.lp file containing custom "
         "settings. If not set, the settings from the boards will be used "
         "instead."),
      tr("file"));
  QCommandLineOption runSpecificJobOption(
      "run-job",
      tr("Run a particular output job. Can be given multiple times to run "
         "multiple jobs."),
      tr("name"));
  QCommandLineOption runAllJobsOption("run-jobs",
                                      tr("Run all existing output jobs."));
  QCommandLineOption customJobsOption(
      "jobs",
      tr("Override output jobs with a *.lp file containing custom jobs. If not "
         "set, the jobs from the project will be used instead."),
      tr("file"));
  QCommandLineOption customOutDirOption(
      "outdir",
      tr("Override the output base directory of jobs. If not set, the "
         "standard output directory from the project is used."),
      tr("path"));
  QCommandLineOption exportSchematicsOption(
      "export-schematics",
      tr("Export schematics to given file(s). Existing files will be "
         "overwritten. Supported file extensions: %1")
          .arg(GraphicsExport::getSupportedExtensions().join(", ")),
      tr("file"));
  QCommandLineOption exportBomOption(
      "export-bom",
      tr("Export generic BOM to given file(s). Existing files will be "
         "overwritten. Supported file extensions: %1")
          .arg("csv"),
      tr("file"));
  QCommandLineOption exportBoardBomOption(
      "export-board-bom",
      tr("Export board-specific BOM to given file(s). Existing files "
         "will be overwritten. Supported file extensions: %1")
          .arg("csv"),
      tr("file"));
  QCommandLineOption bomAttributesOption(
      "bom-attributes",
      tr("Comma-separated list of additional attributes to be exported "
         "to the BOM. Example: \"%1\"")
          .arg("SUPPLIER, SKU"),
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
  QCommandLineOption exportPnpTopOption(
      "export-pnp-top",
      tr("Export pick&place file for automated assembly of the top board side. "
         "Existing files will be overwritten. Supported file extensions: %1")
          .arg("csv, gbr"),
      tr("file"));
  QCommandLineOption exportPnpBottomOption(
      "export-pnp-bottom",
      tr("Export pick&place file for automated assembly of the bottom board "
         "side. Existing files will be overwritten. Supported file extensions: "
         "%1")
          .arg("csv, gbr"),
      tr("file"));
  QCommandLineOption exportNetlistOption(
      "export-netlist",
      tr("Export netlist file for automated PCB testing. Existing files will "
         "be overwritten. Supported file extensions: %1")
          .arg("d356"),
      tr("file"));
  QCommandLineOption boardOption("board",
                                 tr("The name of the board(s) to export. Can "
                                    "be given multiple times. If not set, "
                                    "all boards are exported."),
                                 tr("name"));
  QCommandLineOption boardIndexOption("board-index",
                                      tr("Same as '%1', but allows to specify "
                                         "boards by index instead of by name.")
                                          .arg("--board"),
                                      tr("index"));
  QCommandLineOption removeOtherBoardsOption(
      "remove-other-boards",
      tr("Remove all boards not specified with '%1' from the project before "
         "executing all the other actions. If '%1' is not passed, all boards "
         "will be removed. Pass '%2' to save the modified project to disk.")
          .arg("--board[-index]")
          .arg("--save"));
  QCommandLineOption assemblyVariantOption(
      "variant",
      tr("The name of the assembly variant(s) to export. Can be given multiple "
         "times. If not set, all assembly variants are exported."),
      tr("name"));
  QCommandLineOption assemblyVariantIndexOption(
      "variant-index",
      tr("Same as '%1', but allows to specify assembly variants by index "
         "instead of by name.")
          .arg("--variant"),
      tr("index"));
  QCommandLineOption setDefaultAssemblyVariantOption(
      "set-default-variant",
      tr("Move the specified assembly variant to the top before executing all "
         "the other actions. Pass '%1' to save the modified project to disk.")
          .arg("--save"),
      tr("name"));
  QCommandLineOption saveOption(
      "save",
      tr("Save project before closing it (useful to upgrade file format)."));
  QCommandLineOption prjStrictOption(
      "strict",
      tr("Fail if the project files are not strictly canonical, i.e. "
         "there would be changes when saving the project. Note that "
         "this option is not available for *.lppz files."));

  // Define options for "open-library"
  QCommandLineOption libAllOption(
      "all",
      tr("Perform the selected action(s) on all elements contained in "
         "the opened library."));
  QCommandLineOption libCheckOption(
      "check",
      tr("Run the library element check, print all non-approved messages and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption libMinifyStepOption(
      "minify-step",
      tr("Minify the STEP models of all packages. Only works in conjunction "
         "with '--all'. Pass '--save' to write the minified files to disk."));
  QCommandLineOption libSaveOption(
      "save",
      tr("Save library (and contained elements if '--all' is given) "
         "before closing them (useful to upgrade file format)."));
  QCommandLineOption libStrictOption(
      "strict",
      tr("Fail if the opened files are not strictly canonical, i.e. "
         "there would be changes when saving the library elements."));

  // Define options for "open-symbol"
  QCommandLineOption symCheckOption(
      "check",
      tr("Run the symbol check, print all non-approved messages and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption symExportOption(
      "export",
      tr("Export the symbol to a graphical "
         "file. Supported file extensions: %1")
          .arg(GraphicsExport::getSupportedExtensions().join(", ")),
      tr("file"));

  // Define options for "open-package"
  QCommandLineOption pkgCheckOption(
      "check",
      tr("Run the package check, print all non-approved messages and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption pkgExportOption(
      "export",
      tr("Export the contained footprint(s) to a graphical "
         "file. Supported file extensions: %1")
          .arg(GraphicsExport::getSupportedExtensions().join(", ")),
      tr("file"));

  // Define options for "open-step"
  QCommandLineOption stepMinifyOption(
      "minify",
      tr("Minify the STEP model before validating it. Use in conjunction with "
         "'%1' to save the output of the operation.")
          .arg("--save-to"));
  QCommandLineOption stepTesselateOption(
      "tesselate",
      tr("Tesselate the loaded STEP model to check if LibrePCB is able to "
         "render it. Reports failure (exit code = 1) if no content is "
         "detected."));
  QCommandLineOption stepSaveToOption(
      "save-to",
      tr("Write the (modified) STEP file to this output location (may be equal "
         "to the opened file path). Only makes sense in conjunction with '%1'.")
          .arg("--minify"),
      tr("file"));

  // Build help text.
  const QString executable = args.value(0);
  QString helpText = parser.helpText() % "\n" % tr("Commands:") % "\n";
  for (auto it = commands.constBegin(); it != commands.constEnd(); ++it) {
    helpText += "  " % it.key().leftJustified(15) % it.value().first % "\n";
  }
  helpText += "\n" % tr("List command-specific options:") % "\n  " %
      executable % " <command> --help";
  QString usageHelpText = helpText.split("\n").value(0);
  const QString helpCommandTextPrefix = tr("Help:") % " ";
  QString helpCommandText = helpCommandTextPrefix % executable % " --help";

  // First parse to get the supplied command (ignoring errors because the parser
  // does not yet know the command-dependent options).
  parser.parse(args);

  // Add command-dependent options
  const QString command = parser.positionalArguments().value(0);
  parser.clearPositionalArguments();
  if (command == "open-project") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("project",
                                 tr("Path to project file (*.lpp[z])."));
    positionalArgNames.append("project");
    parser.addOption(ercOption);
    parser.addOption(drcOption);
    parser.addOption(drcSettingsOption);
    parser.addOption(runSpecificJobOption);
    parser.addOption(runAllJobsOption);
    parser.addOption(customJobsOption);
    parser.addOption(customOutDirOption);
    parser.addOption(exportSchematicsOption);
    parser.addOption(exportBomOption);
    parser.addOption(exportBoardBomOption);
    parser.addOption(bomAttributesOption);
    parser.addOption(exportPcbFabricationDataOption);
    parser.addOption(pcbFabricationSettingsOption);
    parser.addOption(exportPnpTopOption);
    parser.addOption(exportPnpBottomOption);
    parser.addOption(exportNetlistOption);
    parser.addOption(boardOption);
    parser.addOption(boardIndexOption);
    parser.addOption(removeOtherBoardsOption);
    parser.addOption(assemblyVariantOption);
    parser.addOption(assemblyVariantIndexOption);
    parser.addOption(setDefaultAssemblyVariantOption);
    parser.addOption(saveOption);
    parser.addOption(prjStrictOption);
  } else if (command == "open-library") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("library",
                                 tr("Path to library directory (*.lplib)."));
    positionalArgNames.append("library");
    parser.addOption(libAllOption);
    parser.addOption(libCheckOption);
    parser.addOption(libMinifyStepOption);
    parser.addOption(libSaveOption);
    parser.addOption(libStrictOption);
  } else if (command == "open-symbol") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument(
        "symbol", tr("Path to symbol directory (containing *.lp)."));
    positionalArgNames.append("symbol");
    parser.addOption(symCheckOption);
    parser.addOption(symExportOption);
  } else if (command == "open-package") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument(
        "package", tr("Path to package directory (containing *.lp)."));
    positionalArgNames.append("package");
    parser.addOption(pkgCheckOption);
    parser.addOption(pkgExportOption);
  } else if (command == "open-step") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument(
        "file", tr("Path to the STEP file (%1).").arg("*.step"));
    positionalArgNames.append("file");
    parser.addOption(stepMinifyOption);
    parser.addOption(stepTesselateOption);
    parser.addOption(stepSaveToOption);
  } else if (!command.isEmpty()) {
    printErr(tr("Unknown command '%1'.").arg(command));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // If a command is given, make the help texts command-specific now.
  if (!command.isEmpty()) {
    helpText = parser.helpText().trimmed();  // Remove the list of commands.
    usageHelpText = helpText.split("\n").value(0);
    helpCommandText =
        helpCommandTextPrefix % executable % " " % command % " --help";
  }

  // Parse the actual command line arguments given by the user
  if (!parser.parse(args)) {
    printErr(parser.errorText());
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // --verbose
  if (parser.isSet(verboseOption)) {
    Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::All);
    OccModel::setVerboseOutput(true);
  }

  // --help (also shown if no arguments supplied)
  if (parser.isSet(helpOption) || (args.count() <= 1)) {
    print(helpText);
    return 0;
  }

  // --version
  if (parser.isSet(versionOption)) {
    // Note: Do not translate this output as it probably looks ugly and this
    // way it is deterministic even if LANG/LC_ALL is not explicitly set.
    QString revision = Application::getGitRevision();
    const QDate date = Application::getGitCommitDate().date();
    if (date.isValid()) {
      revision += " (" + date.toString(Qt::ISODate) + ")";
    }
    print(QString("LibrePCB CLI Version %1").arg(Application::getVersion()));
    print(QString("File Format %1")
              .arg(Application::getFileFormatVersion().toStr()) %
          " " %
          (Application::isFileFormatStable() ? "(stable)" : "(unstable)"));
    print(QString("Git Revision %1").arg(revision));
    print(QString("Qt Version %1 (compiled against %2)")
              .arg(qVersion(), QT_VERSION_STR));
    print("OpenCascade " % OccModel::getOccVersionString());
    return 0;
  }

  // Check number of passed positional command arguments.
  const QStringList positionalArgs = parser.positionalArguments();
  if (positionalArgs.count() < positionalArgNames.count()) {
    const QStringList names = positionalArgNames.mid(positionalArgs.count());
    printErr(tr("Missing arguments:") % " " % names.join(" "));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  } else if (positionalArgs.count() > positionalArgNames.count()) {
    const QStringList args = positionalArgs.mid(positionalArgNames.count());
    printErr(tr("Unknown arguments:") % " " % args.join(" "));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // Execute command
  bool cmdSuccess = false;
  if (command == "open-project") {
    cmdSuccess = openProject(
        positionalArgs.value(1),  // project filepath
        parser.isSet(ercOption),  // run ERC
        parser.isSet(drcOption),  // run DRC
        parser.value(drcSettingsOption),  // DRC settings
        parser.values(runSpecificJobOption),  // run specific output jobs
        parser.isSet(runAllJobsOption),  // run all output jobs
        parser.value(customJobsOption).trimmed(),  // custom jobs file path
        parser.value(customOutDirOption).trimmed(),  // custom jobs outdir
        parser.values(exportSchematicsOption),  // export schematics
        parser.values(exportBomOption),  // export generic BOM
        parser.values(exportBoardBomOption),  // export board BOM
        parser.value(bomAttributesOption),  // BOM attributes
        parser.isSet(exportPcbFabricationDataOption),  // export PCB fab. data
        parser.value(pcbFabricationSettingsOption),  // PCB fab. settings
        parser.values(exportPnpTopOption),  // export PnP top
        parser.values(exportPnpBottomOption),  // export PnP bottom
        parser.values(exportNetlistOption),  // export netlist
        parser.values(boardOption),  // board names
        parser.values(boardIndexOption),  // board indices
        parser.isSet(removeOtherBoardsOption),  // remove other boards
        parser.values(assemblyVariantOption),  // assembly variant names
        parser.values(assemblyVariantIndexOption),  // assembly variant indices
        parser.value(setDefaultAssemblyVariantOption),  // set default AV
        parser.isSet(saveOption),  // save project
        parser.isSet(prjStrictOption)  // strict mode
    );
  } else if (command == "open-library") {
    cmdSuccess = openLibrary(positionalArgs.value(1),  // library directory
                             parser.isSet(libAllOption),  // all elements
                             parser.isSet(libCheckOption),  // run check
                             parser.isSet(libMinifyStepOption),  // minify STEP
                             parser.isSet(libSaveOption),  // save
                             parser.isSet(libStrictOption)  // strict mode
    );
  } else if (command == "open-package") {
    cmdSuccess = openPackage(positionalArgs.value(1),  // package directory
                             parser.isSet(pkgCheckOption),  // run check
                             parser.value(pkgExportOption)  // export file
    );
  } else if (command == "open-symbol") {
    cmdSuccess = openSymbol(positionalArgs.value(1),  // symbol directory
                            parser.isSet(symCheckOption),  // run check
                            parser.value(symExportOption)  // export file
    );
  } else if (command == "open-step") {
    cmdSuccess = openStep(positionalArgs.value(1),  // STEP file path
                          parser.isSet(stepMinifyOption),  // minify
                          parser.isSet(stepTesselateOption),  // tesselate
                          parser.value(stepSaveToOption)  // save to
    );
  } else {
    printErr("Internal failure.");  // No tr() because this cannot occur.
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
    const QString& projectFile, bool runErc, bool runDrc,
    const QString& drcSettingsPath, const QStringList& runJobs, bool runAllJobs,
    const QString& customJobsPath, const QString& customOutDir,
    const QStringList& exportSchematicsFiles, const QStringList& exportBomFiles,
    const QStringList& exportBoardBomFiles, const QString& bomAttributes,
    bool exportPcbFabricationData, const QString& pcbFabricationSettingsPath,
    const QStringList& exportPnpTopFiles,
    const QStringList& exportPnpBottomFiles,
    const QStringList& exportNetlistFiles, const QStringList& boardNames,
    const QStringList& boardIndices, bool removeOtherBoards,
    const QStringList& avNames, const QStringList& avIndices,
    const QString& setDefaultAv, bool save, bool strict) const noexcept {
  try {
    bool success = true;
    QMap<FilePath, int> writtenFilesCounter;
    QMap<FilePath, int> writtenOutputJobFilesCounter;

    // Open project
    FilePath projectFp(QFileInfo(projectFile).absoluteFilePath());
    print(tr("Open project '%1'...").arg(prettyPath(projectFp, projectFile)));
    std::shared_ptr<TransactionalFileSystem> projectFs;
    QString projectFileName;
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
    ProjectLoader loader;
    std::unique_ptr<Project> project =
        loader.open(std::unique_ptr<TransactionalDirectory>(
                        new TransactionalDirectory(projectFs)),
                    projectFileName);  // can throw
    if (auto messages = loader.getUpgradeMessages()) {
      print(tr("Attention: Project has been upgraded to a newer file format!"));
      std::sort(messages->begin(), messages->end(),
                [](const FileFormatMigration::Message& a,
                   const FileFormatMigration::Message& b) {
                  if (a.severity > b.severity) return true;
                  if (a.message < b.message) return true;
                  return false;
                });
      foreach (const auto& msg, *messages) {
        const QString multiplier = msg.affectedItems > 0
            ? QString(" (%1x)").arg(msg.affectedItems)
            : "";
        print(QString(" - %1%2: %3")
                  .arg(msg.getSeverityStrTr())
                  .arg(multiplier)
                  .arg(msg.message));
      }
    }

    // Set the default assembly variant.
    if (!setDefaultAv.isEmpty()) {
      print(tr("Set default assembly variant to '%1'...").arg(setDefaultAv));
      if (project->getCircuit().getAssemblyVariants().contains(setDefaultAv)) {
        project->getCircuit().getAssemblyVariants().insert(
            0, project->getCircuit().getAssemblyVariants().take(setDefaultAv));
      } else {
        printErr(tr("ERROR: No assembly variant with the name '%1' found.")
                     .arg(setDefaultAv));
        success = false;
      }
    }

    // Parse list of assembly variants.
    QVector<std::shared_ptr<AssemblyVariant>> assemblyVariants;
    foreach (const QString& avName, avNames) {
      if (auto av = project->getCircuit().getAssemblyVariants().find(avName)) {
        if (!assemblyVariants.contains(av)) {
          assemblyVariants.append(av);
        }
      } else {
        printErr(tr("ERROR: No assembly variant with the name '%1' found.")
                     .arg(avName));
        success = false;
      }
    }
    foreach (const QString& avIndex, avIndices) {
      bool ok = false;
      const int index = avIndex.trimmed().toInt(&ok);
      auto av = project->getCircuit().getAssemblyVariants().value(index);
      if (ok && av) {
        if (!assemblyVariants.contains(av)) {
          assemblyVariants.append(av);
        }
      } else {
        printErr(
            tr("ERROR: Assembly variant index '%1' is invalid.").arg(avIndex));
        success = false;
      }
    }

    // If no assembly variants are specified, export all variants.
    if (avNames.isEmpty() && avIndices.isEmpty()) {
      for (auto it = project->getCircuit().getAssemblyVariants().begin();
           it != project->getCircuit().getAssemblyVariants().end(); ++it) {
        assemblyVariants.append(it.ptr());
      }
    }

    // Parse list of boards.
    QList<Board*> boards;
    foreach (const QString& boardName, boardNames) {
      if (Board* board = project->getBoardByName(boardName)) {
        if (!boards.contains(board)) {
          boards.append(board);
        }
      } else {
        printErr(
            tr("ERROR: No board with the name '%1' found.").arg(boardName));
        success = false;
      }
    }
    foreach (const QString& boardIndex, boardIndices) {
      bool ok = false;
      const int index = boardIndex.trimmed().toInt(&ok);
      Board* board = project->getBoardByIndex(index);
      if (ok && board) {
        if (!boards.contains(board)) {
          boards.append(board);
        }
      } else {
        printErr(tr("ERROR: Board index '%1' is invalid.").arg(boardIndex));
        success = false;
      }
    }

    // Remove other boards (note: do this at the very beginning to make all
    // the other commands, e.g. the ERC, working without the removed boards).
    if (removeOtherBoards) {
      print(tr("Remove other boards..."));
      foreach (Board* board, project->getBoards()) {
        if (!boards.contains(board)) {
          print(QString("  - '%1'").arg(*board->getName()));
          project->removeBoard(*board);
        }
      }
    }

    // If no boards are specified, export all boards.
    if (boardNames.isEmpty() && boardIndices.isEmpty()) {
      boards = project->getBoards();
    }

    // Build planes, if needed.
    if (runDrc || exportPcbFabricationData || (!runJobs.isEmpty()) ||
        runAllJobs) {
      foreach (Board* board, boards) {
        qInfo().nospace().noquote() << "Rebuilding all planes of board '"
                                    << *board->getName() << "'...";
        BoardPlaneFragmentsBuilder builder;
        builder.runAndApply(*board);  // can throw
      }
    } else {
      qInfo() << "No need to rebuild planes, thus skipped.";
    }

    // Check for non-canonical files (strict mode)
    if (strict) {
      print(tr("Check for non-canonical files..."));
      if (projectFp.getSuffix() == "lppz") {
        printErr("  " %
                 tr("ERROR: The option '--strict' is not available for "
                    "*.lppz files!"));
        success = false;
      } else {
        project->save();  // can throw
        QStringList paths = projectFs->checkForModifications();  // can throw
        // ignore user config files
        paths = paths.filter(QRegularExpression("^((?!\\.user\\.lp).)*$"));
        // sort file paths to increases readability of console output
        std::sort(paths.begin(), paths.end());
        foreach (const QString& path, paths) {
          printErr(
              QString("    - Non-canonical file: '%1'")
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
      ElectricalRuleCheck erc(*project);
      int approvedMsgCount = 0;
      const RuleCheckMessageList messages = erc.runChecks();
      const QStringList nonApproved = prepareRuleCheckMessages(
          messages, project->getErcMessageApprovals(), approvedMsgCount);

      // Print summary using shared formatting
      QStringList summaryMessages =
          formatCheckSummary(approvedMsgCount, nonApproved.count(), "  ");
      foreach (const QString& msg, summaryMessages) {
        print(msg);
      }

      foreach (const QString& msg, nonApproved) {
        printErr("    - " % msg);
        success = false;
      }
    }

    // DRC
    if (runDrc) {
      print(tr("Run DRC..."));
      std::optional<BoardDesignRuleCheckSettings> customSettings;
      QList<Board*> boardsToCheck = boards;
      if (!drcSettingsPath.isEmpty()) {
        try {
          qDebug() << "Load custom DRC settings:" << drcSettingsPath;
          const FilePath fp(QFileInfo(drcSettingsPath).absoluteFilePath());
          const std::unique_ptr<const SExpression> root =
              SExpression::parse(FileUtils::readFile(fp), fp);
          customSettings = BoardDesignRuleCheckSettings(*root);  // can throw
        } catch (const Exception& e) {
          printErr(
              tr("ERROR: Failed to load custom settings: %1").arg(e.getMsg()));
          success = false;
          boardsToCheck.clear();  // avoid exporting any boards
        }
      }
      foreach (Board* board, boardsToCheck) {
        print("  " % tr("Board '%1':").arg(*board->getName()));
        BoardDesignRuleCheck drc;
        drc.start(*board,
                  customSettings ? *customSettings : board->getDrcSettings(),
                  false);
        const BoardDesignRuleCheck::Result result = drc.waitForFinished();
        for (const QString& msg : result.errors) {
          printErr("FATAL ERROR: " % msg);
          success = false;
        }
        int approvedMsgCount = 0;
        const QStringList nonApproved = prepareRuleCheckMessages(
            result.messages, board->getDrcMessageApprovals(), approvedMsgCount);

        // Print summary using shared formatting
        QStringList summaryMessages =
            formatCheckSummary(approvedMsgCount, nonApproved.count(), "    ");
        foreach (const QString& msg, summaryMessages) {
          print(msg);
        }

        foreach (const QString& msg, nonApproved) {
          printErr("      - " % msg);
          success = false;
        }
      }
    }

    // Run output jobs.
    if ((!runJobs.isEmpty()) || runAllJobs) {
      // Determine jobs.
      std::optional<OutputJobList> allJobs;
      if (!customJobsPath.isEmpty()) {
        try {
          qDebug() << "Load custom output jobs:" << customJobsPath;
          const FilePath fp(QFileInfo(customJobsPath).absoluteFilePath());
          const std::unique_ptr<const SExpression> root =
              SExpression::parse(FileUtils::readFile(fp), fp);
          allJobs = deserialize<OutputJobList>(*root);  // can throw
        } catch (const Exception& e) {
          printErr(tr("ERROR: Failed to load custom output jobs: %1")
                       .arg(e.getMsg()));
          success = false;
        }
      } else {
        allJobs = project->getOutputJobs();
      }
      if (allJobs) {
        QVector<std::shared_ptr<OutputJob>> jobs;
        if (runAllJobs) {
          jobs = allJobs->values();
        } else {
          foreach (const QString& name, runJobs) {
            if (auto job = allJobs->find(name)) {
              jobs.append(job);
            } else {
              printErr(tr("ERROR: No output job with the name '%1' found.")
                           .arg(name));
              success = false;
            }
          }
        }
        try {
          OutputJobRunner runner(*project);
          QObject::connect(
              &runner, &OutputJobRunner::jobStarted,
              [](std::shared_ptr<const OutputJob> job) {
                print(tr("Run output job '%1'...").arg(*job->getName()));
              });
          QObject::connect(
              &runner, &OutputJobRunner::aboutToWriteFile,
              [&projectFile,
               &writtenOutputJobFilesCounter](const FilePath& fp) {
                print(QString("  => '%1'").arg(prettyPath(fp, projectFile)));
                writtenOutputJobFilesCounter[fp]++;
              });
          if (!customOutDir.isEmpty()) {
            runner.setOutputDirectory(
                QDir::isRelativePath(customOutDir)
                    ? FilePath(QDir::currentPath()).getPathTo(customOutDir)
                    : FilePath(customOutDir));
          }
          qDebug() << "Using output base directory:"
                   << runner.getOutputDirectory().toNative();
          runner.run(jobs);  // can throw
        } catch (const Exception& e) {
          printErr(tr("ERROR:") % " " % e.getMsg());
          success = false;
        }
      }
    }

    // Export schematics
    foreach (const QString& destStr, exportSchematicsFiles) {
      print(tr("Export schematics to '%1'...").arg(destStr));
      QString destPathStr = AttributeSubstitutor::substitute(
          destStr, ProjectAttributeLookup(*project, nullptr),
          [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          });
      FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());
      GraphicsExport graphicsExport;
      graphicsExport.setDocumentName(*project->getName());
      std::shared_ptr<GraphicsExportSettings> settings =
          std::make_shared<GraphicsExportSettings>();
      GraphicsExport::Pages pages;
      foreach (const Schematic* schematic, project->getSchematics()) {
        pages.append(std::make_pair(
            std::make_shared<SchematicPainter>(*schematic), settings));
      }
      graphicsExport.startExport(pages, destPath);
      const GraphicsExport::Result result = graphicsExport.waitForFinished();
      foreach (const FilePath& writtenFile, result.writtenFiles) {
        print(QString("  => '%1'").arg(prettyPath(writtenFile, destPathStr)));
        writtenFilesCounter[writtenFile]++;
      }
      if (!result.errorMsg.isEmpty()) {
        printErr("  " % tr("ERROR") % ": " % result.errorMsg);
        success = false;
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
      if (bomAttributes.isEmpty()) {
        attributes = project->getCustomBomAttributes();
      } else {
        foreach (const QString str, bomAttributes.simplified().split(',')) {
          if (!str.trimmed().isEmpty()) {
            attributes.append(str.trimmed());
          }
        }
      }
      foreach (const auto& job, jobs) {
        QList<Board*> boardsToExport;
        const QString& destStr = job.first;
        bool boardSpecific = job.second;
        if (boardSpecific) {
          print(tr("Export board-specific BOM to '%1'...").arg(destStr));
          boardsToExport = boards;
        } else {
          print(tr("Export generic BOM to '%1'...").arg(destStr));
          boardsToExport = {nullptr};
        }
        foreach (const Board* board, boardsToExport) {
          foreach (std::shared_ptr<AssemblyVariant> av, assemblyVariants) {
            QString destPathStr = AttributeSubstitutor::substitute(
                destStr,
                board ? ProjectAttributeLookup(*board, av)
                      : ProjectAttributeLookup(*project, av),
                [&](const QString& str) {
                  return FilePath::cleanFileName(
                      str, FilePath::ReplaceSpaces | FilePath::KeepCase);
                });
            FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
            BomGenerator gen(*project);
            gen.setAdditionalAttributes(attributes);
            std::shared_ptr<Bom> bom = gen.generate(board, av->getUuid());
            if (board) {
              print(QString("  - '%1' => '%2'")
                        .arg(*board->getName(), prettyPath(fp, destPathStr)));
            } else {
              print(QString("  => '%1'").arg(prettyPath(fp, destPathStr)));
            }
            QString suffix = destStr.split('.').last().toLower();
            if (suffix == "csv") {
              BomCsvWriter writer(*bom);
              std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
              csv->saveToFile(fp);  // can throw
              writtenFilesCounter[fp]++;
            } else {
              printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
              success = false;
            }
          }
        }
      }
    }

    // Export PCB fabrication data
    if (exportPcbFabricationData) {
      print(tr("Export PCB fabrication data..."));
      std::optional<BoardFabricationOutputSettings> customSettings;
      QList<Board*> boardsToExport = boards;
      if (!pcbFabricationSettingsPath.isEmpty()) {
        try {
          qDebug() << "Load custom fabrication output settings:"
                   << pcbFabricationSettingsPath;
          const FilePath fp(
              QFileInfo(pcbFabricationSettingsPath).absoluteFilePath());
          const std::unique_ptr<const SExpression> root =
              SExpression::parse(FileUtils::readFile(fp), fp);
          customSettings = BoardFabricationOutputSettings(*root);  // can throw
        } catch (const Exception& e) {
          printErr(
              tr("ERROR: Failed to load custom settings: %1").arg(e.getMsg()));
          success = false;
          boardsToExport.clear();  // avoid exporting any boards
        }
      }
      foreach (const Board* board, boardsToExport) {
        print("  " % tr("Board '%1':").arg(*board->getName()));
        BoardGerberExport grbExport(*board);
        grbExport.exportPcbLayers(
            customSettings
                ? *customSettings
                : board->getFabricationOutputSettings());  // can throw
        foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
          print(QString("    => '%1'").arg(prettyPath(fp, projectFile)));
          writtenFilesCounter[fp]++;
        }
      }
    }

    // Export pick&place files
    if ((exportPnpTopFiles.count() + exportPnpBottomFiles.count()) > 0) {
      struct Job {
        QString boardSideStr;
        PickPlaceCsvWriter::BoardSide boardSideCsv;
        BoardGerberExport::BoardSide boardSideGbr;
        QString destStr;
      };
      QVector<Job> jobs;
      foreach (const QString& fp, exportPnpTopFiles) {
        jobs.append(Job{tr("top"), PickPlaceCsvWriter::BoardSide::Top,
                        BoardGerberExport::BoardSide::Top, fp});
      }
      foreach (const QString& fp, exportPnpBottomFiles) {
        jobs.append(Job{tr("bottom"), PickPlaceCsvWriter::BoardSide::Bottom,
                        BoardGerberExport::BoardSide::Bottom, fp});
      }
      foreach (const auto& job, jobs) {
        print(tr("Export %1 assembly data to '%2'...")
                  .arg(job.boardSideStr)
                  .arg(job.destStr));
        foreach (const Board* board, boards) {
          foreach (std::shared_ptr<AssemblyVariant> av, assemblyVariants) {
            const QString destPathStr = AttributeSubstitutor::substitute(
                job.destStr, ProjectAttributeLookup(*board, av),
                [&](const QString& str) {
                  return FilePath::cleanFileName(
                      str, FilePath::ReplaceSpaces | FilePath::KeepCase);
                });
            const FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
            print(QString("  - '%1' => '%2'")
                      .arg(*board->getName(), prettyPath(fp, destPathStr)));
            const QString suffix = job.destStr.split('.').last().toLower();
            if (suffix == "csv") {
              BoardPickPlaceGenerator gen(*board, av->getUuid());
              std::shared_ptr<PickPlaceData> data = gen.generate();
              PickPlaceCsvWriter writer(*data);
              writer.setIncludeMetadataComment(true);
              writer.setBoardSide(job.boardSideCsv);
              std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
              csv->saveToFile(fp);  // can throw
              writtenFilesCounter[fp]++;
            } else if (suffix == "gbr") {
              BoardGerberExport gen(*board);
              gen.exportComponentLayer(job.boardSideGbr, av->getUuid(),
                                       fp);  // can throw
              writtenFilesCounter[fp]++;
            } else {
              printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
              success = false;
            }
          }
        }
      }
    }

    // Export netlist files
    foreach (const QString& destStr, exportNetlistFiles) {
      print(tr("Export netlist to '%1'...").arg(destStr));
      foreach (const Board* board, boards) {
        QString destPathStr = AttributeSubstitutor::substitute(
            destStr, ProjectAttributeLookup(*board, nullptr),
            [&](const QString& str) {
              return FilePath::cleanFileName(
                  str, FilePath::ReplaceSpaces | FilePath::KeepCase);
            });
        const FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
        print(QString("  - '%1' => '%2'")
                  .arg(*board->getName(), prettyPath(fp, destPathStr)));
        const QString suffix = destStr.split('.').last().toLower();
        if (suffix == "d356") {
          BoardD356NetlistExport exp(*board);
          FileUtils::writeFile(fp, exp.generate());  // can throw
          writtenFilesCounter[fp]++;
        } else {
          printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
          success = false;
        }
      }
    }

    // Save project
    if (save) {
      print(tr("Save project..."));
      if (failIfFileFormatUnstable()) {
        success = false;
      } else {
        project->save();  // can throw
        if (projectFp.getSuffix() == "lppz") {
          projectFs->exportToZip(projectFp);  // can throw
        } else {
          projectFs->save();  // can throw
        }
      }
    }

    // Fail if some files were written multiple times
    bool filesOverwritten = false;
    for (auto it = writtenFilesCounter.begin(); it != writtenFilesCounter.end();
         ++it) {
      const int totalWritten =
          it.value() + writtenOutputJobFilesCounter.value(it.key(), 0);
      if (totalWritten > 1) {
        filesOverwritten = true;
        printErr(tr("ERROR: The file '%1' was written multiple times!")
                     .arg(prettyPath(it.key(), projectFile)));
      }
    }
    if (filesOverwritten) {
      printErr(tr("NOTE: To avoid writing files multiple times, make "
                  "sure to pass unique filepaths to all export "
                  "functions. For board output files, you could either "
                  "add the placeholder '%1' to the path or specify the "
                  "boards to export with the '%2' argument.")
                   .arg("{{BOARD}}", "--board"));
      success = false;
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

bool CommandLineInterface::openLibrary(const QString& libDir, bool all,
                                       bool runCheck, bool minifyStepFiles,
                                       bool save, bool strict) const noexcept {
  try {
    bool success = true;

    // Open library
    FilePath libFp(QFileInfo(libDir).absoluteFilePath());
    print(tr("Open library '%1'...").arg(prettyPath(libFp, libDir)));

    std::shared_ptr<TransactionalFileSystem> libFs =
        TransactionalFileSystem::open(libFp, save);  // can throw
    std::unique_ptr<Library> lib =
        Library::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(libFs)));  // can throw
    processLibraryElement(libDir, *libFs, *lib, runCheck, minifyStepFiles, save,
                          strict,
                          success);  // can throw

    // Open all component categories
    if (all) {
      QStringList elements = lib->searchForElements<ComponentCategory>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 component categories...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<ComponentCategory> element =
            ComponentCategory::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    // Open all package categories
    if (all) {
      QStringList elements = lib->searchForElements<PackageCategory>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 package categories...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<PackageCategory> element =
            PackageCategory::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    // Open all symbols
    if (all) {
      QStringList elements = lib->searchForElements<Symbol>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 symbols...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Symbol> element =
            Symbol::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    // Open all packages
    if (all) {
      QStringList elements = lib->searchForElements<Package>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 packages...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Package> element =
            Package::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    // Open all components
    if (all) {
      QStringList elements = lib->searchForElements<Component>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 components...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Component> element =
            Component::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    // Open all devices
    if (all) {
      QStringList elements = lib->searchForElements<Device>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 devices...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Device> element =
            Device::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, minifyStepFiles,
                              save, strict,
                              success);  // can throw
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

CommandLineInterface::CheckResult
    CommandLineInterface::gatherElementCheckMessages(
        const LibraryBaseElement& element) const {
  CheckResult result;
  result.approvedMsgCount = 0;
  const RuleCheckMessageList messages = element.runChecks();
  result.nonApprovedMessages = prepareRuleCheckMessages(
      messages, element.getMessageApprovals(), result.approvedMsgCount);
  return result;
}

QStringList CommandLineInterface::formatCheckSummary(
    const FilePath& path, const QString& relPath,
    const CheckResult& checkResult) const {
  QStringList messages;
  messages << tr("Check '%1' for non-approved messages...")
                  .arg(prettyPath(path, relPath));
  messages += formatCheckSummary(checkResult.approvedMsgCount,
                                 checkResult.nonApprovedMessages.count(), "  ");
  return messages;
}

QStringList CommandLineInterface::formatCheckSummary(
    int approvedCount, int nonApprovedCount, const QString& indent) const {
  QStringList messages;
  messages << indent % tr("Approved messages: %1").arg(approvedCount);
  messages << indent % tr("Non-approved messages: %1").arg(nonApprovedCount);
  return messages;
}

void CommandLineInterface::processLibraryElement(
    const QString& libDir, TransactionalFileSystem& fs,
    LibraryBaseElement& element, bool runCheck, bool minifyStepFiles, bool save,
    bool strict, bool& success) const {
  // Keep track of whether we've yet printed the error header for this element
  bool errorHeaderPrinted = false;
  auto printErrorHeaderOnce = [&errorHeaderPrinted, &element]() {
    if (!errorHeaderPrinted) {
      printErr(QString("  - %1 (%2):")
                   .arg(*element.getNames().getDefaultValue(),
                        element.getUuid().toStr()));
      errorHeaderPrinted = true;
    }
  };

  // Save element to transactional file system, if needed
  if (strict || save) {
    element.save();  // can throw
  }

  // Minify STEP files, if needed.
  if (minifyStepFiles && dynamic_cast<Package*>(&element)) {
    foreach (const QString& file, fs.getFiles()) {
      if (file.endsWith(".step")) {
        const QString fp = prettyPath(fs.getAbsPath(file), libDir);
        qInfo().noquote() << tr("Minify STEP model '%1'...").arg(fp);
        try {
          const QByteArray content = fs.read(file);  // can throw
          const QByteArray minified =
              OccModel::minifyStep(content);  // can throw
          if (minified != content) {
            print(tr("  - Minified '%1' from %2 to %3 bytes")
                      .arg(fp)
                      .arg(content.size())
                      .arg(minified.size()));
            OccModel::loadStep(minified);  // throws if STEP is invalid
            fs.write(file, minified);
          }
        } catch (const Exception& e) {
          printErrorHeaderOnce();
          printErr(QString("    - Failed to minify STEP model '%1': %2")
                       .arg(fp, e.getMsg()));
          success = false;
        }
      }
    }
  }

  // Check for non-canonical files (strict mode)
  if (strict) {
    qInfo().noquote() << tr("Check '%1' for non-canonical files...")
                             .arg(prettyPath(fs.getPath(), libDir));

    QStringList paths = fs.checkForModifications();  // can throw
    if (!paths.isEmpty()) {
      // sort file paths to increases readability of console output
      std::sort(paths.begin(), paths.end());
      printErrorHeaderOnce();
      foreach (const QString& path, paths) {
        printErr(QString("    - Non-canonical file: '%1'")
                     .arg(prettyPath(fs.getAbsPath(path), libDir)));
      }
      success = false;
    }
  }

  // Run library element check, if needed.
  if (runCheck) {
    // Gather messages
    CheckResult checkResult = gatherElementCheckMessages(element);

    // Print summary to qInfo (stderr) for libraries
    QStringList summaryMessages =
        formatCheckSummary(fs.getPath(), libDir, checkResult);
    foreach (const QString& msg, summaryMessages) {
      qInfo().noquote() << msg;
    }

    // If we have non-approved messages, print the header once, then all
    // messages
    foreach (const QString& msg, checkResult.nonApprovedMessages) {
      printErrorHeaderOnce();
      printErr("    - " % msg);
      success = false;
    }
  }

  // Save element to file system, if needed
  if (save) {
    qInfo().noquote()
        << tr("Save '%1'...").arg(prettyPath(fs.getPath(), libDir));
    if (failIfFileFormatUnstable()) {
      success = false;
    } else {
      fs.save();  // can throw
    }
  }

  // Do not propagate changes in the transactional file system to the
  // following checks
  fs.discardChanges();
}

bool CommandLineInterface::openSymbol(
    const QString& symbolFile, bool runCheck,
    const QString& exportFile) const noexcept {
  try {
    bool success = true;

    // Open symbol directory (similar to openPackage)
    FilePath symbolFp(QFileInfo(symbolFile).absoluteFilePath());
    print(tr("Open symbol '%1'...").arg(prettyPath(symbolFp, symbolFile)));

    std::shared_ptr<TransactionalFileSystem> symbolFs =
        TransactionalFileSystem::open(symbolFp, false);  // can throw
    std::unique_ptr<Symbol> symbol =
        Symbol::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(symbolFs)));  // can throw

    qInfo().noquote()
        << tr("Opened symbol: %1").arg(*symbol->getNames().getDefaultValue());
    // Process the symbol element (validation checks)
    if (runCheck) {
      // Gather messages
      CheckResult checkResult = gatherElementCheckMessages(*symbol);

      // Print summary to stdout for individual elements
      QStringList summaryMessages =
          formatCheckSummary(symbolFs->getPath(), symbolFile, checkResult);
      foreach (const QString& msg, summaryMessages) {
        print(msg);
      }

      foreach (const QString& msg, checkResult.nonApprovedMessages) {
        printErr("  - " % msg);
        success = false;
      }
    }

    // Export symbol to graphics file
    if (!exportFile.isEmpty()) {
      print(tr("Export symbol to '%1'...").arg(exportFile));
      // Generate output filename
      QString destPathStr = exportFile;

      // Apply attribute substitution
      auto lookupFunc = [&symbol](const QString& key) -> QString {
        if (key == QLatin1String("SYMBOL")) {
          return *symbol->getNames().getDefaultValue();
        } else if (key == QLatin1String("SYMBOL_UUID")) {
          return symbol->getUuid().toStr();
        }
        return QString();  // Unknown attribute
      };
      destPathStr = AttributeSubstitutor::substitute(
          exportFile, lookupFunc, [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          });

      // Create absolute file path
      FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());

      // Set up graphics export
      GraphicsExport graphicsExport;
      graphicsExport.setDocumentName(*symbol->getNames().getDefaultValue());

      // Create export settings
      std::shared_ptr<GraphicsExportSettings> settings =
          std::make_shared<GraphicsExportSettings>();
      settings->setMarginLeft(UnsignedLength(0));
      settings->setMarginTop(UnsignedLength(0));
      settings->setMarginRight(UnsignedLength(0));
      settings->setMarginBottom(UnsignedLength(0));

      // Create pages with symbol painter
      GraphicsExport::Pages pages;
      pages.append(
          std::make_pair(std::make_shared<SymbolPainter>(*symbol), settings));

      // Start export and wait for completion
      graphicsExport.startExport(pages, destPath);
      const GraphicsExport::Result result = graphicsExport.waitForFinished();

      // Report results
      foreach (const FilePath& writtenFile, result.writtenFiles) {
        print(QString("  => '%1'").arg(prettyPath(writtenFile, destPathStr)));
      }
      if (!result.errorMsg.isEmpty()) {
        printErr("  " % tr("ERROR") % ": " % result.errorMsg);
        success = false;
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

bool CommandLineInterface::openPackage(
    const QString& packageFile, bool runCheck,
    const QString& exportFile) const noexcept {
  try {
    bool success = true;
    QMap<FilePath, int> writtenFilesCounter;

    // Open package directory (similar to openLibrary)
    FilePath packageFp(QFileInfo(packageFile).absoluteFilePath());
    print(tr("Open package '%1'...").arg(prettyPath(packageFp, packageFile)));

    std::shared_ptr<TransactionalFileSystem> packageFs =
        TransactionalFileSystem::open(packageFp, false);  // can throw
    std::unique_ptr<Package> package =
        Package::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(packageFs)));  // can throw

    qInfo().noquote()
        << tr("Package name: %1").arg(*package->getNames().getDefaultValue());

    // Process the package element (validation checks)
    if (runCheck) {
      // Gather messages
      CheckResult checkResult = gatherElementCheckMessages(*package);

      // Print summary to stdout for individual elements
      QStringList summaryMessages =
          formatCheckSummary(packageFs->getPath(), packageFile, checkResult);
      foreach (const QString& msg, summaryMessages) {
        print(msg);
      }

      foreach (const QString& msg, checkResult.nonApprovedMessages) {
        printErr("  - " % msg);
        success = false;
      }
    }

    // Export package to graphics file
    if (!exportFile.isEmpty()) {
      print(tr("Export footprint(s) to '%1'...").arg(exportFile));

      // Export each footprint
      QMap<FilePath, int> writtenFilesCounter;
      int index = 1;
      for (auto it = package->getFootprints().begin();
           it != package->getFootprints().end(); ++it) {
        const std::shared_ptr<Footprint>& footprint = it.ptr();
        // Generate output filename
        QString destPathStr = exportFile;

        // Apply attribute substitution
        auto lookupFunc = [&package, &footprint,
                           index](const QString& key) -> QString {
          if (key == QLatin1String("PACKAGE")) {
            return *package->getNames().getDefaultValue();
          } else if (key == QLatin1String("PACKAGE_UUID")) {
            return package->getUuid().toStr();
          } else if (key == QLatin1String("FOOTPRINT")) {
            return *footprint->getNames().getDefaultValue();
          } else if (key == QLatin1String("FOOTPRINT_UUID")) {
            return footprint->getUuid().toStr();
          } else if (key == QLatin1String("FOOTPRINT_INDEX")) {
            return QString::number(index);
          }
          return QString();  // Unknown attribute
        };
        destPathStr = AttributeSubstitutor::substitute(
            exportFile, lookupFunc, [&](const QString& str) {
              return FilePath::cleanFileName(
                  str, FilePath::ReplaceSpaces | FilePath::KeepCase);
            });

        // Create absolute file path
        FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());

        // Set up graphics export
        GraphicsExport graphicsExport;
        graphicsExport.setDocumentName(
            QString("%1 (%2)")
                .arg(*package->getNames().getDefaultValue())
                .arg(*footprint->getNames().getDefaultValue()));

        // Create export settings
        std::shared_ptr<GraphicsExportSettings> settings =
            std::make_shared<GraphicsExportSettings>();
        settings->setMarginLeft(UnsignedLength(0));
        settings->setMarginTop(UnsignedLength(0));
        settings->setMarginRight(UnsignedLength(0));
        settings->setMarginBottom(UnsignedLength(0));

        // Create pages with footprint painter
        GraphicsExport::Pages pages;
        pages.append(std::make_pair(
            std::make_shared<FootprintPainter>(*footprint), settings));

        // Start export and wait for completion
        graphicsExport.startExport(pages, destPath);
        const GraphicsExport::Result result = graphicsExport.waitForFinished();

        // Report results
        foreach (const FilePath& writtenFile, result.writtenFiles) {
          print(QString("  => '%1'").arg(prettyPath(writtenFile, destPathStr)));
          writtenFilesCounter[writtenFile]++;
        }
        if (!result.errorMsg.isEmpty()) {
          printErr("  " % tr("ERROR") % ": " % result.errorMsg);
          success = false;
        }

        index++;
      }

      // Fail if some files were written multiple times
      bool filesOverwritten = false;
      for (auto it = writtenFilesCounter.begin();
           it != writtenFilesCounter.end(); ++it) {
        if (it.value() > 1) {
          filesOverwritten = true;
          printErr(tr("ERROR: The file '%1' was written multiple times!")
                       .arg(prettyPath(it.key(), packageFile)));
        }
      }
      if (filesOverwritten) {
        printErr(tr("NOTE: To avoid writing files multiple times, make sure to "
                    "pass unique filepaths "
                    "to all export functions. For package output files, you "
                    "could add a placeholder "
                    "like '%1' to the path.")
                     .arg("{{FOOTPRINT}}"));
        success = false;
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

bool CommandLineInterface::openStep(const QString& filePath, bool minify,
                                    bool tesselate,
                                    const QString& saveTo) const noexcept {
  try {
    // Note: Not using tr() for this command as it is basically intended for
    // developers, not end users.

    bool success = true;

    // Open file.
    const FilePath stepFp(QFileInfo(filePath).absoluteFilePath());
    print(QString("Open STEP file '%1'...").arg(prettyPath(stepFp, filePath)));
    QByteArray stepContent = FileUtils::readFile(stepFp);  // can throw

    // Minify before validation.
    if (minify) {
      print("Perform minify...");
      const QByteArray minified =
          OccModel::minifyStep(stepContent);  // can throw
      if (minified != stepContent) {
        const qreal percent = 100 * (minified.size() - stepContent.size()) /
            qreal(stepContent.size());
        QLocale locale = QLocale::c();
        locale.setNumberOptions(QLocale::DefaultNumberOptions);
        print(QString(" - Minified from %1 bytes to %2 bytes (%3%)")
                  .arg(locale.toString(stepContent.size()))
                  .arg(locale.toString(minified.size()))
                  .arg(percent, 0, 'f', 0));
        if (minified.size() > stepContent.size()) {
          printErr(" - ERROR: The output is larger than the input!");
          success = false;
        }
        stepContent = minified;
      } else {
        print(" - File is already minified");
      }
    }

    // Write to output *before* validating it, otherwise it won't be possible
    // to inspect the invalid result of the minify operation.
    if (!saveTo.isEmpty()) {
      const FilePath outFp(QFileInfo(saveTo).absoluteFilePath());
      print(QString("Save to '%1'...").arg(prettyPath(outFp, saveTo)));
      FileUtils::writeFile(outFp, stepContent);  // can throw
    }

    // Validate.
    print("Load model...");
    std::unique_ptr<OccModel> model =
        OccModel::loadStep(stepContent);  // throws if STEP is invalid

    // Tesselate.
    if (tesselate) {
      print("Tesselate model...");
      const QMap<OccModel::Color, QVector<QVector3D>> vertices =
          model->tesselate();  // can throw
      int vertexCount = 0;
      foreach (const auto& v, vertices) {
        vertexCount += v.count();
      }
      print(QString(" - Built %1 vertices with %2 different colors")
                .arg(vertexCount)
                .arg(vertices.count()));
      if (vertexCount == 0) {
        printErr(" - ERROR: No content found in model!");
        success = false;
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

QStringList CommandLineInterface::prepareRuleCheckMessages(
    RuleCheckMessageList messages, const QSet<SExpression>& approvals,
    int& approvedMsgCount) noexcept {
  // Sort messages to increases readability of console output.
  Toolbox::sortNumeric(
      messages,
      [](const QCollator& cmp,
         const std::shared_ptr<const RuleCheckMessage>& lhs,
         const std::shared_ptr<const RuleCheckMessage>& rhs) {
        if (lhs->getSeverity() != rhs->getSeverity()) {
          return lhs->getSeverity() > rhs->getSeverity();
        } else {
          return cmp(lhs->getMessage(), rhs->getMessage());
        }
      },
      Qt::CaseInsensitive, false);
  approvedMsgCount = 0;
  QStringList printedMessages;
  foreach (const auto& msg, messages) {
    if (approvals.contains(msg->getApproval())) {
      ++approvedMsgCount;
    } else {
      printedMessages.append(QString("[%1] %2").arg(
          msg->getSeverityTr().toUpper(), msg->getMessage()));
    }
  }
  return printedMessages;
}

QString CommandLineInterface::prettyPath(const FilePath& path,
                                         const QString& style) noexcept {
  if (QFileInfo(style).isAbsolute()) {
    // absolute path
    return path.toNative();
  } else if (path == FilePath(QDir::currentPath())) {
    // name of current directory
    return path.getFilename();
  } else {
    // relative path
    return path.toRelativeNative(FilePath(QDir::currentPath()));
  }
}

bool CommandLineInterface::failIfFileFormatUnstable() noexcept {
  if ((!Application::isFileFormatStable()) &&
      (qgetenv("LIBREPCB_DISABLE_UNSTABLE_WARNING") != "1")) {
    printErr(
        tr("This application version is UNSTABLE! Option '%1' is disabled to "
           "avoid breaking projects or libraries. Please use a stable "
           "release instead.")
            .arg("--save"));
    return true;
  } else {
    qInfo() << "Application version is unstable, but warning is disabled with "
               "environment variable LIBREPCB_DISABLE_UNSTABLE_WARNING.";
    return false;
  }
}

void CommandLineInterface::print(const QString& str) noexcept {
  QTextStream s(stdout);
  s << str << '\n';
}

void CommandLineInterface::printErr(const QString& str) noexcept {
  QTextStream s(stderr);
  s << str << '\n';
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace cli
}  // namespace librepcb
