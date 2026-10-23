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
#include <librepcb/automation/boardtools.h>
#include <librepcb/automation/designsession.h>
#include <librepcb/automation/librarytools.h>
#include <librepcb/automation/mcpserver.h>
#include <librepcb/automation/mcpstdiotransport.h>
#include <librepcb/automation/projecttools.h>
#include <librepcb/automation/schematictools.h>
#include <librepcb/core/application.h>
#include <librepcb/core/debug.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  if (qEnvironmentVariableIsEmpty("QT_MAX_CACHED_GLYPH_SIZE")) {
    qputenv("QT_MAX_CACHED_GLYPH_SIZE", "32");
  }

  // Creates the Debug object which installs the message handler. This must be
  // done as early as possible.
  Debug::instance();

#ifdef Q_OS_LINUX
  // The server never shows a window, so a platform or theme set for GUI
  // applications (e.g. "wayland", "gtk3") would only fail where the display is
  // not reachable, like in the sandbox of an agent. The "-platform" argument
  // still overrides it.
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qunsetenv("QT_QPA_PLATFORMTHEME");
#endif

  // Stdout is reserved for protocol messages, and logging output on stderr
  // would only clutter the logs of MCP clients.
  Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Fatal);

  // A QApplication is needed for the graphics scenes some editor commands
  // work on.
  QApplication app(argc, argv);
  QApplication::setOrganizationName("LibrePCB");
  QApplication::setOrganizationDomain("librepcb.org");
  QApplication::setApplicationName("LibrePCB MCP");
  QApplication::setApplicationVersion(Application::getVersion());

  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Model Context Protocol server for LibrePCB on stdin/stdout.");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(app);

  Application::loadBundledFonts();
  Application::setTranslationLocale(QLocale::system());

  automation::McpServer server("librepcb", Application::getVersion());
  automation::DesignSession session;
  automation::addLibraryTools(server, session);
  automation::addProjectTools(server, session);
  automation::addSchematicTools(server, session);
  automation::addBoardTools(server, session);
  automation::McpStdioTransport transport(server);
  QObject::connect(&transport, &automation::McpStdioTransport::finished, &app,
                   &QApplication::quit, Qt::QueuedConnection);
  transport.start();
  return app.exec();
}
