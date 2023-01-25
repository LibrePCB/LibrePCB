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
#include "graphicslayer.h"

#include "../workspace/theme.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsLayer::GraphicsLayer(const GraphicsLayer& other) noexcept
  : QObject(nullptr),
    onEdited(*this),
    mName(other.mName),
    mNameTr(other.mNameTr),
    mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted),
    mIsVisible(other.mIsVisible),
    mIsEnabled(other.mIsEnabled) {
}

GraphicsLayer::GraphicsLayer(const QString& name, const QColor& color,
                             const QColor& colorHighlighted, bool visible,
                             bool enabled) noexcept
  : QObject(nullptr),
    onEdited(*this),
    mName(name),
    mNameTr(getTranslation(mName)),
    mColor(color),
    mColorHighlighted(colorHighlighted),
    mIsVisible(visible),
    mIsEnabled(enabled) {
}

GraphicsLayer::~GraphicsLayer() noexcept {
  onEdited.notify(Event::Destroyed);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsLayer::setColor(const QColor& color) noexcept {
  if (color != mColor) {
    mColor = color;
    onEdited.notify(Event::ColorChanged);
    emit attributesChanged();
  }
}

void GraphicsLayer::setColorHighlighted(const QColor& color) noexcept {
  if (color != mColorHighlighted) {
    mColorHighlighted = color;
    onEdited.notify(Event::HighlightColorChanged);
    emit attributesChanged();
  }
}

void GraphicsLayer::setVisible(bool visible) noexcept {
  if (visible != mIsVisible) {
    mIsVisible = visible;
    onEdited.notify(Event::VisibleChanged);
    emit attributesChanged();
  }
}

void GraphicsLayer::setEnabled(bool enable) noexcept {
  if (enable != mIsEnabled) {
    mIsEnabled = enable;
    onEdited.notify(Event::EnabledChanged);
    emit attributesChanged();
  }
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

GraphicsLayer& GraphicsLayer::operator=(const GraphicsLayer& rhs) noexcept {
  mName = rhs.mName;
  mNameTr = rhs.mNameTr;
  mColor = rhs.mColor;
  mColorHighlighted = rhs.mColorHighlighted;
  mIsVisible = rhs.mIsVisible;
  mIsEnabled = rhs.mIsEnabled;
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool GraphicsLayer::isBoardLayer(const QString& name) noexcept {
  return name.startsWith("brd_") || isTopLayer(name) || isBottomLayer(name);
}

bool GraphicsLayer::isTopLayer(const QString& name) noexcept {
  return name.startsWith("top_");
}

bool GraphicsLayer::isBottomLayer(const QString& name) noexcept {
  return name.startsWith("bot_");
}

bool GraphicsLayer::isInnerLayer(const QString& name) noexcept {
  return name.startsWith("in");
}

bool GraphicsLayer::isCopperLayer(const QString& name) noexcept {
  return name.endsWith("_cu");
}

QString GraphicsLayer::getInnerLayerName(int number) noexcept {
  return QString("in%1_cu").arg(number);
}

int GraphicsLayer::getInnerLayerNumber(const QString& name) noexcept {
  QString number = name;
  number.remove("in");
  number.remove("_cu");
  bool ok = false;
  int result = number.toInt(&ok);
  return ok ? result : -1;
}

QString GraphicsLayer::getMirroredLayerName(const QString& name) noexcept {
  if (name.startsWith("top_")) {
    return QString(name).replace(0, 3, "bot");
  } else if (name.startsWith("bot_")) {
    return QString(name).replace(0, 3, "top");
  } else {
    return name;  // Layer cannot be mirrored
  }
}

QString GraphicsLayer::getGrabAreaLayerName(
    const QString& outlineLayerName) noexcept {
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

QString GraphicsLayer::getTranslation(const QString& name) noexcept {
  static QHash<QString, QString> map;
  if (map.isEmpty()) {
    map.insert(sSchematicReferences, tr("References"));
    map.insert(sSchematicSheetFrames, tr("Sheet Frames"));
    map.insert(sSchematicNetLines, tr("Netlines"));
    map.insert(sSchematicNetLabels, tr("Netlabels"));
    map.insert(sSchematicNetLabelAnchors, tr("Netlabel Anchors"));
    map.insert(sSchematicDocumentation, tr("Documentation"));
    map.insert(sSchematicComments, tr("Comments"));
    map.insert(sSchematicGuide, tr("Guide"));
    map.insert(sSymbolOutlines, tr("Outlines"));
    map.insert(sSymbolGrabAreas, tr("Grab Areas"));
    map.insert(sSymbolHiddenGrabAreas, tr("Hidden Grab Areas"));
    map.insert(sSymbolNames, tr("Names"));
    map.insert(sSymbolValues, tr("Values"));
    map.insert(sSymbolPinCirclesOpt, tr("Optional Pins"));
    map.insert(sSymbolPinCirclesReq, tr("Required Pins"));
    map.insert(sSymbolPinLines, tr("Pin Lines"));
    map.insert(sSymbolPinNames, tr("Pin Names"));
    map.insert(sSymbolPinNumbers, tr("Pin Numbers"));
    map.insert(sBoardSheetFrames, tr("Sheet Frames"));
    map.insert(sBoardOutlines, tr("Board Outlines"));
    map.insert(sBoardMillingPth, tr("Milling (PTH"));
    map.insert(sBoardDrillsNpth, tr("Drills (NPTH"));
    map.insert(sBoardPadsTht, tr("Pads"));
    map.insert(sBoardViasTht, tr("Vias"));
    map.insert(sBoardAirWires, tr("Air Wires"));
    map.insert(sBoardMeasures, tr("Measures"));
    map.insert(sBoardAlignment, tr("Alignment"));
    map.insert(sBoardDocumentation, tr("Documentation"));
    map.insert(sBoardComments, tr("Comments"));
    map.insert(sBoardGuide, tr("Guide"));
    map.insert(sTopPlacement, tr("Top Placement"));
    map.insert(sBotPlacement, tr("Bot Placement"));
    map.insert(sTopDocumentation, tr("Top Documentation"));
    map.insert(sBotDocumentation, tr("Bot Documentation"));
    map.insert(sTopGrabAreas, tr("Top Grab Areas"));
    map.insert(sBotGrabAreas, tr("Bot Grab Areas"));
    map.insert(sTopHiddenGrabAreas, tr("Top Hidden Grab Areas"));
    map.insert(sBotHiddenGrabAreas, tr("Bot Hidden Grab Areas"));
    map.insert(sTopReferences, tr("Top References"));
    map.insert(sBotReferences, tr("Bot References"));
    map.insert(sTopNames, tr("Top Names"));
    map.insert(sBotNames, tr("Bot Names"));
    map.insert(sTopValues, tr("Top Values"));
    map.insert(sBotValues, tr("Bot Values"));
    map.insert(sTopCourtyard, tr("Top Courtyard"));
    map.insert(sBotCourtyard, tr("Bot Courtyard"));
    map.insert(sTopStopMask, tr("Top Stop Mask"));
    map.insert(sBotStopMask, tr("Bot Stop Mask"));
    map.insert(sTopSolderPaste, tr("Top Solder Paste"));
    map.insert(sBotSolderPaste, tr("Bot Solder Paste"));
    map.insert(sTopFinish, tr("Top Finish"));
    map.insert(sBotFinish, tr("Bot Finish"));
    map.insert(sTopGlue, tr("Top Glue"));
    map.insert(sBotGlue, tr("Bot Glue"));
    map.insert(sTopCopper, tr("Top Copper"));
    map.insert(sBotCopper, tr("Bot Copper"));
    for (int i = 1; i <= getInnerLayerCount(); ++i) {
      map.insert(getInnerLayerName(i), tr("Inner Copper %1").arg(i));
    }
  }
  return map.value(name, "Unknown");
}

/*******************************************************************************
 *  Class IF_GraphicsLayerProvider
 ******************************************************************************/

void IF_GraphicsLayerProvider::applyTheme(const Theme& theme) noexcept {
  foreach (GraphicsLayer* layer, getAllLayers()) {
    const ThemeColor& color = theme.getColorForLayer(layer->getName());
    layer->setColor(color.getPrimaryColor());
    layer->setColorHighlighted(color.getSecondaryColor());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
