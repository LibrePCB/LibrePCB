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

#include "3d/occmodel.h"
#include "application.h"
#include "fileio/filepath.h"
#include "fileio/transactionalfilesystem.h"
#include "font/strokefontpool.h"
#include "librepcb_build_env.h"
#include "systeminfo.h"
#include "types/version.h"

#include <QtCore>
#include <QtNetwork>

#include <tuple>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString Application::getVersion() noexcept {
  return QStringLiteral(LIBREPCB_APP_VERSION);
}

QString Application::getGitRevision() noexcept {
  return QStringLiteral(GIT_COMMIT_SHA);
}

QDateTime Application::getBuildDate() noexcept {
  static const QDateTime value = QDateTime(
      QLocale(QLocale::C)
          .toDate(QString(__DATE__).simplified(), QLatin1String("MMM d yyyy")),
      QTime::fromString(__TIME__, Qt::TextDate));
  return value;
}

QString Application::getBuildAuthor() noexcept {
  return QStringLiteral(LIBREPCB_BUILD_AUTHOR);
}

const Version& Application::getFileFormatVersion() noexcept {
  static const Version value =
      Version::fromString(LIBREPCB_FILE_FORMAT_VERSION);
  Q_ASSERT(getVersion().startsWith(value.toStr() % "."));
  return value;
}

bool Application::isFileFormatStable() noexcept {
  return LIBREPCB_FILE_FORMAT_STABLE;
}

QString Application::buildFullVersionDetails() noexcept {
  // Always English, not translatable!
  QStringList details;
  const QString date = getBuildDate().toString(Qt::ISODate);
  QString qt = QString(qVersion()) + " (built against " + QT_VERSION_STR + ")";
  details << "LibrePCB Version: " + getVersion();
  details << "Git Revision:     " + getGitRevision();
  details << "Build Date:       " + date;
  if (!getBuildAuthor().isEmpty()) {
    details << "Build Author:     " + getBuildAuthor();
  }
  details << "Qt Version:       " + qt;
  details << "CPU Architecture: " + QSysInfo::currentCpuArchitecture();
  details << "Operating System: " + QSysInfo::prettyProductName();
  details << "Platform Plugin:  " + qApp->platformName();
  details << "TLS Library:      " + QSslSocket::sslLibraryVersionString();
  details << "OCC Library:      " + OccModel::getOccVersionString();
  if (!SystemInfo::detectRuntime().isEmpty()) {
    details << "Runtime:          " + SystemInfo::detectRuntime();
  }
  return details.join("\n");
}

FilePath Application::getCacheDir() noexcept {
  auto detect = []() {
    // Use different configuration directory if supplied by environment
    // variable "LIBREPCB_CACHE_DIR" (useful for functional testing).
    FilePath fp(qgetenv("LIBREPCB_CACHE_DIR"));

    // If no valid path was specified, use the default cache directory.
    if (!fp.isValid()) {
      fp.setPath(
          QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    }
    return fp;
  };

  static const FilePath value = detect();
  return value;
}

const FilePath& Application::getResourcesDir() noexcept {
  auto detect = []() {
    // get the directory of the currently running executable
    const FilePath exeFilePath(qApp->applicationFilePath());
    Q_ASSERT(exeFilePath.isValid());

    // determine the path to the resources directory (e.g. /usr/share/librepcb)
    FilePath fp;
#if defined(LIBREPCB_BINARY_DIR) && defined(LIBREPCB_SHARE_SOURCE)
    // TODO: The following code checks for paths related to the application
    // binary, even though this code is located in the library source. This is a
    // bit of a layer violation and should be refactored.
    FilePath buildOutputDirPath(LIBREPCB_BINARY_DIR);
    bool runningFromBuildOutput =
        exeFilePath.isLocatedInDir(buildOutputDirPath);
    if (runningFromBuildOutput) {
      // The executable is located inside the build output directory, so we
      // assume this is a developer build and thus we use the "share" directory
      // from the repository root.
      fp = FilePath(LIBREPCB_SHARE_SOURCE).getPathTo("librepcb");
    }
#endif
    if (!fp.isValid()) {
      if (QDir::isAbsolutePath(LIBREPCB_SHARE)) {
        fp.setPath(LIBREPCB_SHARE);
      } else {
        fp = exeFilePath.getParentDir().getPathTo(LIBREPCB_SHARE);
      }
    }

    // warn if runtime resource files are not found
    if (!fp.getPathTo("README.md").isExistingFile()) {
      qCritical()
          << "Could not find resource files! Probably packaging went wrong?!";
      qCritical() << "Expected resources location:" << fp.toNative();
      qCritical() << "Executable location:        " << exeFilePath.toNative();
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
    return fp;
  };

  static const FilePath value = detect();
  return value;
}

QStringList Application::getTranslationLocales() noexcept {
  auto detect = []() {
    QStringList locales;
    QDir dir(getResourcesDir().getPathTo("i18n").toStr());
    foreach (QString filename,
             dir.entryList({"*.qm"}, QDir::Files, QDir::Name)) {
      filename.remove("librepcb_");
      filename.remove(".qm");
      locales.append(filename);
    }
    return locales;
  };

  static const QStringList value = detect();
  return value;
}

const QFont& Application::getDefaultSansSerifFont() noexcept {
  auto create = []() {
    QFont font;
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferQuality));
    font.setStyleHint(QFont::SansSerif);
    font.setFamily("Noto Sans");
    return font;
  };

  static const QFont value = create();
  return value;
}

const QFont& Application::getDefaultMonospaceFont() noexcept {
  auto create = []() {
    QFont font;
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferQuality));
    font.setStyleHint(QFont::TypeWriter);
    font.setFamily("Noto Sans Mono");
    return font;
  };

  static const QFont value = create();
  return value;
}

const StrokeFontPool& Application::getStrokeFonts() noexcept {
  static const TransactionalFileSystem fs(
      getResourcesDir().getPathTo("fontobene"), false,
      &TransactionalFileSystem::RestoreMode::no);
  static const StrokeFontPool pool(fs);

  // Abort the application if there's no default stroke font!
  auto checkDefaultFontExistence = [](const StrokeFontPool& p) {
    if (!p.exists(getDefaultStrokeFontName())) {
      qFatal("Failed to load default stroke font, terminating application!");
    }
    return true;
  };
  static bool check = checkDefaultFontExistence(pool);
  Q_UNUSED(check);

  return pool;
}

const StrokeFont& Application::getDefaultStrokeFont() noexcept {
  return getStrokeFonts().getFont(getDefaultStrokeFontName());
}

QString Application::getDefaultStrokeFontName() noexcept {
  return QStringLiteral("newstroke.bene");
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Application::loadBundledFonts() noexcept {
  QDir fontsDir(Application::getResourcesDir().getPathTo("fonts").toStr());
  fontsDir.setFilter(QDir::Files);
  fontsDir.setNameFilters({"*.ttf", "*.otf"});
  foreach (const QFileInfo& info, fontsDir.entryInfoList()) {
    QString fp = info.absoluteFilePath();
    int id = QFontDatabase::addApplicationFont(fp);
    if (id < 0) {
      qCritical().nospace() << "Failed to register font " << fp << ".";
    }
  }
}

void Application::setTranslationLocale(const QLocale& locale) noexcept {
  static QVector<QTranslator*> installedTranslators;

  // First, remove all currently installed translations to avoid falling back to
  // wrong languages. The fallback language must always be en_US, i.e.
  // untranslated strings. See https://github.com/LibrePCB/LibrePCB/issues/611
  foreach (QTranslator* translator, installedTranslators) {
    if (!qApp->removeTranslator(translator)) {
      qWarning() << "Failed to remove translator.";
    }
  }
  qDeleteAll(installedTranslators);
  installedTranslators.clear();

  // Install Qt translations
  QTranslator* qtTranslator = new QTranslator(qApp);
  std::ignore =
      qtTranslator->load("qt_" % locale.name(),
                         QLibraryInfo::path(QLibraryInfo::TranslationsPath));
  qApp->installTranslator(qtTranslator);
  installedTranslators.append(qtTranslator);

  // Install system language translations (all system languages defined in the
  // system settings, in the defined order)
  const QString dir = Application::getResourcesDir().getPathTo("i18n").toStr();
  QTranslator* systemTranslator = new QTranslator(qApp);
  std::ignore = systemTranslator->load(locale, "librepcb", "_", dir);
  qApp->installTranslator(systemTranslator);
  installedTranslators.append(systemTranslator);

  // Install language translations (like "de" for German)
  QTranslator* appTranslator1 = new QTranslator(qApp);
  std::ignore =
      appTranslator1->load("librepcb_" % locale.name().split("_").at(0), dir);
  qApp->installTranslator(appTranslator1);
  installedTranslators.append(appTranslator1);

  // Install language/country translations (like "de_ch" for German/Switzerland)
  QTranslator* appTranslator2 = new QTranslator(qApp);
  std::ignore = appTranslator2->load("librepcb_" % locale.name(), dir);
  qApp->installTranslator(appTranslator2);
  installedTranslators.append(appTranslator2);
}

void Application::cleanTemporaryDirectory() noexcept {
  const qint64 maxAgeMs = 60LL * 24LL * 3600LL * 1000LL;  // 60 days

  QDir dir(FilePath::getApplicationTempPath().toStr());
  dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  foreach (const QFileInfo& info, dir.entryInfoList()) {
    bool ok = false;
    qlonglong ts = info.fileName().split("_").value(0).toLongLong(&ok);
    if ((!ok) || (info.fileName().count("_") != 1)) {
      const QDateTime dt = info.lastModified();
      if (dt.isValid()) {
        ts = dt.toMSecsSinceEpoch();
      } else {
        qWarning() << "Could not determine file age:"
                   << info.absoluteFilePath();
        ts = 0;
      }
    }
    if ((QDateTime::currentMSecsSinceEpoch() - ts) > maxAgeMs) {
      if (info.isDir()) {
        qInfo() << "Removing old temporary directory:"
                << info.absoluteFilePath();
        QDir(info.absoluteFilePath()).removeRecursively();
      } else {
        qInfo() << "Removing old temporary file:" << info.absoluteFilePath();
        QFile::remove(info.absoluteFilePath());
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
