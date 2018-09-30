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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "./commandlineinterface.h"

#include <librepcb/common/application.h>
#include <librepcb/common/debug.h>

#include <QTranslator>
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;

/*******************************************************************************
 *  Function Prototypes
 ******************************************************************************/

static void setApplicationMetadata() noexcept;
static void installTranslations() noexcept;

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  // --------------------------------- INITIALIZATION
  // ----------------------------------

  // Creates the Debug object which installs the message handler. This must be
  // done as early as possible.
  Debug::instance();

  // Silence debug output, it's a command line tool
  Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Nothing);

  // Create Application instance
  Application app(argc, argv);

  // Set the organization / application names must be done very early because
  // some other classes will use these values (for example QSettings, Debug)!
  setApplicationMetadata();

  // Install translation files. This must be done before any widget is shown.
  installTranslations();

  // --------------------------------- RUN APPLICATION
  // ---------------------------------
  cli::CommandLineInterface cli(app);
  return cli.execute();
}

/*******************************************************************************
 *  setApplicationMetadata()
 ******************************************************************************/

static void setApplicationMetadata() noexcept {
  Application::setOrganizationName("LibrePCB");
  Application::setOrganizationDomain("librepcb.org");
  Application::setApplicationName("LibrePCB CLI");
}

/*******************************************************************************
 *  installTranslations()
 ******************************************************************************/

static void installTranslations() noexcept {
  // Install Qt translations
  QTranslator* qtTranslator = new QTranslator(qApp);
  qtTranslator->load("qt_" % QLocale::system().name(),
                     QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  qApp->installTranslator(qtTranslator);

  // Install system language translations (all system languages defined in the
  // system settings, in the defined order)
  const QString dir              = qApp->getResourcesFilePath("i18n").toStr();
  QTranslator*  systemTranslator = new QTranslator(qApp);
  systemTranslator->load(QLocale::system(), "librepcb", "_", dir);
  qApp->installTranslator(systemTranslator);

  // Install language translations (like "de" for German)
  QTranslator* appTranslator1 = new QTranslator(qApp);
  appTranslator1->load("librepcb_" % QLocale::system().name().split("_").at(0),
                       dir);
  qApp->installTranslator(appTranslator1);

  // Install language/country translations (like "de_ch" for German/Switzerland)
  QTranslator* appTranslator2 = new QTranslator(qApp);
  appTranslator2->load("librepcb_" % QLocale::system().name(), dir);
  qApp->installTranslator(appTranslator2);
}
