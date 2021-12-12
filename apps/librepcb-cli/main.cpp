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
#include "./commandlineinterface.h"

#include <librepcb/core/application.h>
#include <librepcb/core/debug.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  // Creates the Debug object which installs the message handler. This must be
  // done as early as possible.
  Debug::instance();

  // Silence logging output, it's a command line tool where logging messages
  // could lead to issues when parsing the CLI output. Real errors will be
  // printed to stderr explicitly and logging output can optionally be enabled
  // with the "--verbose" flag. But still print fatal errors since this is the
  // only way to print any error to stderr before the application gets aborted.
  Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Fatal);

  // Create Application instance
  Application app(argc, argv);

  // Set the organization / application names must be done very early because
  // some other classes will use these values (for example QSettings, Debug)!
  Application::setOrganizationName("LibrePCB");
  Application::setOrganizationDomain("librepcb.org");
  Application::setApplicationName("LibrePCB CLI");

  // Install translation files. This must be done before any widget is shown.
  app.setTranslationLocale(QLocale::system());

  // Run application
  cli::CommandLineInterface cli(app);
  return cli.execute();
}
