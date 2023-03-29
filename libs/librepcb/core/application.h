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
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class StrokeFont;
class StrokeFontPool;
class Version;

/*******************************************************************************
 *  Class Application
 ******************************************************************************/

/**
 * @brief Static functions to access some global application configuration
 */
class Application final {
public:
  // Constructors / Destructor
  Application() = delete;
  Application(const Application& other) = delete;
  ~Application() = delete;

  // Getters

  /**
   * @brief Get the application version
   *
   * @note This function is thread-safe.
   *
   * @return Version string (e.g. "0.2.0-unstable")
   */
  static QString getVersion() noexcept;

  /**
   * @brief Get the git revision of the sources used to build the application
   *
   * @note This function is thread-safe.
   *
   * @return Revision hash (might be empty)
   */
  static QString getGitRevision() noexcept;

  /**
   * @brief Get the date/time when the application was built
   *
   * @note This function is thread-safe.
   *
   * @return Date/time (might be invalid)
   */
  static QDateTime getBuildDate() noexcept;

  /**
   * @brief Get the author who has built the application
   *
   * @note This function is thread-safe.
   *
   * @return Author (e.g. "LibrePCB CI"; might be empty)
   */
  static QString getBuildAuthor() noexcept;

  /**
   * @brief Get the used file format version
   *
   * @note This function is thread-safe.
   *
   * @return File format version number
   */
  static const Version& getFileFormatVersion() noexcept;

  /**
   * @brief Check whether the used file format is stable
   *
   * @note This function is thread-safe.
   *
   * @retval true Stable
   * @retval false Unstable
   */
  static bool isFileFormatStable() noexcept;

  /**
   * @brief Get the path to the resources directory
   *
   * @note This function is thread-safe.
   *
   * @return Guaranteed valid file path (e.g. "/usr/share/librepcb/")
   */
  static const FilePath& getResourcesDir() noexcept;

  /**
   * @brief Get all available translation locales
   *
   * @return Locales like "de_CH"
   */
  static QStringList getTranslationLocales() noexcept;

  /**
   * @brief Get the default sans serif font
   *
   * Font to be used e.g. in schematics.
   *
   * @warning This function is not thread-safe!
   *
   * @return Reference to font object
   */
  static const QFont& getDefaultSansSerifFont() noexcept;

  /**
   * @brief Get the default monospace font
   *
   * Font to be used e.g. in schematics.
   *
   * @warning This function is not thread-safe!
   *
   * @return Reference to font object
   */
  static const QFont& getDefaultMonospaceFont() noexcept;

  /**
   * @brief Get all globally available stroke fonts
   *
   * @note This function is thread-safe.
   *
   * @return Reference to stroke font pool
   */
  static const StrokeFontPool& getStrokeFonts() noexcept;

  /**
   * @brief Get the default stroke font
   *
   * @note This function is thread-safe.
   *
   * @return Reference to stroke font
   */
  static const StrokeFont& getDefaultStrokeFont() noexcept;

  /**
   * @brief Get the name of the default stroke font
   *
   * @note This function is thread-safe.
   *
   * @return Name of the default stroke font
   */
  static QString getDefaultStrokeFontName() noexcept;

  // General Methods

  /**
   * @brief Load all bundled fonts to make them available in the application
   *
   * To be called once at application startup.
   */
  static void loadBundledFonts() noexcept;

  /**
   * @brief Install all translators for a given locale
   *
   * @param locale    Locale to use for translated strings.
   */
  static void setTranslationLocale(const QLocale& locale) noexcept;

  // Operator Overloadings
  Application& operator=(const Application& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
