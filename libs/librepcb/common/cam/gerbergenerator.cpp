/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "gerbergenerator.h"
#include "gerberaperturelist.h"
#include "../geometry/circle.h"
#include "../geometry/path.h"
#include "../fileio/smarttextfile.h"
#include "../application.h"
#include "../toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GerberGenerator::GerberGenerator(const QString& projName, const Uuid& projUuid,
                                 const QString& projRevision) noexcept :
    mProjectId(escapeString(projName)), mProjectUuid(projUuid),
    mProjectRevision(escapeString(projRevision)), mOutput(), mContent(),
    mApertureList(new GerberApertureList()), mCurrentApertureNumber(-1),
    mMultiQuadrantArcModeOn(false)
{
}

GerberGenerator::~GerberGenerator() noexcept
{
}

/*****************************************************************************************
 *  Plot Methods
 ****************************************************************************************/

void GerberGenerator::setLayerPolarity(LayerPolarity p) noexcept
{
    switch (p)
    {
        case LayerPolarity::Positive: mContent.append("%LPD*%\n"); break;
        case LayerPolarity::Negative: mContent.append("%LPC*%\n"); break;
        default: qCritical() << "Invalid Layer Polarity:" << static_cast<int>(p); break;
    }
}

void GerberGenerator::drawLine(const Point& start, const Point& end, const Length& width) noexcept
{
    setCurrentAperture(mApertureList->setCircle(width, Length(0)));
    moveToPosition(start);
    linearInterpolateToPosition(end);
}

void GerberGenerator::drawEllipseOutline(const Ellipse& ellipse) noexcept
{
    if (ellipse.getRadiusX() == ellipse.getRadiusY()) {
        Length outerDia = (ellipse.getRadiusX() * 2) + ellipse.getLineWidth();
        Length innerDia = (ellipse.getRadiusX() * 2) - ellipse.getLineWidth();
        if (innerDia < 0) innerDia = 0;
        flashCircle(ellipse.getCenter(), outerDia, innerDia);
    } else {
        // TODO!
        qWarning() << "Ellipse was ignored in gerber output!";
    }
}

void GerberGenerator::drawEllipseArea(const Ellipse& ellipse) noexcept
{
    if (ellipse.getRadiusX() == ellipse.getRadiusY()) {
        flashCircle(ellipse.getCenter(), ellipse.getRadiusX() * 2, Length(0));
    } else {
        // TODO!
        qWarning() << "Ellipse was ignored in gerber output!";
    }
}

void GerberGenerator::drawPathOutline(const Path& path, const Length& lineWidth) noexcept
{
    if (path.getVertices().count() < 2) {
        qWarning() << "Invalid path was ignored in gerber output!";
        return;
    }
    setCurrentAperture(mApertureList->setCircle(lineWidth, Length(0)));
    moveToPosition(path.getVertices().first().getPos());
    for (int i = 1; i < path.getVertices().count(); ++i) {
        const Vertex& v = path.getVertices().at(i);
        const Vertex& v0 = path.getVertices().at(i-1);
        if (v0.getAngle() == 0) {
            // linear segment
            linearInterpolateToPosition(v.getPos());
        } else {
            // arc segment
            if (v0.getAngle().abs() <= Angle::deg90()) {
                setMultiQuadrantArcModeOff();
            } else {
                setMultiQuadrantArcModeOn();
            }
            if (v0.getAngle() < 0) {
                switchToCircularCwInterpolationModeG02();
            } else {
                switchToCircularCcwInterpolationModeG03();
            }
            Point center = Toolbox::arcCenter(v0.getPos(), v.getPos(), v0.getAngle());
            circularInterpolateToPosition(v0.getPos(), center, v.getPos());
            switchToLinearInterpolationModeG01();
        }
    }
}

void GerberGenerator::drawPathArea(const Path& path) noexcept
{
    if (!path.isClosed()) {
        qWarning() << "Non-closed path was ignored in gerber output!";
        return;
    }
    setCurrentAperture(mApertureList->setCircle(Length(0), Length(0)));
    setRegionModeOn();
    moveToPosition(path.getVertices().first().getPos());
    for (int i = 1; i < path.getVertices().count(); ++i) {
        const Vertex& v = path.getVertices().at(i);
        const Vertex& v0 = path.getVertices().at(i-1);
        if (v0.getAngle() == 0) {
            // linear segment
            linearInterpolateToPosition(v.getPos());
        } else {
            // arc segment
            if (v0.getAngle().abs() <= Angle::deg90()) {
                setMultiQuadrantArcModeOff();
            } else {
                setMultiQuadrantArcModeOn();
            }
            if (v0.getAngle() < 0) {
                switchToCircularCwInterpolationModeG02();
            } else {
                switchToCircularCcwInterpolationModeG03();
            }
            Point center = Toolbox::arcCenter(v0.getPos(), v.getPos(), v0.getAngle());
            circularInterpolateToPosition(v0.getPos(), center, v.getPos());
            switchToLinearInterpolationModeG01();
        }
    }
    setRegionModeOff();
}

void GerberGenerator::flashCircle(const Point& pos, const Length& dia, const Length& hole) noexcept
{
    setCurrentAperture(mApertureList->setCircle(dia, hole));
    flashAtPosition(pos);
}

void GerberGenerator::flashRect(const Point& pos, const Length& w, const Length& h,
                                const Angle& rot, const Length& hole) noexcept
{
    setCurrentAperture(mApertureList->setRect(w, h, rot, hole));
    flashAtPosition(pos);
}

void GerberGenerator::flashObround(const Point& pos, const Length& w, const Length& h,
                                   const Angle& rot, const Length& hole) noexcept
{
    setCurrentAperture(mApertureList->setObround(w, h, rot, hole));
    flashAtPosition(pos);
}

void GerberGenerator::flashRegularPolygon(const Point& pos, const Length& dia, int n,
                                          const Angle& rot, const Length& hole) noexcept
{
    setCurrentAperture(mApertureList->setRegularPolygon(dia, n, rot, hole));
    flashAtPosition(pos);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GerberGenerator::reset() noexcept
{
    mOutput.clear();
    mContent.clear();
    mApertureList->reset();
    mCurrentApertureNumber = -1;
}

void GerberGenerator::generate()
{
    mOutput.clear();
    printHeader();
    printApertureList();
    printContent();
    printFooter();
}

void GerberGenerator::saveToFile(const FilePath& filepath) const
{
    QScopedPointer<SmartTextFile> file(SmartTextFile::create(filepath));
    file->setContent(mOutput.toLatin1());
    file->save(true);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void GerberGenerator::setCurrentAperture(int number) noexcept
{
    if (number != mCurrentApertureNumber) {
        mContent.append(QString("D%1*\n").arg(number));
        mCurrentApertureNumber = number;
    }
}

void GerberGenerator::setRegionModeOn() noexcept
{
    mContent.append("G36*\n");
}

void GerberGenerator::setRegionModeOff() noexcept
{
    mContent.append("G37*\n");
}

void GerberGenerator::setMultiQuadrantArcModeOn() noexcept
{
    if (!mMultiQuadrantArcModeOn) {
        mContent.append("G75*\n");
        mMultiQuadrantArcModeOn = true;
    }
}

void GerberGenerator::setMultiQuadrantArcModeOff() noexcept
{
    if (mMultiQuadrantArcModeOn) {
        mContent.append("G74*\n");
        mMultiQuadrantArcModeOn = false;
    }
}

void GerberGenerator::switchToLinearInterpolationModeG01() noexcept
{
    mContent.append("G01*\n");
}

void GerberGenerator::switchToCircularCwInterpolationModeG02() noexcept
{
    mContent.append("G02*\n");
}

void GerberGenerator::switchToCircularCcwInterpolationModeG03() noexcept
{
    mContent.append("G03*\n");
}

void GerberGenerator::moveToPosition(const Point& pos) noexcept
{
    mContent.append(QString("X%1Y%2D02*\n").arg(pos.getX().toNmString(),
                                                pos.getY().toNmString()));
}

void GerberGenerator::linearInterpolateToPosition(const Point& pos) noexcept
{
    mContent.append(QString("X%1Y%2D01*\n").arg(pos.getX().toNmString(),
                                                pos.getY().toNmString()));
}

void GerberGenerator::circularInterpolateToPosition(const Point& start, const Point& center, const Point& end) noexcept
{
    Point diff = center - start;
    if (!mMultiQuadrantArcModeOn) {
        diff.makeAbs(); // no sign allowed in single quadrant mode!
    }
    mContent.append(QString("X%1Y%2I%3J%4D01*\n").arg(end.getX().toNmString(),
                                                      end.getY().toNmString(),
                                                      diff.getX().toNmString(),
                                                      diff.getY().toNmString()));
}

void GerberGenerator::flashAtPosition(const Point& pos) noexcept
{
    mContent.append(QString("X%1Y%2D03*\n").arg(pos.getX().toNmString(),
                                                pos.getY().toNmString()));
}

void GerberGenerator::printHeader() noexcept
{
    mOutput.append("G04 --- HEADER BEGIN --- *\n");

    // add some X2 attributes
    QString appVersion = qApp->getAppVersion().toPrettyStr(3);
    QString creationDate = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString projId = mProjectId.remove(',');
    QString projUuid = mProjectUuid.toStr();
    QString projRevision = mProjectRevision.remove(',');
    mOutput.append(QString("%TF.GenerationSoftware,LibrePCB,LibrePCB,%1*%\n").arg(appVersion));
    mOutput.append(QString("%TF.CreationDate,%1*%\n").arg(creationDate));
    mOutput.append(QString("%TF.ProjectId,%1,%2,%3*%\n").arg(projId, projUuid, projRevision));
    mOutput.append("%TF.Part,Single*%\n"); // "Single" means "this is a PCB"
    //mOutput.append("%TF.FilePolarity,Positive*%\n");

    // coordinate format specification:
    //  - leading zeros omitted
    //  - absolute coordinates
    //  - coordiante format "6.6" --> allows us to directly use LengthBase_t (nanometers)!
    mOutput.append("%FSLAX66Y66*%\n");

    // set unit to millimeters
    mOutput.append("%MOMM*%\n");

    // start linear interpolation mode
    mOutput.append("G01*\n");

    // use single quadrant arc mode
    mOutput.append("G74*\n");

    mOutput.append("G04 --- HEADER END --- *\n");
}

void GerberGenerator::printApertureList() noexcept
{
    mOutput.append(mApertureList->generateString());
}

void GerberGenerator::printContent() noexcept
{
    mOutput.append("G04 --- BOARD BEGIN --- *\n");
    mOutput.append(mContent);
    mOutput.append("G04 --- BOARD END --- *\n");
}

void GerberGenerator::printFooter() noexcept
{
    // MD5 checksum over content
    mOutput.append(QString("%TF.MD5,%1*%\n").arg(calcOutputMd5Checksum()));

    // end of file
    mOutput.append("M02*\n");
}

QString GerberGenerator::calcOutputMd5Checksum() const noexcept
{
    // according to the RS-274C standard, linebreaks are not included in the checksum
    QString data = QString(mOutput).remove(QChar('\n'));
    return QString(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5).toHex());
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QString GerberGenerator::escapeString(const QString& str) noexcept
{
    // perform compatibility decomposition (NFKD)
    QString ret = str.normalized(QString::NormalizationForm_KD);
    // remove all invalid characters
    // Note: Even if backslashes are allowed, we will remove them because we haven't
    // implemented proper escaping. Escaping of unicode characters is also missing here.
    QString validChars("[a-zA-Z0-9_+-/!?<>”’(){}.|&@# ,;$:=]");
    ret.remove(QRegularExpression(QString("[^%1]").arg(validChars)));
    // limit length to 65535 characters
    ret.truncate(65535);
    return ret;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
