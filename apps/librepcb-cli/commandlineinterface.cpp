/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "commandlineinterface.h"
#include <librepcb/common/application.h>
#include <librepcb/common/debug.h>
#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardgerberexport.h>
#include <librepcb/project/erc/ercmsg.h>
#include <librepcb/project/erc/ercmsglist.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace cli {

using namespace librepcb::project;

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CommandLineInterface::CommandLineInterface(const Application& app) noexcept :
    mApp(app)
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

int CommandLineInterface::execute() noexcept
{
    QMap<QString, QPair<QString, QString>> commands = {
        {
            "open-project", {
                tr("Open a project to execute project-related tasks."),
                tr("open-project [command_options]")
            }
        },
    };

    // Add global options
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("LibrePCB Command Line Interface"));
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();
    QCommandLineOption verboseOption("verbose", tr("Verbose output."));
    parser.addOption(verboseOption);
    parser.addPositionalArgument("command", tr("The command to execute."));

    // Define options for "open-project"
    QCommandLineOption ercOption(
        "erc",
        tr("Run the electrical rule check, print all non-approved warnings/errors and "
           "report failure (exit code = 1) if there are non-approved messages."));
    QCommandLineOption exportSchematicsOption(
        "export-schematics",
        QString(tr("Export schematics to given file(s). Existing files will be "
                   "overwritten. Supported file extensions: %1")).arg("pdf"),
        tr("file"));
    QCommandLineOption exportPcbFabricationDataOption(
        "export-pcb-fabrication-data",
        tr("Export PCB fabrication data (Gerber/Excellon) according the fabrication "
           "output settings of boards. Existing files will be overwritten."));
    QCommandLineOption boardOption(
        "board",
        tr("The name of the board(s) to export. Can be given multiple times. If not set, "
           "all boards are exported."),
        tr("name"));
    QCommandLineOption saveOption(
        "save",
        tr("Save project before closing it (useful to upgrade file format)."));

    // First parse to get the supplied command (ignoring errors because the parser does
    // not yet know the command-dependent options).
    parser.parse(mApp.arguments());

    // Add command-dependent options
    QStringList positionalArgs = parser.positionalArguments();
    QString command = positionalArgs.isEmpty() ? QString() : positionalArgs.first();
    if (positionalArgs.count() > 0) {
        positionalArgs.removeFirst(); // command is now stored in separate variable
    }
    if (command == "open-project") {
        parser.clearPositionalArguments();
        parser.addPositionalArgument(command, commands[command].first, commands[command].second);
        parser.addPositionalArgument("project", tr("Path to project file (*.lpp)."));
        parser.addOption(ercOption);
        parser.addOption(exportSchematicsOption);
        parser.addOption(exportPcbFabricationDataOption);
        parser.addOption(boardOption);
        parser.addOption(saveOption);
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

    // --version
    if (parser.isSet(versionOption)) {
        print(QString(tr("LibrePCB CLI Version %1")).arg(mApp.getAppVersion().toPrettyStr(3)));
        print(QString(tr("Git Revision %1")).arg(mApp.getGitVersion()));
        print(QString(tr("Qt Version %1 (compiled against %2)")).arg(qVersion(), QT_VERSION_STR));
        print(QString(tr("Built at %1")).arg(mApp.getBuildDate().toString(Qt::LocalDate)));
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
            positionalArgs.value(0),                      // project filepath
            parser.isSet(ercOption),                      // run ERC
            parser.values(exportSchematicsOption),        // export schematics
            parser.isSet(exportPcbFabricationDataOption), // export PCB fabrication data
            parser.values(boardOption),                   // boards
            parser.isSet(saveOption)                      // save project
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

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool CommandLineInterface::openProject(const QString& projectFile, bool runErc,
    const QStringList& exportSchematicsFiles, bool exportPcbFabricationData,
    const QStringList& boards, bool save) const noexcept
{
    try {
        bool success = true;

        // Open project
        FilePath projectFp(QFileInfo(projectFile).absoluteFilePath());
        print(QString(tr("Open project '%1'...")).arg(prettyPath(projectFp, projectFile)));
        Project project(projectFp, !save, false); // can throw

        // ERC
        if (runErc) {
            print(tr("Run ERC..."));
            QStringList messages;
            int approvedMsgCount = 0;
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
                    messages.append(QString("    - [%1] %2").arg(severity, msg->getMsg()));
                }
            }
            print("  " % QString(tr("Approved messages: %1")).arg(approvedMsgCount));
            print("  " % QString(tr("Non-approved messages: %1")).arg(messages.count()));
            qSort(messages); // increases readability of console output
            foreach (const QString& msg, messages) {
                printErr(msg);
            }
            if (messages.count() > 0) {
                success = false;
            }
        }

        // Export schematics
        foreach (const QString& destStr, exportSchematicsFiles) {
            print(QString(tr("Export schematics to '%1'...")).arg(destStr));
            QString suffix = destStr.split('.').last().toLower();
            if (suffix == "pdf") {
                QString destPathStr = AttributeSubstitutor::substitute(destStr, &project, [&](const QString& str){
                    return FilePath::cleanFileName(str, FilePath::ReplaceSpaces | FilePath::KeepCase);
                });
                FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());
                project.exportSchematicsAsPdf(destPath); // can throw
                print(QString("  => '%1'").arg(prettyPath(destPath, destPathStr)));
            } else {
                printErr("  " % QString(tr("ERROR: Unknown extension '%1'.")).arg(suffix));
                success = false;
            }
        }

        // Export PCB fabrication data
        if (exportPcbFabricationData) {
            print(tr("Export PCB fabrication data..."));
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
            QHash<FilePath, int> filesCounter;
            bool filesOverwritten = false;
            foreach (const Board* board, boardList) {
                print("  " % QString(tr("Board '%1':")).arg(*board->getName()));
                BoardGerberExport grbExport(*board);
                grbExport.exportAllLayers(); // can throw
                foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
                    filesCounter[fp]++;
                    if (filesCounter[fp] > 1) filesOverwritten = true;
                    print(QString("    => '%1'").arg(prettyPath(fp, projectFile)));
                }
            }
            if (filesOverwritten) {
                printErr("  " % tr("ERROR: Some files were written multiple times! "
                                   "Please make sure that every board uses a different "
                                   "fabrication output path or specify the board to "
                                   "export with the '--board' argument."));
                success = false;
            }
        }

        // Save project
        if (save) {
            print(tr("Save project..."));
            // first save to temporary files, then to original files
            project.save(false); // can throw
            project.save(true); // can throw
        }

        return success;
    } catch (const Exception& e) {
        printErr(QString(tr("ERROR: %1")).arg(e.getMsg()));
        return false;
    }
}

QString CommandLineInterface::prettyPath(const FilePath& path, const QString& style) noexcept
{
    return QFileInfo(style).isRelative()
            ? path.toRelative(FilePath(QDir::currentPath()))
            : path.toStr();
}

void CommandLineInterface::print(const QString& str, int newlines) noexcept
{
    QTextStream s(stdout);
    s << str;
    for (int i = 0; i < newlines; ++i) {
        s << endl;
    }
}

void CommandLineInterface::printErr(const QString& str, int newlines) noexcept
{
    QTextStream s(stderr);
    s << str;
    for (int i = 0; i < newlines; ++i) {
        s << endl;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace cli
} // namespace librepcb
