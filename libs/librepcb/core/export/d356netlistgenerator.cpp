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
#include "d356netlistgenerator.h"

#include "../types/angle.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

D356NetlistGenerator::D356NetlistGenerator(
    const QString& projName, const QString& projRevision,
    const QString& brdName, const QDateTime& generationDate) noexcept
  : mComments(), mRecords() {
  mComments.append("IPC-D-356A Netlist");
  mComments.append("");
  mComments.append("Project Name:        " % projName);
  mComments.append("Project Version:     " % projRevision);
  mComments.append("Board Name:          " % brdName);
  mComments.append("Generation Software: LibrePCB " %
                   qApp->applicationVersion());
  mComments.append("Generation Date:     " %
                   generationDate.toString(Qt::ISODate));
  mComments.append("");
  mComments.append(
      "Note that due to limitations of this file format, LibrePCB");
  mComments.append("applies the following operations during the export:");
  mComments.append("  - suffix net names with unique numbers within braces");
  mComments.append(
      "  - truncate long net names (uniqueness guaranteed by suffix)");
  mComments.append(
      "  - truncate long component names (uniqueness not guaranteed)");
  mComments.append("  - truncate long pad names (uniqueness not guaranteed)");
  mComments.append("  - clip drill/pad sizes to 9.999mm");
  mComments.append("");
}

D356NetlistGenerator::~D356NetlistGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void D356NetlistGenerator::smtPad(const QString& netName,
                                  const QString& cmpName,
                                  const QString& padName, const Point& position,
                                  const PositiveLength& width,
                                  const PositiveLength& height,
                                  const Angle& rotation, int layer) {
  mRecords.append(Record{
      OperationCode::SurfaceMount, netName, checkedComponentName(cmpName),
      padName, false, tl::nullopt, layer, position, width, height, rotation,
      (layer == 1) ? SolderMask::SecondarySide : SolderMask::PrimarySide});
}

void D356NetlistGenerator::thtPad(const QString& netName,
                                  const QString& cmpName,
                                  const QString& padName, const Point& position,
                                  const PositiveLength& width,
                                  const PositiveLength& height,
                                  const Angle& rotation,
                                  const PositiveLength& drillDiameter) {
  mRecords.append(Record{OperationCode::ThroughHole, netName,
                         checkedComponentName(cmpName), padName, false,
                         std::make_pair(drillDiameter, true), 0, position,
                         width, height, rotation, SolderMask::None});
}

void D356NetlistGenerator::via(const QString& netName, const Point& position,
                               const PositiveLength& width,
                               const PositiveLength& height,
                               const Angle& rotation,
                               const PositiveLength& drillDiameter,
                               bool solderMaskCovered) {
  mRecords.append(Record{
      OperationCode::ThroughHole, netName, "VIA", QString(), true,
      std::make_pair(drillDiameter, true), 0, position, width, height, rotation,
      solderMaskCovered ? SolderMask::BothSides : SolderMask::None});
}

QByteArray D356NetlistGenerator::generate() const {
  QStringList lines;

  // Add header.
  foreach (const QString& comment, mComments) {
    // Limit length to 80 characters in total (with or without newline?).
    lines.append(cleanString("C  " % comment).left(79));
  }
  lines.append("P  UNITS CUST 1");  // Millimeters / degrees

  // Guarantee unique signal names by adding their index as a suffix.
  QHash<QString, QString> signalNameMap;
  const int signalNameLength = 14;
  foreach (const Record& record, mRecords) {
    QString name = record.signalName;
    if (!signalNameMap.contains(name)) {
      if (name.isEmpty()) {
        name = "N/C";
      } else {
        const QString nbr = QString("{%1}").arg(signalNameMap.count() + 1);
        name = cleanString(name).left(signalNameLength - nbr.length()) % nbr;
      }
      signalNameMap[record.signalName] = name;
    }
  }

  // Add records.
  foreach (const Record& record, mRecords) {
    QString line;
    line += QString("%1").arg(static_cast<int>(record.code), 3, 10,
                              QLatin1Char('0'));
    line +=
        QString("%1").arg(signalNameMap[record.signalName], -signalNameLength);
    line += "   ";
    line += QString("%1").arg(cleanString(record.componentName).left(6), -6);
    line += record.padName.isEmpty() ? " " : "-";
    line += QString("%1").arg(cleanString(record.padName).left(4), -4);
    line += record.midPoint ? "M" : " ";
    if (const auto hole = record.hole) {
      line += QString("D%1%2")
                  .arg(formatLength(*hole->first, false, 4))
                  .arg(hole->second ? "P" : "U");
    } else {
      line += "      ";
    }
    line += QString("A%1").arg(record.accessCode, 2, 10, QLatin1Char('0'));
    line += QString("X%1Y%2")
                .arg(formatLength(record.position.getX(), true, 6))
                .arg(formatLength(record.position.getY(), true, 6));
    line += QString("X%1Y%2")
                .arg(formatLength(*record.width, false, 4))
                .arg(formatLength(*record.height, false, 4));
    line += QString("R%1").arg(record.rotation.mappedTo0_360deg().toDeg(), 3,
                               'f', 0, '0');
    line += " ";
    line += QString("S%1").arg(static_cast<int>(record.solderMask));
    lines.append(line);
  }

  // Add footer, including a final linebreak.
  lines.append("999\n");

  // Make sure there are no non-ASCII characters in the file.
  return lines.join("\n").toLatin1();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString D356NetlistGenerator::cleanString(QString str) noexcept {
  // Remove CRLF newlines.
  str.remove('\r');

  // Replace newlines by spaces.
  str.replace('\n', ' ');

  // Perform compatibility decomposition (NFKD).
  str = str.normalized(QString::NormalizationForm_KD);

  // Remove all invalid characters for maximum compatibility with readers.
  const QString validChars("-a-zA-Z0-9_+/!?<>\"'(){}.|&@# ,;$:=~");
  str.remove(QRegularExpression(QString("[^%1]").arg(validChars)));

  return str;
}

QString D356NetlistGenerator::checkedComponentName(
    const QString& name) noexcept {
  if (name.toLower() == "via") {
    return name % "_";
  } else {
    return name;
  }
}

QString D356NetlistGenerator::formatLength(const Length& value, bool isSigned,
                                           int digits) noexcept {
  QString str =
      QString("%1").arg(value.abs().toMicrometers(), digits, 'f', 0, '0');
  if (str.length() > digits) {
    qWarning() << "Too large number in IPC-D-356A export clipped!";
    str = QString("9").repeated(digits);
  }
  if (isSigned) {
    str.prepend((value < 0) ? "-" : "+");
  }
  return str;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
