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
#include <QtWidgets>
#include "graphicslayer.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

GraphicsLayer::GraphicsLayer(const GraphicsLayer& other) noexcept :
    QObject(nullptr), mName(other.mName), mNameTr(other.mNameTr), mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted), mIsVisible(other.mIsVisible),
    mIsEnabled(other.mIsEnabled)
{
}

GraphicsLayer::GraphicsLayer(const QString& name) noexcept :
    QObject(nullptr), mName(name), mIsEnabled(true)
{
    getDefaultValues(mName, mNameTr, mColor, mColorHighlighted, mIsVisible);
}

GraphicsLayer::~GraphicsLayer() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void GraphicsLayer::setColor(const QColor& color) noexcept
{
    if (color != mColor) {
        mColor = color;
        foreach (IF_GraphicsLayerObserver* object, mObservers) {
            object->layerColorChanged(*this, mColor);
        }
        emit attributesChanged();
    }
}

void GraphicsLayer::setColorHighlighted(const QColor& color) noexcept
{
    if (color != mColorHighlighted) {
        mColorHighlighted = color;
        foreach (IF_GraphicsLayerObserver* object, mObservers) {
            object->layerHighlightColorChanged(*this, mColorHighlighted);
        }
        emit attributesChanged();
    }
}

void GraphicsLayer::setVisible(bool visible) noexcept
{
    if (visible != mIsVisible) {
        mIsVisible = visible;
        foreach (IF_GraphicsLayerObserver* object, mObservers) {
            object->layerVisibleChanged(*this, mIsVisible);
        }
        emit attributesChanged();
    }
}

void GraphicsLayer::setEnabled(bool enable) noexcept
{
    if (enable != mIsEnabled) {
        mIsEnabled = enable;
        foreach (IF_GraphicsLayerObserver* object, mObservers) {
            object->layerEnabledChanged(*this, mIsEnabled);
        }
        emit attributesChanged();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void GraphicsLayer::registerObserver(IF_GraphicsLayerObserver& object) const noexcept
{
    mObservers.insert(&object);
}

void GraphicsLayer::unregisterObserver(IF_GraphicsLayerObserver& object) const noexcept
{
    mObservers.remove(&object);
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

bool GraphicsLayer::isTopLayer(const QString& name) noexcept
{
    return name.startsWith("top_");
}

bool GraphicsLayer::isBottomLayer(const QString& name) noexcept
{
    return name.startsWith("bot_");
}

bool GraphicsLayer::isInnerLayer(const QString& name) noexcept
{
    return name.startsWith("in");
}

bool GraphicsLayer::isCopperLayer(const QString& name) noexcept
{
    return name.endsWith("_cu");
}

QString GraphicsLayer::getInnerLayerName(int number) noexcept
{
    return QString("in%1_cu").arg(number);
}

int GraphicsLayer::getInnerLayerNumber(const QString& name) noexcept
{
    QString number = name;
    number.remove("in");
    number.remove("_cu");
    bool ok = false;
    int result = number.toInt(&ok);
    return ok ? result : -1;
}

QString GraphicsLayer::getMirroredLayerName(const QString& name) noexcept
{
    if (name.startsWith("top_")) {
        return QString(name).replace(0, 3, "bot");
    } else if (name.startsWith("bot_")) {
        return QString(name).replace(0, 3, "top");
    } else {
        return name; // Layer cannot be mirrored
    }
}

QString GraphicsLayer::getGrabAreaLayerName(const QString& outlineLayerName) noexcept
{
    if (outlineLayerName == sTopPlacement) {
        return sTopGrabAreas;
    } else if (outlineLayerName == sBotPlacement) {
        return sBotGrabAreas;
    } else if (outlineLayerName == sSymbolOutlines) {
        return sSymbolGrabAreas;
    } else {
        return QString();
    }
}

const QStringList& GraphicsLayer::getSchematicGeometryElementLayerNames() noexcept
{
    static QStringList names = {
        sSymbolOutlines,
        sSymbolHiddenGrabAreas,
        sSymbolNames,
        sSymbolValues,
        sSchematicSheetFrames,
        sSchematicDocumentation,
        sSchematicComments,
        sSchematicGuide,
    };
    return names;
}

const QStringList& GraphicsLayer::getBoardGeometryElementLayerNames() noexcept
{
    static QStringList names = {
        sBoardSheetFrames,
        sBoardOutlines,
        sBoardMillingPth,
        sBoardMeasures,
        sBoardAlignment,
        sBoardDocumentation,
        sBoardComments,
        sBoardcGuide,
        sTopPlacement,
        sTopHiddenGrabAreas,
        sTopDocumentation,
        sTopNames,
        sTopValues,
        sTopCopper,
        sTopCourtyard,
        sTopGlue,
        sTopSolderPaste,
        sTopStopMask,
        sBotPlacement,
        sBotHiddenGrabAreas,
        sBotDocumentation,
        sBotNames,
        sBotValues,
        sBotCopper,
        sBotCourtyard,
        sBotGlue,
        sBotSolderPaste,
        sBotStopMask,
    };
    return names;
}

void GraphicsLayer::getDefaultValues(const QString& name, QString& nameTr, QColor& color,
                                     QColor& colorHl, bool& visible) noexcept
{
    typedef struct {QString nameTr; QColor color; QColor colorHl; bool visible;} Item;
    static QHash<QString, Item> h;
    if (h.isEmpty()) {
        // schematic
        h.insert(sSchematicReferences,      {tr("References"),                  QColor(0, 0, 0, 50),        QColor(0, 0, 0, 80),        true});
        h.insert(sSchematicSheetFrames,     {tr("Sheet Frames"),                Qt::black,                  Qt::darkGray,               true});
        h.insert(sSchematicNetLines,        {tr("Netlines"),                    Qt::darkGreen,              Qt::green,                  true});
        h.insert(sSchematicNetLabels,       {tr("Netlabels"),                   Qt::darkGreen,              Qt::green,                  true});
        h.insert(sSchematicNetLabelAnchors, {tr("Netlabel Anchors"),            Qt::darkGray,               Qt::gray,                   true});
        h.insert(sSchematicDocumentation,   {tr("Documentation"),               Qt::darkGray,               Qt::gray,                   true});
        h.insert(sSchematicComments,        {tr("Comments"),                    Qt::darkBlue,               Qt::blue,                   true});
        h.insert(sSchematicGuide,           {tr("Guide"),                       Qt::darkYellow,             Qt::yellow,                 true});
        // symbol
        h.insert(sSymbolOutlines,           {tr("Outlines"),                    Qt::darkRed,                Qt::red,                    true});
        h.insert(sSymbolGrabAreas,          {tr("Grab Areas"),                  QColor(255, 255, 0, 30),    QColor(255, 255, 0, 50),    true});
        h.insert(sSymbolHiddenGrabAreas,    {tr("Hidden Grab Areas"),           QColor(0, 0, 255, 30),      QColor(0, 0, 255, 50),      false});
        h.insert(sSymbolNames,              {tr("Names"),                       QColor(32, 32, 32, 255),    Qt::darkGray,               true});
        h.insert(sSymbolValues,             {tr("Values"),                      QColor(80, 80, 80, 255),    Qt::gray,                   true});
        h.insert(sSymbolPinCirclesOpt,      {tr("Optional Pins"),               QColor(0, 255, 0, 255),     QColor(0, 255, 0, 127),     true});
        h.insert(sSymbolPinCirclesReq,      {tr("Required Pins"),               QColor(255, 0, 0, 255),     QColor(255, 0, 0, 127),     true});
        h.insert(sSymbolPinNames,           {tr("Pin Names"),                   QColor(64, 64, 64, 255),    Qt::gray,                   true});
        h.insert(sSymbolPinNumbers,         {tr("Pin Numbers"),                 QColor(64, 64, 64, 255),    Qt::gray,                   true});
        // board asymmetric
        h.insert(sBoardSheetFrames,         {tr("Sheet Frames"),                Qt::lightGray,              Qt::white,                  true});
        h.insert(sBoardOutlines,            {tr("Board Outlines"),              QColor(255, 255, 255, 180), QColor(255, 255, 255, 220), true});
        h.insert(sBoardMillingPth,          {tr("Milling (PTH)"),               Qt::cyan,                   Qt::blue,                   true});
        h.insert(sBoardDrillsNpth,          {tr("Drills (NPTH)"),               QColor(255, 255, 255, 150), QColor(255, 255, 255, 220), true});
        h.insert(sBoardPadsTht,             {tr("Pads"),                        QColor(0, 255, 0, 150),     QColor(0, 255, 0, 220),     true});
        h.insert(sBoardViasTht,             {tr("Vias"),                        QColor(0, 255, 0, 150),     QColor(0, 255, 0, 220),     true});
        h.insert(sBoardMeasures,            {tr("Measures"),                    Qt::lightGray,              Qt::gray,                   true});
        h.insert(sBoardAlignment,           {tr("Alignment"),                   Qt::darkCyan,               Qt::cyan,                   true});
        h.insert(sBoardDocumentation,       {tr("Documentation"),               Qt::white,                  Qt::lightGray,              true});
        h.insert(sBoardComments,            {tr("Comments"),                    Qt::yellow,                 Qt::darkYellow,             true});
        h.insert(sBoardcGuide,              {tr("Guide"),                       Qt::darkYellow,             Qt::yellow,                 true});
        // board symmetric
        h.insert(sTopPlacement,             {tr("Top Placement"),               QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sBotPlacement,             {tr("Bot Placement"),               QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sTopDocumentation,         {tr("Top Documentation"),           QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sBotDocumentation,         {tr("Bot Documentation"),           QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sTopGrabAreas,             {tr("Top Grab Areas"),              QColor(255, 255, 255, 20),  QColor(255, 255, 255, 50),  false});
        h.insert(sBotGrabAreas,             {tr("Bot Grab Areas"),              QColor(255, 255, 255, 20),  QColor(255, 255, 255, 50),  false});
        h.insert(sTopHiddenGrabAreas,       {tr("Top Hidden Grab Areas"),       QColor(255, 255, 255, 40),  QColor(255, 255, 255, 70),  false});
        h.insert(sBotHiddenGrabAreas,       {tr("Bot Hidden Grab Areas"),       QColor(255, 255, 255, 40),  QColor(255, 255, 255, 70),  false});
        h.insert(sTopReferences,            {tr("Top References"),              QColor(255, 255, 255, 50),  QColor(255, 255, 255, 80),  true});
        h.insert(sBotReferences,            {tr("Bot References"),              QColor(255, 255, 255, 50),  QColor(255, 255, 255, 80),  true});
        h.insert(sTopNames,                 {tr("Top Names"),                   QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sBotNames,                 {tr("Bot Names"),                   QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sTopValues,                {tr("Top Values"),                  QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sBotValues,                {tr("Bot Values"),                  QColor(224, 224, 224, 150), QColor(224, 224, 224, 220), true});
        h.insert(sTopCourtyard,             {tr("Top Courtyard"),               QColor(255, 0, 255, 70),    QColor(255, 0, 255, 90),    false});
        h.insert(sBotCourtyard,             {tr("Bot Courtyard"),               QColor(0, 255, 255, 70),    QColor(0, 255, 255, 90),    false});
        h.insert(sTopStopMask,              {tr("Top Stop Mask"),               QColor(255, 255, 255, 100), QColor(255, 0, 0, 150),     false});
        h.insert(sBotStopMask,              {tr("Bot Stop Mask"),               QColor(255, 255, 255, 100), QColor(255, 0, 0, 150),     false});
        h.insert(sTopSolderPaste,           {tr("Top Solder Paste"),            QColor(224, 224, 224, 100), QColor(224, 224, 224, 120), false});
        h.insert(sBotSolderPaste,           {tr("Bot Solder Paste"),            QColor(224, 224, 224, 100), QColor(224, 224, 224, 120), false});
        h.insert(sTopFinish,                {tr("Top Finish"),                  QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130),     true});
        h.insert(sBotFinish,                {tr("Bot Finish"),                  QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130),     true});
        h.insert(sTopGlue,                  {tr("Top Glue"),                    QColor(224, 224, 224, 100), QColor(224, 224, 224, 120), false});
        h.insert(sBotGlue,                  {tr("Bot Glue"),                    QColor(224, 224, 224, 100), QColor(224, 224, 224, 120), false});
        // board copper
        h.insert(sTopCopper,                {tr("Top Copper"),                  QColor(255, 0, 0, 150),     QColor(255, 0, 0, 224),     true});
        h.insert(sBotCopper,                {tr("Bot Copper"),                  QColor(0, 0, 255, 150),     QColor(0, 0, 255, 224),     true});
        for (int i = 1; i <= getInnerLayerCount(); ++i) {
            QString nameTr = QString(tr("Inner Copper %1")).arg(i);
            int magicColorHue = (i * (60 + 360 / getInnerLayerCount())) % 360; // very magic formula!! :D
            QColor magicColor = QColor::fromHsv(magicColorHue, 255, 255, 150);
            h.insert(getInnerLayerName(i),  {nameTr,                            magicColor,                 magicColor.lighter(150),    true});
        }
    }

    Item item = h.value(name, {name, Qt::darkRed, Qt::red, false});
    nameTr = item.nameTr;
    color = item.color;
    colorHl = item.colorHl;
    visible = item.visible;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
