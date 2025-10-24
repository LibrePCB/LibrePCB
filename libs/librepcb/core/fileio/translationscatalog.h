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

#ifndef LIBREPCB_CORE_TRANSLATIONSCATALOG_H
#define LIBREPCB_CORE_TRANSLATIONSCATALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

/*******************************************************************************
 *  Class TranslationsCatalog
 ******************************************************************************/

/**
 * @brief Catalog of translatable strings, to read/write gettext *.po files
 */
class TranslationsCatalog final {
  Q_DECLARE_TR_FUNCTIONS(TranslationsCatalog)

public:
  // Types
  struct Message {
    QString id;
    QString string;
    QString comment;
    QString location;
  };

  // Constructors / Destructor
  TranslationsCatalog() noexcept;
  TranslationsCatalog(const TranslationsCatalog& other) = delete;
  ~TranslationsCatalog() noexcept;

  /**
   * @brief Get the number of translations
   *
   * @return Count
   */
  int getCount() const noexcept { return mStrings.count(); }

  /**
   * @brief Add a message to the catalog
   */
  void add(const Message& msg) noexcept { mStrings.insert(msg.location, msg); }

  /**
   * @brief Save tanslations to *.po file
   *
   * @param fp  The destination file path.
   *
   * @throw ::librepcb::Exception if the file could not be written.
   */
  void saveTo(const FilePath& fp) const;

  // Operator Overloadings
  TranslationsCatalog& operator=(const TranslationsCatalog& rhs) = delete;

private:  // Data
  QMap<QString, Message> mStrings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
