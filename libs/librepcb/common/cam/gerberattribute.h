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

#ifndef LIBREPCB_GERBERATTRIBUTE_H
#define LIBREPCB_GERBERATTRIBUTE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Uuid;

/*******************************************************************************
 *  Class GerberAttribute
 ******************************************************************************/

/**
 * @brief A Gerber X2 attribute
 */
class GerberAttribute final {
  Q_DECLARE_TR_FUNCTIONS(GerberAttribute)

public:
  // Types
  enum class Type { Invalid, File, Aperture, Object, Delete };

  // Constructors / Destructor
  GerberAttribute() noexcept;
  GerberAttribute(const GerberAttribute& other) noexcept;
  ~GerberAttribute() noexcept;

  // Getters
  Type getType() const noexcept { return mType; }
  const QString& getKey() const noexcept { return mKey; }
  const QStringList& getValues() const noexcept { return mValues; }

  // General Methods
  QString toGerberString() const noexcept;
  QString toExcellonString() const noexcept;

  // Operator Overloadings
  GerberAttribute& operator=(const GerberAttribute& rhs) noexcept;
  bool operator==(const GerberAttribute& rhs) const noexcept;

  // Static Methods
  static GerberAttribute unset(const QString& key) noexcept;
  static GerberAttribute fileGenerationSoftware(
      const QString& vendor, const QString& application,
      const QString& version) noexcept;
  static GerberAttribute fileCreationDate(const QDateTime& date) noexcept;
  static GerberAttribute fileProjectId(const QString& name, const Uuid& uuid,
                                       const QString& revision) noexcept;
  static GerberAttribute filePartSingle() noexcept;
  static GerberAttribute fileMd5(const QString& md5) noexcept;

private:  // Methods
  GerberAttribute(Type type, const QString& key,
                  const QStringList& values) noexcept;
  QString toString() const noexcept;
  static QString escapeValue(const QString& value) noexcept;

private:  // Data
  Type mType;
  QString mKey;
  QStringList mValues;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
