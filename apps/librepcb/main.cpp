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
#include "controlpanel/controlpanel.h"
#include "firstrunwizard/firstrunwizard.h"

#include <librepcb/common/application.h>
#include <librepcb/common/debug.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/network/networkaccessmanager.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QTranslator>
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;
using namespace librepcb::workspace;
using namespace librepcb::application;

/*******************************************************************************
 *  Function Prototypes
 ******************************************************************************/

static void     setApplicationMetadata() noexcept;
static void     configureApplicationSettings() noexcept;
static void     writeLogHeader() noexcept;
static void     installTranslations() noexcept;
static void     init3rdPartyLibs() noexcept;
static void     cleanup3rdPartyLibs() noexcept;
static bool     isFileFormatStableOrAcceptUnstable() noexcept;
static FilePath determineWorkspacePath() noexcept;
static int      openWorkspace(const FilePath& path) noexcept;
static int      appExec() noexcept;

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  Application app(argc, argv);

  // --------------------------------- INITIALIZATION
  // ----------------------------------

  // Set the organization / application names must be done very early because
  // some other classes will use these values (for example QSettings, Debug)!
  setApplicationMetadata();

  // Creates the Debug object which installs the message handler. This must be
  // done as early as possible, but *after* setting application metadata
  // (organization + name).
  Debug::instance();

  // Configure the application settings format and location used by QSettings
  configureApplicationSettings();

  // Write some information about the application instance to the log.
  writeLogHeader();

  // Install translation files. This must be done before any widget is shown.
  installTranslations();

  // This is to remove the ugly frames around widgets in all status bars...
  // (from http://www.qtcentre.org/threads/1904)
  app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

  // Initialize all 3rd party libraries
  init3rdPartyLibs();

  // Start network access manager thread
  QScopedPointer<NetworkAccessManager> networkAccessManager(
      new NetworkAccessManager());

  // --------------------------------- OPEN WORKSPACE
  // ----------------------------------

  int retval = 0;

  // If the file format is unstable (e.g. for nightly builds), ask to abort now.
  // This warning *must* come that early to be really sure that no files are
  // overwritten with unstable content!
  if (isFileFormatStableOrAcceptUnstable()) {
    // Get the path of the workspace to open (may show the first run wizard)
    FilePath wsPath = determineWorkspacePath();

    // Open the workspace and catch the return value
    if (wsPath.isValid()) {
      retval = openWorkspace(wsPath);
    }
  }

  // -------------------------------- EXIT APPLICATION
  // ---------------------------------

  // Stop network access manager thread
  networkAccessManager.reset();

  // Cleanup all 3rd party libraries
  cleanup3rdPartyLibs();

  qDebug() << "Exit application with code" << retval;
  return retval;
}

/*******************************************************************************
 *  setApplicationMetadata()
 ******************************************************************************/

static void setApplicationMetadata() noexcept {
  Application::setOrganizationName("LibrePCB");
  Application::setOrganizationDomain("librepcb.org");
  Application::setApplicationName("LibrePCB");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  Application::setDesktopFileName("org.librepcb.LibrePCB");
#endif
}

/*******************************************************************************
 *  configureApplicationSettings()
 ******************************************************************************/

static void configureApplicationSettings() noexcept {
  // Make sure the INI format is used for settings on all platforms because:
  // - Consistent storage format on all platforms
  // - Useful for functional testing (control settings by fixtures)
  // - Windows Registry is a mess (hard to find, edit and track changes of our
  // settings)
  QSettings::setDefaultFormat(QSettings::IniFormat);

  // Use different configuration directory if supplied by environment variable
  // "LIBREPCB_CONFIG_DIR" (useful for functional testing)
  QString customConfigDir = qgetenv("LIBREPCB_CONFIG_DIR");
  if (!customConfigDir.isEmpty()) {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       customConfigDir);
  }
}

/*******************************************************************************
 *  writeLogHeader()
 ******************************************************************************/

static void writeLogHeader() noexcept {
  // write application name and version to log
  qInfo() << QString("LibrePCB %1 (%2)")
                 .arg(qApp->applicationVersion(), qApp->getGitRevision());

  // write Qt version to log
  qInfo() << QString("Qt version: %1 (compiled against %2)")
                 .arg(qVersion(), QT_VERSION_STR);

  // write resources directory path to log
  qInfo() << QString("Resources directory: %1")
                 .arg(qApp->getResourcesDir().toNative());

  // write application settings directory to log (nice to know for users)
  qInfo() << QString("Application settings: %1")
                 .arg(FilePath(QSettings().fileName()).toNative());
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

/*******************************************************************************
 *  init3rdPartyLibs()
 ******************************************************************************/

static void init3rdPartyLibs() noexcept {
}

/*******************************************************************************
 *  cleanup3rdPartyLibs()
 ******************************************************************************/

static void cleanup3rdPartyLibs() noexcept {
}

/*******************************************************************************
 *  isFileFormatStableOrAcceptUnstable()
 ******************************************************************************/

static bool isFileFormatStableOrAcceptUnstable() noexcept {
  if (qApp->isFileFormatStable() ||
      (qgetenv("LIBREPCB_DISABLE_UNSTABLE_WARNING") == "1")) {
    return true;
  } else {
    QMessageBox::StandardButton btn = QMessageBox::critical(
        nullptr, QCoreApplication::translate("main", "Unstable file format!"),
        QCoreApplication::translate(
            "main",
            "<p><b>ATTENTION: This application version is UNSTABLE!</b></p>"
            "<p>Everything you do with this application could break your "
            "workspace, "
            "libraries or projects! It's highly recommended to create a backup "
            "before proceeding. If you are unsure, please download an official "
            "stable release instead.</p>"
            "<p>Are you really sure to continue with the risk of breaking your "
            "files?!</p>"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    return (btn == QMessageBox::Yes);
  }
}

/*******************************************************************************
 *  determineWorkspacePath()
 ******************************************************************************/

static FilePath determineWorkspacePath() noexcept {
  FilePath wsPath(Workspace::getMostRecentlyUsedWorkspacePath());
  if (!Workspace::isValidWorkspacePath(wsPath)) {
    FirstRunWizard wizard;
    if (wizard.exec() == QDialog::Accepted) {
      wsPath = wizard.getWorkspaceFilePath();
      if (wizard.getCreateNewWorkspace()) {
        try {
          // create new workspace
          Workspace::createNewWorkspace(wsPath);  // can throw

          // open workspace and apply settings
          Workspace ws(wsPath);
          ws.getSettings().getUser().setName(wizard.getNewWorkspaceUserName());
          ws.getSettings().applyAll();  // can throw
        } catch (const Exception& e) {
          QMessageBox::critical(0, Application::translate("Workspace", "Error"),
                                e.getMsg());
          return FilePath();  // TODO: Show the wizard again instead of closing
                              // the application
        }
      }
      Workspace::setMostRecentlyUsedWorkspacePath(wsPath);
    } else {
      return FilePath();  // abort
    }
  }
  return wsPath;
}

/*******************************************************************************
 *  openWorkspace()
 ******************************************************************************/

static int openWorkspace(const FilePath& path) noexcept {
  try {
    Workspace    ws(path);  // The Workspace constructor can throw an exception
    ControlPanel p(ws);
    p.show();

    return appExec();
  } catch (UserCanceled& e) {
    return 0;
  } catch (Exception& e) {
    QMessageBox::critical(
        0, Application::translate("Workspace", "Cannot open the workspace"),
        QString(Application::translate(
                    "Workspace", "The workspace \"%1\" cannot be opened: %2"))
            .arg(path.toNative(), e.getMsg()));
    return 0;
  }
}

/*******************************************************************************
 *  appExec()
 ******************************************************************************/

static int appExec() noexcept {
  // please note that we shouldn't show a dialog or message box in the catch()
  // blocks! from http://qt-project.org/doc/qt-5/exceptionsafety.html:
  //      "After an exception is thrown, the connection to the windowing server
  //      might already be closed. It is not safe to call a GUI related function
  //      after catching an exception."
  try {
    return Application::exec();
  } catch (std::exception& e) {
    qFatal("UNCAUGHT EXCEPTION: %s --- PROGRAM EXITED", e.what());
  } catch (...) {
    qFatal("UNCAUGHT EXCEPTION --- PROGRAM EXITED");
  }

  return -1;
}
