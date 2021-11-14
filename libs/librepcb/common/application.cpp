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
#include "application.h"

#include "build_env.h"
#include "fileio/transactionalfilesystem.h"
#include "font/strokefontpool.h"
#include "units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Application::Application(int& argc, char** argv) noexcept
  : QApplication(argc, argv),
    mAppVersion(
        Version::fromString(QString(LIBREPCB_APP_VERSION).section('-', 0, 0))),
    mAppVersionLabel(QString(LIBREPCB_APP_VERSION).section('-', 1, 1)),
    mGitRevision(GIT_COMMIT_SHA),
    mFileFormatVersion(Version::fromString(LIBREPCB_FILE_FORMAT_VERSION)),
    mIsFileFormatStable(LIBREPCB_FILE_FORMAT_STABLE) {
  // register meta types
  qRegisterMetaType<FilePath>();
  qRegisterMetaType<Point>();
  qRegisterMetaType<Length>();
  qRegisterMetaType<Angle>();

  // set application version
  QApplication::setApplicationVersion(LIBREPCB_APP_VERSION);

  // set build timestamp
  QDate buildDate =
      QLocale(QLocale::C)
          .toDate(QString(__DATE__).simplified(), QLatin1String("MMM d yyyy"));
  QTime buildTime = QTime::fromString(__TIME__, Qt::TextDate);
  mBuildDate = QDateTime(buildDate, buildTime);

  // check file format version
  if (!mFileFormatVersion.isPrefixOf(mAppVersion)) {
    qFatal(
        "The file format version is not a prefix of the application version!");
  }

  // get the directory of the currently running executable
  FilePath executableFilePath(QApplication::applicationFilePath());
  Q_ASSERT(executableFilePath.isValid());

  // determine the path to the resources directory (e.g. /usr/share/librepcb)
#if defined(LIBREPCB_BINARY_DIR) && defined(LIBREPCB_SHARE_SOURCE)
  // TODO: The following code checks for paths related to the application
  // binary, even though this code is located in the library source. This is a
  // bit of a layer violation and should be refactored.
  FilePath buildOutputDirPath(LIBREPCB_BINARY_DIR);
  bool runningFromBuildOutput =
      executableFilePath.isLocatedInDir(buildOutputDirPath);
  if (runningFromBuildOutput) {
    // The executable is located inside the build output directory, so we assume
    // this is a developer build and thus we use the "share" directory from the
    // repository root.
    mResourcesDir = FilePath(LIBREPCB_SHARE_SOURCE).getPathTo("librepcb");
  }
#endif
  if (!mResourcesDir.isValid()) {
    if (QDir::isAbsolutePath(LIBREPCB_SHARE)) {
      mResourcesDir.setPath(LIBREPCB_SHARE);
    } else {
      mResourcesDir =
          executableFilePath.getParentDir().getPathTo(LIBREPCB_SHARE);
    }
  }

  // warn if runtime resource files are not found
  if (!getResourcesFilePath("README.md").isExistingFile()) {
    qCritical()
        << "Could not find resource files! Probably packaging went wrong?!";
    qCritical() << "Expected resources location:" << mResourcesDir.toNative();
    qCritical() << "Executable location:        "
                << executableFilePath.toNative();
    qCritical() << "LIBREPCB_SHARE:             " << QString(LIBREPCB_SHARE);
#ifdef LIBREPCB_BINARY_DIR
    qCritical() << "LIBREPCB_BINARY_DIR:        "
                << QString(LIBREPCB_BINARY_DIR);
#endif
#ifdef LIBREPCB_SHARE_SOURCE
    qCritical() << "LIBREPCB_SHARE_SOURCE:      "
                << QString(LIBREPCB_SHARE_SOURCE);
#endif
  }

  // load all bundled TrueType/OpenType fonts
  QDir fontsDir(getResourcesFilePath("fonts").toStr());
  fontsDir.setFilter(QDir::Files);
  fontsDir.setNameFilters({"*.ttf", "*.otf"});
  foreach (const QFileInfo& info, fontsDir.entryInfoList()) {
    QString fp = info.absoluteFilePath();
    int id = QFontDatabase::addApplicationFont(fp);
    if (id < 0) {
      qCritical() << "Failed to load font" << fp;
    }
  }

  // set default sans serif font
  mSansSerifFont.setStyleStrategy(
      QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
  mSansSerifFont.setStyleHint(QFont::SansSerif);
  mSansSerifFont.setFamily("Noto Sans");

  // set default monospace font
  mMonospaceFont.setStyleStrategy(
      QFont::StyleStrategy(QFont::OpenGLCompatible | QFont::PreferQuality));
  mMonospaceFont.setStyleHint(QFont::TypeWriter);
  mMonospaceFont.setFamily("Noto Sans Mono");

  // load all stroke fonts
  TransactionalFileSystem strokeFontsDir(
      getResourcesFilePath("fontobene"), false,
      &TransactionalFileSystem::RestoreMode::no);
  mStrokeFontPool.reset(new StrokeFontPool(strokeFontsDir));
  getDefaultStrokeFont();  // ensure that the default font is available (aborts
                           // if not)
}

Application::~Application() noexcept {
  // Not sure if needed, but let's unregister translators before destroying
  // (maybe otherwise QCoreApplication has dangling pointers to translators).
  removeAllTranslators();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FilePath Application::getResourcesFilePath(const QString& filepath) const
    noexcept {
  return mResourcesDir.getPathTo(filepath);
}

QStringList Application::getAvailableTranslationLocales() const noexcept {
  QStringList locales;
  QDir dir(getResourcesFilePath("i18n").toStr());
  foreach (QString filename, dir.entryList({"*.qm"}, QDir::Files, QDir::Name)) {
    filename.remove("librepcb_");
    filename.remove(".qm");
    locales.append(filename);
  }
  return locales;
}

const StrokeFont& Application::getDefaultStrokeFont() const noexcept {
  try {
    return mStrokeFontPool->getFont(getDefaultStrokeFontName());
  } catch (const Exception& e) {
    qFatal("Default stroke font could not be loaded!");  // aborts the
                                                         // application!!!
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Application::setTranslationLocale(const QLocale& locale) noexcept {
  // First, remove all currently installed translations to avoid falling back to
  // wrong languages. The fallback language must always be en_US, i.e.
  // untranslated strings. See https://github.com/LibrePCB/LibrePCB/issues/611
  removeAllTranslators();

  // Install Qt translations
  auto qtTranslator = std::make_shared<QTranslator>(this);
  qtTranslator->load("qt_" % locale.name(),
                     QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  installTranslator(qtTranslator.get());
  mTranslators.append(qtTranslator);

  // Install system language translations (all system languages defined in the
  // system settings, in the defined order)
  const QString dir = getResourcesFilePath("i18n").toStr();
  auto systemTranslator = std::make_shared<QTranslator>(this);
  systemTranslator->load(locale, "librepcb", "_", dir);
  installTranslator(systemTranslator.get());
  mTranslators.append(systemTranslator);

  // Install language translations (like "de" for German)
  auto appTranslator1 = std::make_shared<QTranslator>(this);
  appTranslator1->load("librepcb_" % locale.name().split("_").at(0), dir);
  installTranslator(appTranslator1.get());
  mTranslators.append(appTranslator1);

  // Install language/country translations (like "de_ch" for German/Switzerland)
  auto appTranslator2 = std::make_shared<QTranslator>(this);
  appTranslator2->load("librepcb_" % locale.name(), dir);
  installTranslator(appTranslator2.get());
  mTranslators.append(appTranslator2);
}

/*******************************************************************************
 *  Reimplemented from QApplication
 ******************************************************************************/

bool Application::notify(QObject* receiver, QEvent* e) {
  try {
    return QApplication::notify(receiver, e);
  } catch (...) {
    qCritical() << "Exception caught in Application::notify()!";
  }
  return false;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Application* Application::instance() noexcept {
  Application* app = dynamic_cast<Application*>(QCoreApplication::instance());
  Q_ASSERT(app);
  return app;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Application::removeAllTranslators() noexcept {
  foreach (auto& translator, mTranslators) {
    if (!qApp->removeTranslator(translator.get())) {
      qWarning() << "Failed to remove translator.";
    }
  }
  mTranslators.clear();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
