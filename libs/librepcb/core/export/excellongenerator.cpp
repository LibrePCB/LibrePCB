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

#include "../application.h"
#include "../fileio/fileutils.h"
#include "../types/point.h"
#include "../utils/toolbox.h"

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
  : mPlating(plating), mFileAttributes(), mUseG85Slots(false), mOutput() {
  mFileAttributes.append(GerberAttribute::fileGenerationSoftware(
      "LibrePCB", "LibrePCB", Application::getVersion()));
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
  const auto tool = std::make_tuple(*dia, plated, function);
  const NonEmptyPath path{Path({Vertex(pos)})};
  mDrillList.insert(tool, path);
}

void ExcellonGenerator::drill(const NonEmptyPath& path,
                              const PositiveLength& dia, bool plated,
                              Function function) noexcept {
  const auto tool = std::make_tuple(*dia, plated, function);
  mDrillList.insert(tool, path);
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
  const auto tools = mDrillList.uniqueKeys();
  for (int i = 0; i < tools.count(); ++i) {
    bool plated = std::get<1>(tools.at(i));
    Function function = std::get<2>(tools.at(i));
    GerberAttribute apertureFunctionAttribute = (mPlating == Plating::Mixed)
        ? GerberAttribute::apertureFunctionMixedPlatingDrill(plated, function)
        : GerberAttribute::apertureFunction(function);
    mOutput.append(apertureFunctionAttribute.toExcellonString());

    Length dia = std::get<0>(tools.at(i));
    mOutput.append(QString("T%1C%2\n").arg(i + 1).arg(dia.toMmString()));
  }
}

void ExcellonGenerator::printDrills() {
  for (int i = 0; i < mDrillList.uniqueKeys().count(); ++i) {
    mOutput.append(QString("T%1\n").arg(i + 1));  // Select Tool
    auto tool = mDrillList.uniqueKeys().value(i);
    foreach (const NonEmptyPath& path, mDrillList.values(tool)) {
      printPath(path);
    }
  }
}

void ExcellonGenerator::printPath(const NonEmptyPath& path) {
  if (path->getVertices().count() < 1) {
    qCritical() << "Empty path in Excellon export ignored!";
  } else if (path->getVertices().count() == 1) {
    printDrill(path->getVertices().first().getPos());
  } else if (mUseG85Slots) {
    printSlot(path);
  } else {
    printRout(path);
  }
}

void ExcellonGenerator::printDrill(const Point& pos) noexcept {
  mOutput.append(QString("X%1Y%2\n")
                     .arg(pos.getX().toMmString(), pos.getY().toMmString()));
}

void ExcellonGenerator::printSlot(const NonEmptyPath& path) {
  for (int i = 1; i < path->getVertices().count(); ++i) {
    const Vertex& v0 = path->getVertices().at(i - 1);
    const Vertex& v1 = path->getVertices().at(i);
    if (v0.getAngle() != Angle::deg0()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("Using the G85 slot command is not possible for curved slots. "
             "Either remove curved slots or disable the G85 export option."));
    }
    mOutput.append(QString("X%1Y%2G85X%3Y%4\n")
                       .arg(v0.getPos().getX().toMmString(),
                            v0.getPos().getY().toMmString(),
                            v1.getPos().getX().toMmString(),
                            v1.getPos().getY().toMmString()));
  }
}

void ExcellonGenerator::printRout(const NonEmptyPath& path) noexcept {
  printMoveTo(path->getVertices().first().getPos());
  mOutput.append("M15\n");  // Z Axis Route Position
  for (int i = 1; i < path->getVertices().count(); ++i) {
    const Vertex& v0 = path->getVertices().at(i - 1);
    const Vertex& v1 = path->getVertices().at(i);
    if (v0.getAngle() == 0) {
      printLinearInterpolation(v1.getPos());
    } else if (v0.getAngle().abs() > Angle::deg180()) {
      // Split arc as recommended in the XNC format specification from Ucamco.
      const Angle halfAngle = v0.getAngle() / 2;
      const Point center =
          Toolbox::arcCenter(v0.getPos(), v1.getPos(), v0.getAngle());
      const Point middlePos = v0.getPos().rotated(halfAngle, center);
      printCircularInterpolation(v0.getPos(), middlePos, halfAngle);
      printCircularInterpolation(middlePos, v1.getPos(),
                                 v0.getAngle() - halfAngle);
    } else {
      printCircularInterpolation(v0.getPos(), v1.getPos(), v0.getAngle());
    }
  }
  mOutput.append("M16\n");  // Retract With Clamping
  mOutput.append("G05\n");  // Drill Mode
}

void ExcellonGenerator::printMoveTo(const Point& pos) noexcept {
  mOutput.append(QString("G00X%1Y%2\n")
                     .arg(pos.getX().toMmString(), pos.getY().toMmString()));
}

void ExcellonGenerator::printLinearInterpolation(const Point& pos) noexcept {
  mOutput.append(QString("G01X%1Y%2\n")
                     .arg(pos.getX().toMmString(), pos.getY().toMmString()));
}

void ExcellonGenerator::printCircularInterpolation(
    const Point& from, const Point& to, const Angle& angle) noexcept {
  const QString cmd = (angle < 0) ? "G02" : "G03";
  const Length radius = Toolbox::arcRadius(from, to, angle).abs();
  mOutput.append(QString("%1X%2Y%3A%4\n")
                     .arg(cmd, to.getX().toMmString(), to.getY().toMmString(),
                          radius.toMmString()));
}

void ExcellonGenerator::printFooter() noexcept {
  mOutput.append("T0\n");
  mOutput.append("M30\n");  // End of Program Rewind
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
