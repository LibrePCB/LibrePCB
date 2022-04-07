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

#include "../types/uuid.h"

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
  bool strictAscii = true;  // For maximum compatibility with crappy readers!
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
      // ASCII is not sufficient for component values like μ or Ω, thus we
      // allow unicode for such attributes. Note that such attributes should
      // appear in Gerber X3 assembly files anyway (not in PCB data files),
      // so only modern 8X3) readers will need to handle unicode.
      strictAscii = false;
      break;
    }
    case Type::Delete: {
      s += "D";
      break;
    }
    default: { return QString(); }
  }
  s += mKey;
  foreach (const QString& value, mValues) {
    s += "," + escapeValue(value, strictAscii);
  }
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

GerberAttribute GerberAttribute::fileSameCoordinates(
    const QString& identifier) noexcept {
  QStringList values;
  if (!identifier.isEmpty()) {
    values.append(identifier);
  }
  return GerberAttribute(Type::File, ".SameCoordinates", values);
}

GerberAttribute GerberAttribute::fileFunctionProfile(bool plated) noexcept {
  return GerberAttribute(Type::File, ".FileFunction",
                         {"Profile", plated ? "P" : "NP"});
}

GerberAttribute GerberAttribute::fileFunctionCopper(int layer,
                                                    CopperSide side) noexcept {
  QStringList values = {"Copper", "L" % QString::number(layer)};
  switch (side) {
    case CopperSide::Top: {
      values.append("Top");
      break;
    }
    case CopperSide::Inner: {
      values.append("Inr");
      break;
    }
    case CopperSide::Bottom: {
      values.append("Bot");
      break;
    }
    default: {
      qCritical() << "Unknown Gerber copper side:" << static_cast<int>(side);
      return GerberAttribute();
    }
  }
  return GerberAttribute(Type::File, ".FileFunction", values);
}

GerberAttribute GerberAttribute::fileFunctionSolderMask(
    BoardSide side) noexcept {
  switch (side) {
    case BoardSide::Top: {
      return GerberAttribute(Type::File, ".FileFunction",
                             {"Soldermask", "Top"});
    }
    case BoardSide::Bottom: {
      return GerberAttribute(Type::File, ".FileFunction",
                             {"Soldermask", "Bot"});
    }
    default: {
      qCritical() << "Unknown Gerber board side:" << static_cast<int>(side);
      return GerberAttribute();
    }
  }
}

GerberAttribute GerberAttribute::fileFunctionLegend(BoardSide side) noexcept {
  switch (side) {
    case BoardSide::Top: {
      return GerberAttribute(Type::File, ".FileFunction", {"Legend", "Top"});
    }
    case BoardSide::Bottom: {
      return GerberAttribute(Type::File, ".FileFunction", {"Legend", "Bot"});
    }
    default: {
      qCritical() << "Unknown Gerber board side:" << static_cast<int>(side);
      return GerberAttribute();
    }
  }
}

GerberAttribute GerberAttribute::fileFunctionPaste(BoardSide side) noexcept {
  switch (side) {
    case BoardSide::Top: {
      return GerberAttribute(Type::File, ".FileFunction", {"Paste", "Top"});
    }
    case BoardSide::Bottom: {
      return GerberAttribute(Type::File, ".FileFunction", {"Paste", "Bot"});
    }
    default: {
      qCritical() << "Unknown Gerber board side:" << static_cast<int>(side);
      return GerberAttribute();
    }
  }
}

GerberAttribute GerberAttribute::fileFunctionPlatedThroughHole(
    int fromLayer, int toLayer) noexcept {
  return GerberAttribute(
      Type::File, ".FileFunction",
      {"Plated", QString::number(fromLayer), QString::number(toLayer), "PTH"});
}

GerberAttribute GerberAttribute::fileFunctionNonPlatedThroughHole(
    int fromLayer, int toLayer) noexcept {
  return GerberAttribute(Type::File, ".FileFunction",
                         {"NonPlated", QString::number(fromLayer),
                          QString::number(toLayer), "NPTH"});
}

GerberAttribute GerberAttribute::fileFunctionMixedPlating(
    int fromLayer, int toLayer) noexcept {
  // Note that "MixedPlating" is actually not an official Gerber attribute (yet)
  // because Gerber specs say that NPTH and PTH must be separate files. However,
  // some PCB fabricators require to send a single drill file with NPTH and PTH
  // mixed (totally stupid), and in this case, Ucamco recommends to use the
  // "FixedPlating" file function (not publicly documented, I guess).
  return GerberAttribute(
      Type::File, ".FileFunction",
      {"MixedPlating", QString::number(fromLayer), QString::number(toLayer)});
}

GerberAttribute GerberAttribute::filePolarity(Polarity polarity) noexcept {
  switch (polarity) {
    case Polarity::Positive: {
      return GerberAttribute(Type::File, ".FilePolarity", {"Positive"});
    }
    case Polarity::Negative: {
      return GerberAttribute(Type::File, ".FilePolarity", {"Negative"});
    }
    default: {
      qCritical() << "Unknown Gerber file polarity:"
                  << static_cast<int>(polarity);
      return GerberAttribute();
    }
  }
}

GerberAttribute GerberAttribute::fileMd5(const QString& md5) noexcept {
  return GerberAttribute(Type::File, ".MD5", {md5});
}

GerberAttribute GerberAttribute::apertureFunction(
    ApertureFunction function) noexcept {
  switch (function) {
    case ApertureFunction::Profile: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"Profile"});
    }
    case ApertureFunction::ViaDrill: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"ViaDrill"});
    }
    case ApertureFunction::ComponentDrill: {
      return GerberAttribute(Type::Aperture, ".AperFunction",
                             {"ComponentDrill"});
    }
    case ApertureFunction::MechanicalDrill: {
      return GerberAttribute(Type::Aperture, ".AperFunction",
                             {"MechanicalDrill"});
    }
    case ApertureFunction::Conductor: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"Conductor"});
    }
    case ApertureFunction::NonConductor: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"NonConductor"});
    }
    case ApertureFunction::ComponentPad: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"ComponentPad"});
    }
    case ApertureFunction::SmdPadCopperDefined: {
      return GerberAttribute(Type::Aperture, ".AperFunction",
                             {"SMDPad", "CuDef"});
    }
    case ApertureFunction::SmdPadSolderMaskDefined: {
      return GerberAttribute(Type::Aperture, ".AperFunction",
                             {"SMDPad", "SMDef"});
    }
    case ApertureFunction::ViaPad: {
      return GerberAttribute(Type::Aperture, ".AperFunction", {"ViaPad"});
    }
    default: {
      qCritical() << "Unknown Gerber aperture function attribute:"
                  << static_cast<int>(function);
      return GerberAttribute();
    }
  }
}

GerberAttribute GerberAttribute::apertureFunctionMixedPlatingDrill(
    bool plated, ApertureFunction function) noexcept {
  // Note: This function shall only be used in mixed-plating Excellon files!
  // See comment in fileFunctionMixedPlating() for details.
  GerberAttribute a = apertureFunction(function);
  if (plated) {
    a.mValues.prepend("PTH");
    a.mValues.prepend("Plated");
  } else {
    a.mValues.prepend("NPTH");
    a.mValues.prepend("NonPlated");
  }
  return a;
}

GerberAttribute GerberAttribute::objectNet(const QString& net) noexcept {
  return GerberAttribute(Type::Object, ".N", {net});
}

GerberAttribute GerberAttribute::objectComponent(
    const QString& component) noexcept {
  return GerberAttribute(Type::Object, ".C", {component});
}

GerberAttribute GerberAttribute::objectPin(const QString& component,
                                           const QString& pin,
                                           const QString& signal) noexcept {
  QStringList values = {component, pin};
  if (!signal.isEmpty()) {
    values.append(signal);
  }
  return GerberAttribute(Type::Object, ".P", values);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString GerberAttribute::escapeValue(const QString& value,
                                     bool strictAscii) noexcept {
  QString ret = value;
  // remove CRLF newlines
  ret.remove('\r');
  // replace newlines by spaces
  ret.replace('\n', ' ');
  if (strictAscii) {
    // perform compatibility decomposition (NFKD)
    ret = ret.normalized(QString::NormalizationForm_KD);
    // remove all invalid characters for maximum compatibility with readers
    QString validChars("-a-zA-Z0-9_+/!?<>\"'(){}.|&@# ;$:=");
    ret.remove(QRegularExpression(QString("[^%1]").arg(validChars)));
  } else {
    // escape backslash
    ret.replace("\\", "\\u005C");
    // escape '%'
    ret.replace("%", "\\u0025");
    // escape '*'
    ret.replace("*", "\\u002A");
    // escape ','
    ret.replace(",", "\\u002C");
  }

  // limit length to 65535 characters
  ret.truncate(65535);
  return ret;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
