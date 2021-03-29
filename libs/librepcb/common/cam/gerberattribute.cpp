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
#include "gerberattribute.h"

#include "../uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GerberAttribute::GerberAttribute() noexcept
  : mType(Type::Invalid), mKey(), mValues() {
}

GerberAttribute::GerberAttribute(Type type, const QString& key,
                                 const QStringList& values) noexcept
  : mType(type), mKey(key), mValues(values) {
}

GerberAttribute::GerberAttribute(const GerberAttribute& other) noexcept
  : mType(other.mType), mKey(other.mKey), mValues(other.mValues) {
}

GerberAttribute::~GerberAttribute() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString GerberAttribute::toGerberString() const noexcept {
  // Use G04 comments since some PCB fabricators fail to parse X2 attributes.
  // Some day we might provide an option to use real X2 attributes. However,
  // maybe this is not needed at all so let's do it only if it has clear
  // advantages.
  return "G04 #@! " % toString() % "*\n";
}

QString GerberAttribute::toExcellonString() const noexcept {
  return "; #@! " % toString() % "\n";
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

GerberAttribute& GerberAttribute::operator=(
    const GerberAttribute& rhs) noexcept {
  mType = rhs.mType;
  mKey = rhs.mKey;
  mValues = rhs.mValues;
  return *this;
}

bool GerberAttribute::operator==(const GerberAttribute& rhs) const noexcept {
  return (mType == rhs.mType) && (mKey == rhs.mKey) && (mValues == rhs.mValues);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString GerberAttribute::toString() const noexcept {
  QString s = "T";
  switch (mType) {
    case Type::File: {
      s += "F";
      break;
    }
    case Type::Aperture: {
      s += "A";
      break;
    }
    case Type::Object: {
      s += "O";
      break;
    }
    case Type::Delete: {
      s += "D";
      break;
    }
    default: { return QString(); }
  }
  s += mKey;
  foreach (const QString& value, mValues) { s += "," + escapeValue(value); }
  return s;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

GerberAttribute GerberAttribute::unset(const QString& key) noexcept {
  return GerberAttribute(Type::Delete, key, {});
}

GerberAttribute GerberAttribute::fileGenerationSoftware(
    const QString& vendor, const QString& application,
    const QString& version) noexcept {
  QStringList values = {vendor, application};
  if (!version.isEmpty()) {
    values.append(version);
  }
  return GerberAttribute(Type::File, ".GenerationSoftware", values);
}

GerberAttribute GerberAttribute::fileCreationDate(
    const QDateTime& date) noexcept {
  return GerberAttribute(Type::File, ".CreationDate",
                         {date.toString(Qt::ISODate)});
}

GerberAttribute GerberAttribute::fileProjectId(
    const QString& name, const Uuid& uuid, const QString& revision) noexcept {
  return GerberAttribute(Type::File, ".ProjectId",
                         {name, uuid.toStr(), revision});
}

GerberAttribute GerberAttribute::filePartSingle() noexcept {
  return GerberAttribute(Type::File, ".Part", {"Single"});
}

GerberAttribute GerberAttribute::fileMd5(const QString& md5) noexcept {
  return GerberAttribute(Type::File, ".MD5", {md5});
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString GerberAttribute::escapeValue(const QString& value) noexcept {
  // perform compatibility decomposition (NFKD)
  QString ret = value.normalized(QString::NormalizationForm_KD);
  // replace newlines by spaces
  ret = ret.replace('\n', ' ');
  // remove all invalid characters
  // Note: Even if backslashes are allowed, we will remove them because we
  // haven't implemented proper escaping. Escaping of unicode characters is also
  // missing here.
  QString validChars("-a-zA-Z0-9_+/!?<>\"'(){}.|&@# ;$:=");  // No ',' in attrs!
  ret.remove(QRegularExpression(QString("[^%1]").arg(validChars)));
  // limit length to 65535 characters
  ret.truncate(65535);
  return ret;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
