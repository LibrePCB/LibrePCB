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
#include <librepcb/editor/guiapplication.h>
#include <librepcb/editor/project/partinformationprovider.h>
#include <librepcb/editor/workspace/initializeworkspacewizard/initializeworkspacewizard.h>

#include <QtConcurrent>
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

/*******************************************************************************
 *  main()
 ******************************************************************************/

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  // Give the main thread a higher priority than most other threads as GUI
  // rendering and event processing are important for a smooth user experience.
  QThread::currentThread()->setPriority(QThread::HighPriority);

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

  // Perform global initialization tasks. This must be done before any widget is
  // shown.
  Application::loadBundledFonts();
  Application::setTranslationLocale(QLocale::system());

  // Clean up old temporary files since at least on Windows this is not done
  // automatically. Let's do it in a thread to avoid delaying application start.
  std::ignore = QtConcurrent::run(&Application::cleanTemporaryDirectory);

  // This is to remove the ugly frames around widgets in all status bars...
  // (from http://www.qtcentre.org/threads/1904)
  app.setStyleSheet("QStatusBar::item { border: 0px solid black; }");

  // Use Fusion style with custom palette to make the legacy Qt dialogs looking
  // similar to the new Slint UI. Can be removed as soon as no Qt widgets are
  // used anymore.
  {
    QPalette palette;
    palette.setColor(QPalette::Window, "#2a2a2a");
    palette.setColor(QPalette::WindowText, "#c4c4c4");
    palette.setColor(QPalette::Base, "#262626");
    palette.setColor(QPalette::AlternateBase, "#2e2e2e");
    palette.setColor(QPalette::ToolTipBase, "#2e2e2e");
    palette.setColor(QPalette::ToolTipText, "#dedede");
    palette.setColor(QPalette::Text, "#c4c4c4");
    palette.setColor(QPalette::PlaceholderText, "#959595");
    palette.setColor(QPalette::Button, "#202020");
    palette.setColor(QPalette::ButtonText, "#c4c4c4");
    palette.setColor(QPalette::Link, "#29d682");
    palette.setColor(QPalette::LinkVisited, "#29d682");
    palette.setColor(QPalette::Highlight, "#29d682");
    palette.setColor(QPalette::HighlightedText, "#161616");
    palette.setColor(QPalette::Disabled, QPalette::Button, "#1a1a1a");
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, "#707070");
    palette.setColor(QPalette::Disabled, QPalette::WindowText, "#707070");
    palette.setColor(QPalette::Disabled, QPalette::Text, "#707070");
    palette.setColor(QPalette::Disabled, QPalette::Light, "#707070");
    app.setStyle("fusion");
    app.setPalette(palette);
  }

  // Start network access manager thread with HTTP cache to avoid extensive
  // requests (e.g. downloading library pictures each time opening the manager).
  QScopedPointer<NetworkAccessManager> networkAccessManager(
      new NetworkAccessManager(Application::getCacheDir().getPathTo("http")));

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
  QApplication::setOrganizationName("LibrePCB");
  QApplication::setOrganizationDomain("librepcb.org");
  QApplication::setApplicationName("LibrePCB");
  QApplication::setApplicationVersion(Application::getVersion());
  QApplication::setDesktopFileName("org.librepcb.LibrePCB");
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
                           .arg(Application::getVersion(),
                                Application::getGitRevision());

  // write Qt version to log
  qInfo().noquote() << QString("Qt version: %1 (compiled against %2)")
                           .arg(qVersion(), QT_VERSION_STR);

  // write resources directory path to log
  qInfo() << "Resources directory:"
          << Application::getResourcesDir().toNative();

  // write application settings file to log (nice to know for users)
  qInfo() << "Application settings:"
          << FilePath(QSettings().fileName()).toNative();

  // write cache directory to log (nice to know for users)
  qInfo() << "Cache directory:" << Application::getCacheDir().toNative();
}

/*******************************************************************************
 *  openWorkspace()
 ******************************************************************************/

static int runApplication() noexcept {
  // For deployment testing purposes, exit the application now if the flag
  // '--exit-after-startup' is passed. This shall be done *before* any user
  // interaction (e.g. message box) to make it working headless.
  const char* exitFlagName = "--exit-after-startup";
  if (qApp->arguments().contains(exitFlagName)) {
    qInfo().nospace() << "Exit requested by flag '" << exitFlagName << "'.";
    return 0;
  }

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
          nullptr, QApplication::translate("Workspace", "Error"),
          QString(QApplication::translate(
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
  if (Application::isFileFormatStable() ||
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
    Application::setTranslationLocale(locale);
    EditorCommandSet::instance().updateTranslations();
  }

  // Setup global parts information provider (with cache).
  PartInformationProvider::instance().setCacheDir(Application::getCacheDir());
  auto applyPartInformationProviderSettings = [&ws]() {
    PartInformationProvider::instance().setApiEndpoint(
        ws.getSettings().apiEndpoints.get().value(0));
  };
  applyPartInformationProviderSettings();
  QObject::connect(
      &ws.getSettings().apiEndpoints, &WorkspaceSettingsItem::edited,
      &ws.getSettings().apiEndpoints, applyPartInformationProviderSettings);

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

  // Run the application.
  GuiApplication app(ws, wizard.getWorkspaceContainsNewerFileFormats());
  app.exec();
  return 0;
}
