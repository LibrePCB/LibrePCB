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
#include "gerbergenerator.h"

#include "../fileio/fileutils.h"
#include "../geometry/circle.h"
#include "../geometry/path.h"
#include "../toolbox.h"
#include "gerberaperturelist.h"
#include "gerberattribute.h"
#include "gerberattributewriter.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GerberGenerator::GerberGenerator(const QDateTime& creationDate,
                                 const QString& projName, const Uuid& projUuid,
                                 const QString& projRevision) noexcept
  : mOutput(),
    mContent(),
    mAttributeWriter(new GerberAttributeWriter()),
    mApertureList(new GerberApertureList()),
    mCurrentApertureNumber(-1) {
  mFileAttributes.append(GerberAttribute::fileGenerationSoftware(
      "LibrePCB", "LibrePCB", qApp->applicationVersion()));
  mFileAttributes.append(GerberAttribute::fileCreationDate(creationDate));
  mFileAttributes.append(
      GerberAttribute::fileProjectId(projName, projUuid, projRevision));
  mFileAttributes.append(GerberAttribute::filePartSingle());
  mFileAttributes.append(GerberAttribute::fileSameCoordinates(QString()));
}

GerberGenerator::~GerberGenerator() noexcept {
}

/*******************************************************************************
 *  Plot Methods
 ******************************************************************************/

void GerberGenerator::setFileFunctionOutlines(bool plated) noexcept {
  mFileAttributes.append(GerberAttribute::fileFunctionProfile(plated));
}

void GerberGenerator::setFileFunctionCopper(int layer, CopperSide side,
                                            Polarity polarity) noexcept {
  mFileAttributes.append(GerberAttribute::fileFunctionCopper(layer, side));
  mFileAttributes.append(GerberAttribute::filePolarity(polarity));
}

void GerberGenerator::setFileFunctionSolderMask(BoardSide side,
                                                Polarity polarity) noexcept {
  mFileAttributes.append(GerberAttribute::fileFunctionSolderMask(side));
  mFileAttributes.append(GerberAttribute::filePolarity(polarity));
}

void GerberGenerator::setFileFunctionLegend(BoardSide side,
                                            Polarity polarity) noexcept {
  mFileAttributes.append(GerberAttribute::fileFunctionLegend(side));
  mFileAttributes.append(GerberAttribute::filePolarity(polarity));
}

void GerberGenerator::setFileFunctionPaste(BoardSide side,
                                           Polarity polarity) noexcept {
  mFileAttributes.append(GerberAttribute::fileFunctionPaste(side));
  mFileAttributes.append(GerberAttribute::filePolarity(polarity));
}

void GerberGenerator::setLayerPolarity(Polarity p) noexcept {
  switch (p) {
    case Polarity::Positive:
      mContent.append("%LPD*%\n");
      break;
    case Polarity::Negative:
      mContent.append("%LPC*%\n");
      break;
    default:
      qCritical() << "Invalid Layer Polarity:" << static_cast<int>(p);
      break;
  }
}

void GerberGenerator::drawLine(const Point& start, const Point& end,
                               const UnsignedLength& width, Function function,
                               const tl::optional<QString>& net,
                               const QString& component) noexcept {
  setCurrentAperture(mApertureList->addCircle(width, function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       QString(),  // Object: Pin number/name
                       QString()  // Object: Pin signal
  );
  moveToPosition(start);
  linearInterpolateToPosition(end);
}

void GerberGenerator::drawPathOutline(const Path& path,
                                      const UnsignedLength& lineWidth,
                                      Function function,
                                      const tl::optional<QString>& net,
                                      const QString& component) noexcept {
  if (path.getVertices().count() < 2) {
    qWarning() << "Invalid path was ignored in gerber output!";
    return;
  }
  setCurrentAperture(mApertureList->addCircle(lineWidth, function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       QString(),  // Object: Pin number/name
                       QString()  // Object: Pin signal
  );
  moveToPosition(path.getVertices().first().getPos());
  for (int i = 1; i < path.getVertices().count(); ++i) {
    const Vertex& v = path.getVertices().at(i);
    const Vertex& v0 = path.getVertices().at(i - 1);
    interpolateBetween(v0, v);
  }
}

void GerberGenerator::drawPathArea(const Path& path, Function function,
                                   const tl::optional<QString>& net,
                                   const QString& component) noexcept {
  if (!path.isClosed()) {
    qWarning() << "Non-closed path was ignored in gerber output!";
    return;
  }
  // Note: Actually G36/G37 regions do not have an aperture attached. But for
  // compatibility reasons, it's better to still select an aperture as usual.
  // We used an aperture of size 0, but this already caused some issues in
  // the past (although not critical) and the Gerber specs recommends to not
  // use zero-size apertures. So let's use an aperture size of 0.01mm (it has
  // no impact on the rendered image anyway).
  setCurrentAperture(mApertureList->addCircle(UnsignedLength(10000), function));
  setCurrentAttributes(function,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       QString(),  // Object: Pin number/name
                       QString()  // Object: Pin signal
  );
  setRegionModeOn();
  moveToPosition(path.getVertices().first().getPos());
  for (int i = 1; i < path.getVertices().count(); ++i) {
    const Vertex& v = path.getVertices().at(i);
    const Vertex& v0 = path.getVertices().at(i - 1);
    interpolateBetween(v0, v);
  }
  setRegionModeOff();
}

void GerberGenerator::flashCircle(const Point& pos, const PositiveLength& dia,
                                  Function function,
                                  const tl::optional<QString>& net,
                                  const QString& component, const QString& pin,
                                  const QString& signal) noexcept {
  setCurrentAperture(
      mApertureList->addCircle(positiveToUnsigned(dia), function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       pin,  // Object: Pin number/name
                       signal  // Object: Pin signal
  );
  flashAtPosition(pos);
}

void GerberGenerator::flashRect(const Point& pos, const PositiveLength& w,
                                const PositiveLength& h, const Angle& rot,
                                Function function,
                                const tl::optional<QString>& net,
                                const QString& component, const QString& pin,
                                const QString& signal) noexcept {
  setCurrentAperture(mApertureList->addRect(w, h, rot, function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       pin,  // Object: Pin number/name
                       signal  // Object: Pin signal
  );
  flashAtPosition(pos);
}

void GerberGenerator::flashObround(const Point& pos, const PositiveLength& w,
                                   const PositiveLength& h, const Angle& rot,
                                   Function function,
                                   const tl::optional<QString>& net,
                                   const QString& component, const QString& pin,
                                   const QString& signal) noexcept {
  setCurrentAperture(mApertureList->addObround(w, h, rot, function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       pin,  // Object: Pin number/name
                       signal  // Object: Pin signal
  );
  flashAtPosition(pos);
}

void GerberGenerator::flashOctagon(const Point& pos, const PositiveLength& w,
                                   const PositiveLength& h, const Angle& rot,
                                   Function function,
                                   const tl::optional<QString>& net,
                                   const QString& component, const QString& pin,
                                   const QString& signal) noexcept {
  setCurrentAperture(mApertureList->addOctagon(w, h, rot, function));
  setCurrentAttributes(tl::nullopt,  // Aperture: Function
                       net,  // Object: Net name
                       component,  // Object: Component designator
                       pin,  // Object: Pin number/name
                       signal  // Object: Pin signal
  );
  flashAtPosition(pos);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GerberGenerator::generate() {
  mOutput.clear();
  printHeader();
  printApertureList();
  printContent();
  printFooter();
}

void GerberGenerator::saveToFile(const FilePath& filepath) const {
  FileUtils::writeFile(filepath, mOutput.toLatin1());  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GerberGenerator::setCurrentAttributes(Function apertureFunction,
                                           const tl::optional<QString>& netName,
                                           const QString& componentDesignator,
                                           const QString& pinName,
                                           const QString& pinSignal) noexcept {
  QList<GerberAttribute> attributes;
  if (apertureFunction) {
    attributes.append(GerberAttribute::apertureFunction(*apertureFunction));
  }
  if (netName) {
    attributes.append(GerberAttribute::objectNet(*netName));
  }
  if (!componentDesignator.isEmpty()) {
    attributes.append(GerberAttribute::objectComponent(componentDesignator));
  }
  if (!componentDesignator.isEmpty() && !pinName.isEmpty()) {
    attributes.append(
        GerberAttribute::objectPin(componentDesignator, pinName, pinSignal));
  }
  mContent.append(mAttributeWriter->setAttributes(attributes));
}

void GerberGenerator::setCurrentAperture(int number) noexcept {
  if (number != mCurrentApertureNumber) {
    mContent.append(QString("D%1*\n").arg(number));
    mCurrentApertureNumber = number;
  }
}

void GerberGenerator::setRegionModeOn() noexcept {
  mContent.append("G36*\n");
}

void GerberGenerator::setRegionModeOff() noexcept {
  mContent.append("G37*\n");
}

void GerberGenerator::switchToLinearInterpolationModeG01() noexcept {
  mContent.append("G01*\n");
}

void GerberGenerator::switchToCircularCwInterpolationModeG02() noexcept {
  mContent.append("G02*\n");
}

void GerberGenerator::switchToCircularCcwInterpolationModeG03() noexcept {
  mContent.append("G03*\n");
}

void GerberGenerator::moveToPosition(const Point& pos) noexcept {
  mContent.append(QString("X%1Y%2D02*\n")
                      .arg(pos.getX().toNmString(), pos.getY().toNmString()));
}

void GerberGenerator::linearInterpolateToPosition(const Point& pos) noexcept {
  mContent.append(QString("X%1Y%2D01*\n")
                      .arg(pos.getX().toNmString(), pos.getY().toNmString()));
}

void GerberGenerator::circularInterpolateToPosition(const Point& start,
                                                    const Point& center,
                                                    const Point& end) noexcept {
  Point diff = center - start;
  mContent.append(QString("X%1Y%2I%3J%4D01*\n")
                      .arg(end.getX().toNmString(), end.getY().toNmString(),
                           diff.getX().toNmString(), diff.getY().toNmString()));
}

void GerberGenerator::interpolateBetween(const Vertex& from,
                                         const Vertex& to) noexcept {
  if (from.getAngle() == 0) {
    // linear segment
    linearInterpolateToPosition(to.getPos());
  } else {
    // arc segment
    if (from.getAngle() < 0) {
      switchToCircularCwInterpolationModeG02();
    } else {
      switchToCircularCcwInterpolationModeG03();
    }
    Point center =
        Toolbox::arcCenter(from.getPos(), to.getPos(), from.getAngle());
    circularInterpolateToPosition(from.getPos(), center, to.getPos());
    switchToLinearInterpolationModeG01();
  }
}

void GerberGenerator::flashAtPosition(const Point& pos) noexcept {
  mContent.append(QString("X%1Y%2D03*\n")
                      .arg(pos.getX().toNmString(), pos.getY().toNmString()));
}

void GerberGenerator::printHeader() noexcept {
  mOutput.append("G04 --- HEADER BEGIN --- *\n");

  // Add file attributes.
  foreach (const GerberAttribute& a, mFileAttributes) {
    mOutput.append(a.toGerberString());
  }

  // coordinate format specification:
  //  - leading zeros omitted
  //  - absolute coordinates
  //  - coordiante format "6.6" --> allows us to directly use LengthBase_t
  //  (nanometers)!
  mOutput.append("%FSLAX66Y66*%\n");

  // set unit to millimeters
  mOutput.append("%MOMM*%\n");

  // start linear interpolation mode
  mOutput.append("G01*\n");

  // Use multi quadrant arc mode (single quadrant mode is buggy in some CAM
  // software and is now deprecated in the current Gerber specs).
  // See https://github.com/LibrePCB/LibrePCB/issues/247.
  mOutput.append("G75*\n");

  mOutput.append("G04 --- HEADER END --- *\n");
}

void GerberGenerator::printApertureList() noexcept {
  mOutput.append("G04 --- APERTURE LIST BEGIN --- *\n");
  mOutput.append(mApertureList->generateString());
  mOutput.append("G04 --- APERTURE LIST END --- *\n");
}

void GerberGenerator::printContent() noexcept {
  mOutput.append("G04 --- BOARD BEGIN --- *\n");
  mOutput.append(mContent);
  mOutput.append("G04 --- BOARD END --- *\n");
}

void GerberGenerator::printFooter() noexcept {
  // MD5 checksum over content
  mOutput.append(
      GerberAttribute::fileMd5(calcOutputMd5Checksum()).toGerberString());

  // end of file
  mOutput.append("M02*\n");
}

QString GerberGenerator::calcOutputMd5Checksum() const noexcept {
  // according to the RS-274C standard, linebreaks are not included in the
  // checksum
  QString data = QString(mOutput).remove(QChar('\n'));
  return QString(
      QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5).toHex());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
