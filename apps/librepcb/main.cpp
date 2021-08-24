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
#include "initializeworkspacewizard/initializeworkspacewizard.h"

#include <librepcb/common/application.h>
#include <librepcb/common/debug.h>
#include <librepcb/common/dialogs/directorylockhandlerdialog.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/network/networkaccessmanager.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

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

static void setApplicationMetadata() noexcept;
static void configureApplicationSettings() noexcept;
static void writeLogHeader() noexcept;
static int runApplication() noexcept;
static bool isFileFormatStableOrAcceptUnstable() noexcept;
static int openWorkspace(FilePath& path);
static int appExec() noexcept;

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  Application app(argc, argv);

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
  app.setTranslationLocale(QLocale::system());

  // This is to remove the ugly frames around widgets in all status bars...
  // (from http://www.qtcentre.org/threads/1904)
  app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

  // Start network access manager thread
  QScopedPointer<NetworkAccessManager> networkAccessManager(
      new NetworkAccessManager());

  // Run the actual application
  int retval = runApplication();

  // Stop network access manager thread
  networkAccessManager.reset();

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
 *  openWorkspace()
 ******************************************************************************/

static int runApplication() noexcept {
  // If the file format is unstable (e.g. for nightly builds), ask to abort now.
  // This warning *must* come that early to be really sure that no files are
  // overwritten with unstable content!
  if (!isFileFormatStableOrAcceptUnstable()) {
    return 0;
  }

  // Get the path of the workspace to open. By default, open the recently used
  // workspace stored in the user settings.
  FilePath path = Workspace::getMostRecentlyUsedWorkspacePath();
  qDebug() << "Recently used workspace:" << path.toNative();

  // If creating or opening a workspace failed, allow to choose another
  // workspace path until it succeeds or the user aborts.
  try {
    return openWorkspace(path);  // can throw
  } catch (const UserCanceled& e) {
    return 0;  // User canceled -> exit application.
  } catch (const Exception& e) {
    QMessageBox::critical(
        nullptr, Application::translate("Workspace", "Error"),
        QString(Application::translate("Workspace",
                                       "Could not open the workspace \"%1\":"))
                .arg(path.toNative()) %
            "\n\n" % e.getMsg());
    return 0;  // Failure -> exit application.
  }
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
            "<p>Everything you do with this application can break your "
            "workspace, libraries or projects! Saved files will not be "
            "readable with stable releases of LibrePCB. It's highly "
            "recommended to create a backup before proceeding. If you are "
            "unsure, please download an official stable release instead.</p>"
            "<p>For details, please take a look at LibrePCB's "
            "<a href=\"%1\">versioning concept</a>.</p>"
            "<p>Are you really sure to continue with the risk of breaking your "
            "files?!</p>")
            .arg("https://developers.librepcb.org/da/dbc/"
                 "doc_release_workflow.html"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    return (btn == QMessageBox::Yes);
  }
}

/*******************************************************************************
 *  openWorkspace()
 ******************************************************************************/

static int openWorkspace(FilePath& path) {
  // If no valid workspace path is available, ask the user to choose it.
  if (!Workspace::isValidWorkspacePath(path)) {
    FirstRunWizard wizard;
    if (wizard.exec() == QDialog::Accepted) {
      path = wizard.getWorkspaceFilePath();
      if (wizard.getCreateNewWorkspace()) {
        Workspace::createNewWorkspace(path);  // can throw
      }
      Workspace::setMostRecentlyUsedWorkspacePath(path);
    } else {
      throw UserCanceled(__FILE__, __LINE__);
    }
  }

  // Migrate workspace to new major version, if needed. Note that this needs
  // to be done *before* opening the workspace, otherwise the workspace would
  // be default-initialized!
  QList<Version> versions = Workspace::getFileFormatVersionsOfWorkspace(path);
  if (!versions.contains(qApp->getFileFormatVersion())) {
    InitializeWorkspaceWizard wizard(path);
    if (wizard.exec() != QDialog::Accepted) {
      throw UserCanceled(__FILE__, __LINE__);
    }
  }

  // Open the workspace (can throw). If it is locked, a dialog will show
  // an error and possibly provides an option to override the lock.
  Workspace ws(path, DirectoryLockHandlerDialog::createDirectoryLockCallback());

  // Now since workspace settings are loaded, switch to the locale defined
  // there (until now, the system locale was used).
  if (!ws.getSettings().applicationLocale.get().isEmpty()) {
    QLocale locale(ws.getSettings().applicationLocale.get());
    QLocale::setDefault(locale);
    qApp->setTranslationLocale(locale);
  }

  ControlPanel p(ws);
  p.show();

  return appExec();
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
