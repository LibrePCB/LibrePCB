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

#ifndef LIBREPCB_CORE_APPLICATION_H
#define LIBREPCB_CORE_APPLICATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileio/filepath.h"
#include "types/version.h"

#include <QApplication>
#include <QFont>
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class StrokeFont;
class StrokeFontPool;

/*******************************************************************************
 *  Macros
 ******************************************************************************/
#if defined(qApp)
#undef qApp
#endif
#define qApp (Application::instance())

/*******************************************************************************
 *  Class Application
 ******************************************************************************/

/**
 * @brief The Application class extends the QApplication with the exception-safe
 * method #notify()
 */
class Application final : public QApplication {
  Q_OBJECT

public:
  // Constructors / Destructor
  Application() = delete;
  Application(const Application& other) = delete;
  Application(int& argc, char** argv) noexcept;
  ~Application() noexcept;

  // Getters
  const Version& getAppVersion() const noexcept { return mAppVersion; }
  const QString& getAppVersionLabel() const noexcept {
    return mAppVersionLabel;
  }
  const QString& getGitRevision() const noexcept { return mGitRevision; }
  const QDateTime& getBuildDate() const noexcept { return mBuildDate; }
  const Version& getFileFormatVersion() const noexcept {
    return mFileFormatVersion;
  }
  bool isFileFormatStable() const noexcept { return mIsFileFormatStable; }
  const FilePath& getResourcesDir() const noexcept { return mResourcesDir; }
  FilePath getResourcesFilePath(const QString& filepath) const noexcept;
  QStringList getAvailableTranslationLocales() const noexcept;
  const QFont& getDefaultSansSerifFont() const noexcept {
    return mSansSerifFont;
  }
  const QFont& getDefaultMonospaceFont() const noexcept {
    return mMonospaceFont;
  }
  const StrokeFontPool& getStrokeFonts() const noexcept {
    return *mStrokeFontPool;
  }
  QString getDefaultStrokeFontName() const noexcept { return "newstroke.bene"; }
  const StrokeFont& getDefaultStrokeFont() const noexcept;

  // Setters
  void setTranslationLocale(const QLocale& locale) noexcept;

  // Reimplemented from QApplication
  bool notify(QObject* receiver, QEvent* e);

  // Operator Overloadings
  Application& operator=(const Application& rhs) = delete;

  // Static Methods
  static Application* instance() noexcept;

private:  // Methods
  void removeAllTranslators() noexcept;

private:  // Data
  Version mAppVersion;
  QString mAppVersionLabel;
  QString mGitRevision;
  QDateTime mBuildDate;
  Version mFileFormatVersion;
  bool mIsFileFormatStable;
  FilePath mResourcesDir;

  /// Pool containing all application stroke fonts
  QScopedPointer<StrokeFontPool> mStrokeFontPool;

  /// Default sans serif font
  QFont mSansSerifFont;

  /// Default monospace font
  QFont mMonospaceFont;

  /// All currently installed translators
  QList<std::shared_ptr<QTranslator>> mTranslators;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
