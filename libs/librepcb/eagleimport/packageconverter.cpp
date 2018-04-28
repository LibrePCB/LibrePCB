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
#include "packageconverter.h"
#include "converterdb.h"
#include <parseagle/package/package.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/pkg/footprint.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace eagleimport {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageConverter::PackageConverter(const parseagle::Package& package, ConverterDb& db) noexcept :
    mPackage(package), mDb(db)
{
}

PackageConverter::~PackageConverter() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

std::unique_ptr<library::Package> PackageConverter::generate() const
{
    std::shared_ptr<library::Footprint> footprint(
        new library::Footprint(mDb.getFootprintUuid(mPackage.getName()), "default", ""));

    std::unique_ptr<library::Package> package(
        new library::Package(mDb.getPackageUuid(mPackage.getName()), Version("0.1"),
                             "LibrePCB", mPackage.getName(), createDescription(), ""));

    foreach (const parseagle::Wire& wire, mPackage.getWires()) {
        QString layerName = convertBoardLayer(wire.getLayer());
        bool fill = false;
        bool isGrabArea = true;
        Length lineWidth = Length::fromMm(wire.getWidth());
        Point startpos = Point::fromMm(wire.getP1().x, wire.getP1().y);
        Point endpos = Point::fromMm(wire.getP2().x, wire.getP2().y);
        Angle angle = Angle::fromDeg(wire.getCurve());
        footprint->getPolygons().append(std::make_shared<Polygon>(Uuid::createRandom(),
            layerName, lineWidth, fill, isGrabArea, Path::line(startpos, endpos, angle)));
    }

    foreach (const parseagle::Rectangle& rect, mPackage.getRectangles()) {
        QString layerName = convertBoardLayer(rect.getLayer());
        bool fill = true;
        bool isGrabArea = true;
        Length lineWidth = 0;
        Point p1 = Point::fromMm(rect.getP1().x, rect.getP1().y);
        Point p2 = Point::fromMm(rect.getP2().x, rect.getP2().y);
        footprint->getPolygons().append(std::make_shared<Polygon>(Uuid::createRandom(),
            layerName, lineWidth, fill, isGrabArea, Path::rect(p1, p2)));
    }

    foreach (const parseagle::Circle& circle, mPackage.getCircles()) {
        QString layerName = convertBoardLayer(circle.getLayer());
        Length radius = Length::fromMm(circle.getRadius());
        Point center = Point::fromMm(circle.getPosition().x, circle.getPosition().y);
        Length lineWidth = Length::fromMm(circle.getWidth());
        bool fill = (lineWidth == 0);
        bool isGrabArea = true;
        footprint->getEllipses().append(std::make_shared<Ellipse>(Uuid::createRandom(),
            layerName, lineWidth, fill, isGrabArea, center, radius, radius, Angle::deg0()));
    }

    foreach (const parseagle::Polygon& polygon, mPackage.getPolygons()) {
        QString layerName = convertBoardLayer(polygon.getLayer());
        bool fill = false;
        bool isGrabArea = true;
        Length lineWidth = Length::fromMm(polygon.getWidth());
        Path path;
        for (int i = 0; i < polygon.getVertices().count(); ++i) {
            const parseagle::Vertex vertex = polygon.getVertices().at(i);
            Point pos = Point::fromMm(vertex.getPosition().x, vertex.getPosition().y);
            Angle angle = Angle::fromDeg(vertex.getCurve());
            path.addVertex(pos, angle);
        }
        path.close();
        footprint->getPolygons().append(std::make_shared<Polygon>(Uuid::createRandom(),
            layerName, lineWidth, fill, isGrabArea, path));
    }

    foreach (const parseagle::Text& text, mPackage.getTexts()) {
        QString layerName = convertBoardLayer(text.getLayer());
        QString textStr = text.getValue();
        if (textStr.startsWith(">")) {
            textStr = "{{" + textStr.mid(1) + "}}";
        }
        Length height = Length::fromMm(text.getSize());
        Point pos = Point::fromMm(text.getPosition().x, text.getPosition().y);
        Angle rot = Angle::fromDeg(text.getRotation().getAngle());
        Alignment align(HAlign::left(), VAlign::bottom());
        footprint->getStrokeTexts().append(std::make_shared<StrokeText>(
            Uuid::createRandom(), layerName, textStr, pos, rot, height,
            Length(200000), StrokeTextSpacing(), StrokeTextSpacing(), align, false, true));
    }

    foreach (const parseagle::Hole& hole, mPackage.getHoles()) {
        Point pos = Point::fromMm(hole.getPosition().x, hole.getPosition().y);
        Length diameter = Length::fromMm(hole.getDiameter());
        footprint->getHoles().append(std::make_shared<Hole>(Uuid::createRandom(), pos, diameter));
    }

    foreach (const parseagle::ThtPad& pad, mPackage.getThtPads()) {
        Uuid uuid = mDb.getPackagePadUuid(footprint->getUuid(), pad.getName());
        QString name = pad.getName();
        package->getPads().append(std::make_shared<library::PackagePad>(uuid, name));
        Point pos = Point::fromMm(pad.getPosition().x, pad.getPosition().y);
        Length drillDiameter = Length::fromMm(pad.getDrillDiameter());
        Length outerDiameter = Length::fromMm(pad.getOuterDiameter());
        Length padDiameter = (outerDiameter > 0) ? outerDiameter : drillDiameter * 2;
        Length width = padDiameter;
        Length height = padDiameter;
        library::FootprintPad::Shape shape;
        switch (pad.getShape()) {
            case parseagle::ThtPad::Shape::Square:
                shape = library::FootprintPad::Shape::RECT;
                break;
            case parseagle::ThtPad::Shape::Octagon:
                shape = library::FootprintPad::Shape::OCTAGON;
                break;
            case parseagle::ThtPad::Shape::Round:
                shape = library::FootprintPad::Shape::ROUND;
                break;
            case parseagle::ThtPad::Shape::Long:
                shape = library::FootprintPad::Shape::ROUND;
                width = padDiameter * 2;
                break;
            default:
                throw Exception(__FILE__, __LINE__, "Unknown shape");
        }
        Angle rot = Angle::fromDeg(pad.getRotation().getAngle());
        std::shared_ptr<library::FootprintPad> fptPad(new library::FootprintPad(uuid, pos,
            rot, shape, width, height, drillDiameter, library::FootprintPad::BoardSide::THT));
        footprint->getPads().append(fptPad);
    }

    foreach (const parseagle::SmtPad& pad, mPackage.getSmtPads()) {
        Uuid uuid = mDb.getPackagePadUuid(footprint->getUuid(), pad.getName());
        QString name = pad.getName();
        package->getPads().append(std::make_shared<library::PackagePad>(uuid, name));
        QString layerName = convertBoardLayer(pad.getLayer());
        library::FootprintPad::BoardSide side;
        if (layerName == GraphicsLayer::sTopCopper) {
            side = library::FootprintPad::BoardSide::TOP;
        } else if (layerName == GraphicsLayer::sBotCopper) {
            side = library::FootprintPad::BoardSide::BOTTOM;
        } else {
            throw Exception(__FILE__, __LINE__, QString("Invalid pad layer: %1").arg(layerName));
        }
        Point pos = Point::fromMm(pad.getPosition().x, pad.getPosition().y);
        Angle rot = Angle::fromDeg(pad.getRotation().getAngle());
        Length width = Length::fromMm(pad.getWidth());
        Length height = Length::fromMm(pad.getHeight());
        std::shared_ptr<library::FootprintPad> fptPad(new library::FootprintPad(uuid, pos,
            rot, library::FootprintPad::Shape::RECT, width, height, Length(0), side));
        footprint->getPads().append(fptPad);
    }

    package->getFootprints().append(footprint);

    return package;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

QString PackageConverter::createDescription() const noexcept
{
    QString desc = mPackage.getDescription() + "\n\n";
    desc += "This package was automatically imported from Eagle.\n";
    desc += "Library: " % mDb.getCurrentLibraryFilePath().getFilename() % "\n";
    desc += "Package: " % mPackage.getName() % "\n";
    desc += "NOTE: Please remove this text after manual rework!";
    return desc.trimmed();
}

QString PackageConverter::convertBoardLayer(int eagleLayerId)
{
    switch (eagleLayerId)
    {
        case 1:  return GraphicsLayer::sTopCopper;
        case 16: return GraphicsLayer::sBotCopper;
        case 20: return GraphicsLayer::sBoardOutlines;
        case 21: return GraphicsLayer::sTopPlacement;
        case 22: return GraphicsLayer::sBotPlacement;
        case 25: return GraphicsLayer::sTopNames;
        case 27: return GraphicsLayer::sTopValues;
        case 29: return GraphicsLayer::sTopStopMask;
        case 31: return GraphicsLayer::sTopSolderPaste;
        case 35: return GraphicsLayer::sTopGlue;
        case 39: return GraphicsLayer::sTopCourtyard;
        //case 41: return Layer::sTopCopperRestrict;
        //case 42: return Layer::sBotCopperRestrict;
        //case 43: return Layer::sViaRestrict;
        case 46: return GraphicsLayer::sBoardMillingPth;
        case 48: return GraphicsLayer::sBoardDocumentation;
        case 49: return GraphicsLayer::sBoardDocumentation; // reference
        case 51: return GraphicsLayer::sTopDocumentation;
        case 52: return GraphicsLayer::sBotDocumentation;
        default: throw Exception(__FILE__, __LINE__, QString("Invalid board layer: %1").arg(eagleLayerId));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace eagleimport
} // namespace librepcb
