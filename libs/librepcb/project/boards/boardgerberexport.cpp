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
#include "boardgerberexport.h"
#include <librepcb/common/cam/gerbergenerator.h>
#include <librepcb/common/cam/excellongenerator.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/boarddesignrules.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/attributes/attributesubstitutor.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintpad.h>
#include "../metadata/projectmetadata.h"
#include "../project.h"
#include "board.h"
#include "boardlayerstack.h"
#include "boardfabricationoutputsettings.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_via.h"
#include "items/bi_netsegment.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_hole.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardGerberExport::BoardGerberExport(const Board& board) noexcept :
    mProject(board.getProject()), mBoard(board), mCurrentInnerCopperLayer(0)
{
}

BoardGerberExport::~BoardGerberExport() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

FilePath BoardGerberExport::getOutputDirectory() const noexcept
{
    return getOutputFilePath("dummy").getParentDir(); // use dummy suffix
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardGerberExport::exportAllLayers() const
{
    if (mBoard.getFabricationOutputSettings().getMergeDrillFiles()) {
        exportDrills();
    } else {
        exportDrillsNpth();
        exportDrillsPth();
    }
    exportLayerBoardOutlines();
    exportLayerTopCopper();
    exportLayerInnerCopper();
    exportLayerBottomCopper();
    exportLayerTopSolderMask();
    exportLayerBottomSolderMask();
    exportLayerTopSilkscreen();
    exportLayerBottomSilkscreen();
    if (mBoard.getFabricationOutputSettings().getEnableSolderPasteTop()) {
        exportLayerTopSolderPaste();
    }
    if (mBoard.getFabricationOutputSettings().getEnableSolderPasteBot()) {
        exportLayerBottomSolderPaste();
    }
}

/*****************************************************************************************
 *  Inherited from AttributeProvider
 ****************************************************************************************/

QString BoardGerberExport::getBuiltInAttributeValue(const QString& key) const noexcept
{
    if ((key == QLatin1String("CU_LAYER")) && (mCurrentInnerCopperLayer > 0)) {
        return QString::number(mCurrentInnerCopperLayer);
    } else {
        return QString();
    }
}

QVector<const AttributeProvider*> BoardGerberExport::getAttributeProviderParents() const noexcept
{
    return QVector<const AttributeProvider*>{&mBoard};
}


/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardGerberExport::exportDrills() const
{
    ExcellonGenerator gen;
    drawPthDrills(gen);
    drawNpthDrills(gen);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixDrills()));
}

void BoardGerberExport::exportDrillsNpth() const
{
    ExcellonGenerator gen;
    int count = drawNpthDrills(gen);
    if (count > 0) {
        // Some PCB manufacturers don't like to have separate drill files for PTH and NPTH.
        // As many boards don't have non-plated holes anyway, we create this file only if
        // it's really needed. Maybe this avoids unnecessary issues with manufacturers...
        gen.generate();
        gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixDrillsNpth()));
    }
}

void BoardGerberExport::exportDrillsPth() const
{
    ExcellonGenerator gen;
    drawPthDrills(gen);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixDrillsPth()));
}

void BoardGerberExport::exportLayerBoardOutlines() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sBoardOutlines);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixOutlines()));
}

void BoardGerberExport::exportLayerTopCopper() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sTopCopper);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixCopperTop()));
}

void BoardGerberExport::exportLayerBottomCopper() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sBotCopper);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixCopperBot()));
}

void BoardGerberExport::exportLayerInnerCopper() const
{
    for (int i = 1; i <= mBoard.getLayerStack().getInnerLayerCount(); ++i) {
        mCurrentInnerCopperLayer = i; // used for attribute provider
        GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                            mBoard.getUuid(), mProject.getMetadata().getVersion());
        drawLayer(gen, GraphicsLayer::getInnerLayerName(i));
        gen.generate();
        gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixCopperInner()));
    }
    mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sTopStopMask);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSolderMaskTop()));
}

void BoardGerberExport::exportLayerBottomSolderMask() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sBotStopMask);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSolderMaskBot()));
}

void BoardGerberExport::exportLayerTopSilkscreen() const
{
    QStringList layers = mBoard.getFabricationOutputSettings().getSilkscreenLayersTop();
    if (layers.count() > 0) { // don't create silkscreen file if no layers selected
        GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                            mBoard.getUuid(), mProject.getMetadata().getVersion());
        foreach (const QString& layer, layers) {
            drawLayer(gen, layer);
        }
        gen.setLayerPolarity(GerberGenerator::LayerPolarity::Negative);
        drawLayer(gen, GraphicsLayer::sTopStopMask);
        gen.generate();
        gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSilkscreenTop()));
    }
}

void BoardGerberExport::exportLayerBottomSilkscreen() const
{
    QStringList layers = mBoard.getFabricationOutputSettings().getSilkscreenLayersBot();
    if (layers.count() > 0) { // don't create silkscreen file if no layers selected
        GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                            mBoard.getUuid(), mProject.getMetadata().getVersion());
        foreach (const QString& layer, layers) {
            drawLayer(gen, layer);
        }
        gen.setLayerPolarity(GerberGenerator::LayerPolarity::Negative);
        drawLayer(gen, GraphicsLayer::sBotStopMask);
        gen.generate();
        gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSilkscreenBot()));
    }
}

void BoardGerberExport::exportLayerTopSolderPaste() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sTopSolderPaste);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSolderPasteTop()));
}

void BoardGerberExport::exportLayerBottomSolderPaste() const
{
    GerberGenerator gen(mProject.getMetadata().getName() % " - " % mBoard.getName(),
                        mBoard.getUuid(), mProject.getMetadata().getVersion());
    drawLayer(gen, GraphicsLayer::sBotSolderPaste);
    gen.generate();
    gen.saveToFile(getOutputFilePath(mBoard.getFabricationOutputSettings().getSuffixSolderPasteBot()));
}

int BoardGerberExport::drawNpthDrills(ExcellonGenerator& gen) const
{
    int count = 0;

    // footprint holes
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
        const BI_Footprint& footprint = device->getFootprint();
        for (const Hole& hole : footprint.getLibFootprint().getHoles()) {
            gen.drill(footprint.mapToScene(hole.getPosition()), hole.getDiameter());
            ++count;
        }
    }

    // board holes
    foreach (const BI_Hole* hole, mBoard.getHoles()) {
        gen.drill(hole->getHole().getPosition(), hole->getHole().getDiameter());
        ++count;
    }

    return count;
}

int BoardGerberExport::drawPthDrills(ExcellonGenerator& gen) const
{
    int count = 0;

    // footprint pads
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
        const BI_Footprint& footprint = device->getFootprint();
        foreach (const BI_FootprintPad* pad, footprint.getPads()) {
            const library::FootprintPad& libPad = pad->getLibPad();
            if (libPad.getBoardSide() == library::FootprintPad::BoardSide::THT) {
                gen.drill(pad->getPosition(), libPad.getDrillDiameter());
                ++count;
            }
        }
    }

    // vias
    foreach (const BI_NetSegment* netsegment, sortedByUuid(mBoard.getNetSegments())) {
        foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) {
            gen.drill(via->getPosition(), via->getDrillDiameter());
            ++count;
        }
    }

    return count;
}

void BoardGerberExport::drawLayer(GerberGenerator& gen, const QString& layerName) const
{
    // draw footprints incl. pads
    foreach (const BI_Device* device, mBoard.getDeviceInstances()) { Q_ASSERT(device);
        drawFootprint(gen, device->getFootprint(), layerName);
    }

    // draw vias
    foreach (const BI_NetSegment* netsegment, sortedByUuid(mBoard.getNetSegments())) { Q_ASSERT(netsegment);
        foreach (const BI_Via* via, sortedByUuid(netsegment->getVias())) { Q_ASSERT(via);
            drawVia(gen, *via, layerName);
        }
    }

    // draw traces
    foreach (const BI_NetSegment* netsegment, sortedByUuid(mBoard.getNetSegments())) { Q_ASSERT(netsegment);
        foreach (const BI_NetLine* netline, sortedByUuid(netsegment->getNetLines())) { Q_ASSERT(netline);
            if (netline->getLayer().getName() == layerName) {
                gen.drawLine(netline->getStartPoint().getPosition(),
                             netline->getEndPoint().getPosition(),
                             netline->getWidth());
            }
        }
    }

    // draw planes
    foreach (const BI_Plane* plane, sortedByUuid(mBoard.getPlanes())) { Q_ASSERT(plane);
        if (plane->getLayerName() == layerName) {
            foreach (const Path& fragment, plane->getFragments()) {
                gen.drawPathArea(fragment);
            }
        }
    }

    // draw polygons
    foreach (const BI_Polygon* polygon, sortedByUuid(mBoard.getPolygons())) {
        Q_ASSERT(polygon);
        if (layerName == polygon->getPolygon().getLayerName()) {
            Length lineWidth = calcWidthOfLayer(polygon->getPolygon().getLineWidth(), layerName);
            gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth);
        }
    }

    // draw stroke texts
    foreach (const BI_StrokeText* text, sortedByUuid(mBoard.getStrokeTexts())) { Q_ASSERT(text);
        if (layerName == text->getText().getLayerName()) {
            Length lineWidth = calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
            foreach (Path path, text->getText().getPaths()) {
                path.rotate(text->getText().getRotation());
                if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
                path.translate(text->getText().getPosition());
                gen.drawPathOutline(path, lineWidth);
            }
        }
    }
}

void BoardGerberExport::drawVia(GerberGenerator& gen, const BI_Via& via, const QString& layerName) const
{
    bool drawCopper = via.isOnLayer(layerName);
    bool drawStopMask = (layerName == GraphicsLayer::sTopStopMask || layerName == GraphicsLayer::sBotStopMask)
                        && mBoard.getDesignRules().doesViaRequireStopMask(via.getDrillDiameter());
    if (drawCopper || drawStopMask) {
        Length outerDiameter = via.getSize();
        if (drawStopMask) {
            outerDiameter += mBoard.getDesignRules().calcStopMaskClearance(via.getSize()) * 2;
        }
        switch (via.getShape())
        {
            case BI_Via::Shape::Round: {
                gen.flashCircle(via.getPosition(), outerDiameter, Length(0));
                break;
            }
            case BI_Via::Shape::Square: {
                gen.flashRect(via.getPosition(), outerDiameter, outerDiameter,
                              Angle::deg0(), Length(0));
                break;
            }
            case BI_Via::Shape::Octagon: {
                gen.flashRegularPolygon(via.getPosition(), outerDiameter, 8,
                                        Angle::deg0(), Length(0));
                break;
            }
            default: {
                throw LogicError(__FILE__, __LINE__);
            }
        }
    }
}

void BoardGerberExport::drawFootprint(GerberGenerator& gen, const BI_Footprint& footprint, const QString& layerName) const
{
    // draw pads
    foreach (const BI_FootprintPad* pad, footprint.getPads()) {
        drawFootprintPad(gen, *pad, layerName);
    }

    // draw polygons
    for (const Polygon& polygon : footprint.getLibFootprint().getPolygons().sortedByUuid()) {
        QString layer = footprint.getIsMirrored() ? GraphicsLayer::getMirroredLayerName(layerName) : layerName;
        if (layer == polygon.getLayerName()) {
            Path path = polygon.getPath();
            path.rotate(footprint.getRotation());
            if (footprint.getIsMirrored()) path.mirror(Qt::Horizontal);
            path.translate(footprint.getPosition());
            gen.drawPathOutline(path, calcWidthOfLayer(polygon.getLineWidth(), layer));
            if (polygon.isFilled()) {
                gen.drawPathArea(path);
            }
        }
    }

    // draw circles
    for (const Circle& circle : footprint.getLibFootprint().getCircles().sortedByUuid()) {
        QString layer = footprint.getIsMirrored() ? GraphicsLayer::getMirroredLayerName(layerName) : layerName;
        if (layer == circle.getLayerName()) {
            Circle e = circle;
            e.rotate(footprint.getRotation());
            if (footprint.getIsMirrored()) e.mirror(Qt::Horizontal);
            e.translate(footprint.getPosition());
            e.setLineWidth(calcWidthOfLayer(e.getLineWidth(), layer));
            gen.drawCircleOutline(e);
            if (e.isFilled()) {
                gen.drawCircleArea(e);
            }
        }
    }

    // draw stroke texts (from footprint instance, *NOT* from library footprint!)
    foreach (const BI_StrokeText* text, sortedByUuid(footprint.getStrokeTexts())) {
        if (layerName == text->getText().getLayerName()) {
            Length lineWidth = calcWidthOfLayer(text->getText().getStrokeWidth(), layerName);
            foreach (Path path, text->getText().getPaths()) {
                path.rotate(text->getText().getRotation());
                if (text->getText().getMirrored()) path.mirror(Qt::Horizontal);
                path.translate(text->getPosition());
                gen.drawPathOutline(path, lineWidth);
            }
        }
    }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator& gen, const BI_FootprintPad& pad, const QString& layerName) const
{
    bool isSmt = pad.getLibPad().getBoardSide() != library::FootprintPad::BoardSide::THT;
    bool isOnCopperLayer = pad.isOnLayer(layerName);
    bool isOnSolderMaskTop = pad.isOnLayer(GraphicsLayer::sTopCopper) && (layerName == GraphicsLayer::sTopStopMask);
    bool isOnSolderMaskBottom = pad.isOnLayer(GraphicsLayer::sBotCopper) && (layerName == GraphicsLayer::sBotStopMask);
    bool isOnSolderPasteTop = isSmt && pad.isOnLayer(GraphicsLayer::sTopCopper) && (layerName == GraphicsLayer::sTopSolderPaste);
    bool isOnSolderPasteBottom = isSmt && pad.isOnLayer(GraphicsLayer::sBotCopper) && (layerName == GraphicsLayer::sBotSolderPaste);
    if (!isOnCopperLayer && !isOnSolderMaskTop && !isOnSolderMaskBottom && !isOnSolderPasteTop && !isOnSolderPasteBottom) {
        return;
    }

    Angle rot = pad.getIsMirrored() ? -pad.getRotation() : pad.getRotation();
    const library::FootprintPad& libPad = pad.getLibPad();
    Length width = libPad.getWidth();
    Length height = libPad.getHeight();
    if (isOnSolderMaskTop || isOnSolderMaskBottom) {
        Length size = qMin(width, height);
        Length clearance = mBoard.getDesignRules().calcStopMaskClearance(size);
        width += clearance*2;
        height += clearance*2;
    } else if (isOnSolderPasteTop || isOnSolderPasteBottom) {
        Length size = qMin(width, height);
        Length clearance = -mBoard.getDesignRules().calcCreamMaskClearance(size);
        width += clearance*2;
        height += clearance*2;
    }

    if ((width <= 0) || (height <= 0)) {
        qWarning() << "Pad with zero size ignored in gerber export:" << pad.getLibPadUuid();
        return;
    }

    switch (libPad.getShape())
    {
        case library::FootprintPad::Shape::ROUND: {
            if (width == height) {
                gen.flashCircle(pad.getPosition(), width, Length(0));
            } else {
                gen.flashObround(pad.getPosition(), width, height, rot, Length(0));
            }
            break;
        }
        case library::FootprintPad::Shape::RECT: {
            gen.flashRect(pad.getPosition(), width, height, rot, Length(0));
            break;
        }
        case library::FootprintPad::Shape::OCTAGON: {
            if (width != height) {
                throw LogicError(__FILE__, __LINE__,
                    tr("Sorry, non-square octagons are not yet supported."));
            }
            gen.flashRegularPolygon(pad.getPosition(), width, 8, rot, Length(0));
            break;
        }
        default: {
            throw LogicError(__FILE__, __LINE__);
        }
    }
}

FilePath BoardGerberExport::getOutputFilePath(const QString& suffix) const noexcept
{
    QString path = mBoard.getFabricationOutputSettings().getOutputBasePath() + suffix;
    path = AttributeSubstitutor::substitute(path, this, [&](const QString& str){
        return FilePath::cleanFileName(str, FilePath::ReplaceSpaces | FilePath::KeepCase);
    });

    if (QDir::isAbsolutePath(path)) {
        return FilePath(path);
    } else {
        return mBoard.getProject().getPath().getPathTo(path);
    }
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Length BoardGerberExport::calcWidthOfLayer(const Length& width, const QString& name) noexcept
{
    if ((name == GraphicsLayer::sBoardOutlines) && (width < Length(1000))) {
        return Length(1000); // outlines should have a minimum width of 1um
    } else {
        return width;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
