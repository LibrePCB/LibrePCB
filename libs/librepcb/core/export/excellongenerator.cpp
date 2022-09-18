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
#include "excellongenerator.h"

#include "../fileio/fileutils.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ExcellonGenerator::ExcellonGenerator(const QDateTime& creationDate,
                                     const QString& projName,
                                     const Uuid& projUuid,
                                     const QString& projRevision,
                                     Plating plating, int fromLayer,
                                     int toLayer) noexcept
  : mPlating(plating), mFileAttributes(), mOutput() {
  mFileAttributes.append(GerberAttribute::fileGenerationSoftware(
      "LibrePCB", "LibrePCB", qApp->applicationVersion()));
  mFileAttributes.append(GerberAttribute::fileCreationDate(creationDate));
  mFileAttributes.append(
      GerberAttribute::fileProjectId(projName, projUuid, projRevision));
  mFileAttributes.append(GerberAttribute::filePartSingle());
  mFileAttributes.append(GerberAttribute::fileSameCoordinates(QString()));

  switch (plating) {
    case Plating::Yes: {
      mFileAttributes.append(
          GerberAttribute::fileFunctionPlatedThroughHole(fromLayer, toLayer));
      break;
    }
    case Plating::No: {
      mFileAttributes.append(GerberAttribute::fileFunctionNonPlatedThroughHole(
          fromLayer, toLayer));
      break;
    }
    case Plating::Mixed: {
      mFileAttributes.append(
          GerberAttribute::fileFunctionMixedPlating(fromLayer, toLayer));
      break;
    }
    default: {
      qCritical()
          << "Unhandled switch-case in ExcellonGenerator::ExcellonGenerator():"
          << static_cast<int>(plating);
      break;
    }
  }
}

ExcellonGenerator::~ExcellonGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ExcellonGenerator::drill(const Point& pos, const PositiveLength& dia,
                              bool plated, Function function) noexcept {
  mDrillList.insert(std::make_tuple(*dia, plated, function), pos);
}

void ExcellonGenerator::generate() {
  mOutput.clear();
  printHeader();
  printDrills();
  printFooter();
}

void ExcellonGenerator::saveToFile(const FilePath& filepath) const {
  FileUtils::writeFile(filepath, mOutput.toLatin1());  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ExcellonGenerator::printHeader() noexcept {
  mOutput.append("M48\n");  // Beginning of Part Program Header

  // Add file attributes.
  foreach (const GerberAttribute& a, mFileAttributes) {
    mOutput.append(a.toExcellonString());
  }

  mOutput.append("FMAT,2\n");  // Use Format 2 commands
  mOutput.append("METRIC,TZ\n");  // Metric Format, Trailing Zeros Mode

  printToolList();

  mOutput.append("%\n");  // Beginning of Pattern
  mOutput.append("G90\n");  // Absolute Mode
  mOutput.append("G05\n");  // Drill Mode
  mOutput.append("M71\n");  // Metric Measuring Mode
}

void ExcellonGenerator::printToolList() noexcept {
  for (int i = 0; i < mDrillList.uniqueKeys().count(); ++i) {
    bool plated = std::get<1>(mDrillList.uniqueKeys().value(i));
    Function function = std::get<2>(mDrillList.uniqueKeys().value(i));
    GerberAttribute apertureFunctionAttribute = (mPlating == Plating::Mixed)
        ? GerberAttribute::apertureFunctionMixedPlatingDrill(plated, function)
        : GerberAttribute::apertureFunction(function);
    mOutput.append(apertureFunctionAttribute.toExcellonString());

    Length dia = std::get<0>(mDrillList.uniqueKeys().value(i));
    mOutput.append(QString("T%1C%2\n").arg(i + 1).arg(dia.toMmString()));
  }
}

void ExcellonGenerator::printDrills() noexcept {
  for (int i = 0; i < mDrillList.uniqueKeys().count(); ++i) {
    mOutput.append(QString("T%1\n").arg(i + 1));  // Select Tool
    auto tool = mDrillList.uniqueKeys().value(i);
    foreach (const Point& pos, mDrillList.values(tool)) {
      mOutput.append(
          QString("X%1Y%2\n")
              .arg(pos.getX().toMmString(), pos.getY().toMmString()));
    }
  }
}

void ExcellonGenerator::printFooter() noexcept {
  mOutput.append("T0\n");
  mOutput.append("M30\n");  // End of Program Rewind
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
