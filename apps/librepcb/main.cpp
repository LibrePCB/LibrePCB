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
#include <librepcb/core/application.h>
#include <librepcb/core/debug.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/network/networkaccessmanager.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/dialogs/directorylockhandlerdialog.h>
#include <librepcb/editor/editorcommandset.h>
#include <librepcb/editor/workspace/controlpanel/controlpanel.h>
#include <librepcb/editor/workspace/initializeworkspacewizard/initializeworkspacewizard.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;
using namespace librepcb::editor;

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

  qDebug().nospace() << "Exit application with code " << retval << ".";
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
  qInfo().noquote() << QString("LibrePCB %1 (%2)")
                           .arg(qApp->applicationVersion(),
                                qApp->getGitRevision());

  // write Qt version to log
  qInfo().noquote() << QString("Qt version: %1 (compiled against %2)")
                           .arg(qVersion(), QT_VERSION_STR);

  // write resources directory path to log
  qInfo() << "Resources directory:" << qApp->getResourcesDir().toNative();

  // write application settings directory to log (nice to know for users)
  qInfo() << "Application settings:"
          << FilePath(QSettings().fileName()).toNative();
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

  // If the workspace path is specified by environment variable, use this one.
  const char* wsEnvVarName = "LIBREPCB_WORKSPACE";
  QString wsEnvStr = qgetenv(wsEnvVarName);
  if (!wsEnvStr.isEmpty()) {
    qInfo() << "Workspace path overridden by" << wsEnvVarName
            << "environment variable:" << wsEnvStr;
    path.setPath(wsEnvStr);
  }

  // If creating or opening a workspace failed, allow to choose another
  // workspace path until it succeeds or the user aborts.
  while (true) {
    try {
      return openWorkspace(path);  // can throw
    } catch (const UserCanceled& e) {
      return 0;  // User canceled -> exit application.
    } catch (const Exception& e) {
      QMessageBox::critical(
          nullptr, Application::translate("Workspace", "Error"),
          QString(Application::translate(
                      "Workspace", "Could not open the workspace \"%1\":"))
                  .arg(path.toNative()) %
              "\n\n" % e.getMsg());
      path = FilePath();  // Make sure the workspace selector wizard is shown.
    }
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
  InitializeWorkspaceWizard wizard(false);
  wizard.setWorkspacePath(path);  // can throw
  while (wizard.getNeedsToBeShown()) {
    if (wizard.exec() != QDialog::Accepted) {
      throw UserCanceled(__FILE__, __LINE__);
    }
    Workspace::setMostRecentlyUsedWorkspacePath(wizard.getWorkspacePath());

    // Just to be on the safe side that the workspace is now *really* ready
    // to open (created, upgraded, initialized, ...), check the status again
    // before continue opening the workspace.
    wizard.setWorkspacePath(wizard.getWorkspacePath());  // can throw
    wizard.restart();
  }

  // Open the workspace (can throw). If it is locked, a dialog will show
  // an error and possibly provides an option to override the lock.
  Workspace ws(wizard.getWorkspacePath(), wizard.getDataDir(),
               DirectoryLockHandlerDialog::createDirectoryLockCallback());

  // Now since workspace settings are loaded, switch to the locale defined
  // there (until now, the system locale was used).
  if (!ws.getSettings().applicationLocale.get().isEmpty()) {
    QLocale locale(ws.getSettings().applicationLocale.get());
    QLocale::setDefault(locale);
    qApp->setTranslationLocale(locale);
    EditorCommandSet::instance().updateTranslations();
  }

  // Apply keyboard shortcuts from workspace settings globally.
  auto applyKeyboardShortcuts = [&ws]() {
    const auto& overrides = ws.getSettings().keyboardShortcuts.get();
    EditorCommandSet& set = EditorCommandSet::instance();
    foreach (EditorCommandCategory* category, set.getCategories()) {
      foreach (EditorCommand* command, set.getCommands(category)) {
        QList<QKeySequence> sequences =
            overrides.contains(command->getIdentifier())
            ? overrides.value(command->getIdentifier())
            : command->getDefaultKeySequences();
        command->setKeySequences(sequences);
      }
    }
  };
  applyKeyboardShortcuts();
  QObject::connect(&ws.getSettings().keyboardShortcuts,
                   &WorkspaceSettingsItem::edited,
                   &ws.getSettings().keyboardShortcuts, applyKeyboardShortcuts);

  // Open the control panel.
  ControlPanel p(ws, wizard.getWorkspaceContainsNewerFileFormats());
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
