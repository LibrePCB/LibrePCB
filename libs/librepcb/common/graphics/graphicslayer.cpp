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
    mName(other.mName),
    mNameTr(other.mNameTr),
    mColor(other.mColor),
    mColorHighlighted(other.mColorHighlighted),
    mIsVisible(other.mIsVisible),
    mIsEnabled(other.mIsEnabled) {
}

GraphicsLayer::GraphicsLayer(const QString& name) noexcept
  : QObject(nullptr), mName(name), mIsEnabled(true) {
  getDefaultValues(mName, mNameTr, mColor, mColorHighlighted, mIsVisible);
}

GraphicsLayer::~GraphicsLayer() noexcept {
  foreach (IF_GraphicsLayerObserver* object, mObservers) {
    object->layerDestroyed(*this);
  }
  Q_ASSERT(mObservers.isEmpty());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void GraphicsLayer::setColor(const QColor& color) noexcept {
  if (color != mColor) {
    mColor = color;
    foreach (IF_GraphicsLayerObserver* object, mObservers) {
      object->layerColorChanged(*this, mColor);
    }
    emit attributesChanged();
  }
}

void GraphicsLayer::setColorHighlighted(const QColor& color) noexcept {
  if (color != mColorHighlighted) {
    mColorHighlighted = color;
    foreach (IF_GraphicsLayerObserver* object, mObservers) {
      object->layerHighlightColorChanged(*this, mColorHighlighted);
    }
    emit attributesChanged();
  }
}

void GraphicsLayer::setVisible(bool visible) noexcept {
  if (visible != mIsVisible) {
    mIsVisible = visible;
    foreach (IF_GraphicsLayerObserver* object, mObservers) {
      object->layerVisibleChanged(*this, mIsVisible);
    }
    emit attributesChanged();
  }
}

void GraphicsLayer::setEnabled(bool enable) noexcept {
  if (enable != mIsEnabled) {
    mIsEnabled = enable;
    foreach (IF_GraphicsLayerObserver* object, mObservers) {
      object->layerEnabledChanged(*this, mIsEnabled);
    }
    emit attributesChanged();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsLayer::registerObserver(IF_GraphicsLayerObserver& object) const
    noexcept {
  mObservers.insert(&object);
}

void GraphicsLayer::unregisterObserver(IF_GraphicsLayerObserver& object) const
    noexcept {
  mObservers.remove(&object);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

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
  bool ok     = false;
  int  result = number.toInt(&ok);
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

const QStringList&
GraphicsLayer::getSchematicGeometryElementLayerNames() noexcept {
  static QStringList names = {
      sSymbolOutlines,    sSymbolHiddenGrabAreas, sSymbolNames,
      sSymbolValues,      sSchematicSheetFrames,  sSchematicDocumentation,
      sSchematicComments, sSchematicGuide,
  };
  return names;
}

const QStringList& GraphicsLayer::getBoardGeometryElementLayerNames() noexcept {
  static QStringList names = {
      sBoardSheetFrames, sBoardOutlines,
      sBoardMillingPth,  sBoardMeasures,
      sBoardAlignment,   sBoardDocumentation,
      sBoardComments,    sBoardGuide,
      sTopPlacement,     sTopHiddenGrabAreas,
      sTopDocumentation, sTopNames,
      sTopValues,        sTopCopper,
      sTopCourtyard,     sTopGlue,
      sTopSolderPaste,   sTopStopMask,
      sBotPlacement,     sBotHiddenGrabAreas,
      sBotDocumentation, sBotNames,
      sBotValues,        sBotCopper,
      sBotCourtyard,     sBotGlue,
      sBotSolderPaste,   sBotStopMask,
  };
  return names;
}

void GraphicsLayer::getDefaultValues(const QString& name, QString& nameTr,
                                     QColor& color, QColor& colorHl,
                                     bool& visible) noexcept {
  typedef struct {
    QString nameTr;
    QColor  color;
    QColor  colorHl;
    bool    visible;
  } Item;
  static QHash<QString, Item> h;
  if (h.isEmpty()) {
    // clang-format off
    // schematic
    h.insert(sSchematicReferences,      {tr("References"),            QColor(0, 0, 0, 50),        QColor(0, 0, 0, 80),        true});
    h.insert(sSchematicSheetFrames,     {tr("Sheet Frames"),          Qt::black,                  Qt::darkGray,               true});
    h.insert(sSchematicNetLines,        {tr("Netlines"),              Qt::darkGreen,              Qt::green,                  true});
    h.insert(sSchematicNetLabels,       {tr("Netlabels"),             Qt::darkGreen,              Qt::green,                  true});
    h.insert(sSchematicNetLabelAnchors, {tr("Netlabel Anchors"),      Qt::darkGray,               Qt::gray,                   true});
    h.insert(sSchematicDocumentation,   {tr("Documentation"),         Qt::darkGray,               Qt::gray,                   true});
    h.insert(sSchematicComments,        {tr("Comments"),              Qt::darkBlue,               Qt::blue,                   true});
    h.insert(sSchematicGuide,           {tr("Guide"),                 Qt::darkYellow,             Qt::yellow,                 true});
    // symbol
    h.insert(sSymbolOutlines,           {tr("Outlines"),              Qt::darkRed,                Qt::red,                    true});
    h.insert(sSymbolGrabAreas,          {tr("Grab Areas"),            QColor(255, 255, 0, 30),    QColor(255, 255, 0, 50),    true});
    h.insert(sSymbolHiddenGrabAreas,    {tr("Hidden Grab Areas"),     QColor(0, 0, 255, 30),      QColor(0, 0, 255, 50),      false});
    h.insert(sSymbolNames,              {tr("Names"),                 QColor(32, 32, 32, 255),    Qt::darkGray,               true});
    h.insert(sSymbolValues,             {tr("Values"),                QColor(80, 80, 80, 255),    Qt::gray,                   true});
    h.insert(sSymbolPinCirclesOpt,      {tr("Optional Pins"),         QColor(0, 255, 0, 255),     QColor(0, 255, 0, 127),     true});
    h.insert(sSymbolPinCirclesReq,      {tr("Required Pins"),         QColor(255, 0, 0, 255),     QColor(255, 0, 0, 127),     true});
    h.insert(sSymbolPinNames,           {tr("Pin Names"),             QColor(64, 64, 64, 255),    Qt::gray,                   true});
    h.insert(sSymbolPinNumbers,         {tr("Pin Numbers"),           QColor(64, 64, 64, 255),    Qt::gray,                   true});
    // board asymmetric
    h.insert(sBoardSheetFrames,         {tr("Sheet Frames"),          QColor("#96E0E0E0"),        QColor("#FFFFFFFF"),        true});
    h.insert(sBoardOutlines,            {tr("Board Outlines"),        QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"),        true});
    h.insert(sBoardMillingPth,          {tr("Milling (PTH)"),         QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"),        true});
    h.insert(sBoardDrillsNpth,          {tr("Drills (NPTH)"),         QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"),        true});
    h.insert(sBoardPadsTht,             {tr("Pads"),                  QColor("#966DB515"),        QColor("#B44EFC14"),        true});
    h.insert(sBoardViasTht,             {tr("Vias"),                  QColor("#966DB515"),        QColor("#B44EFC14"),        true});
    h.insert(sBoardAirWires,            {tr("Air Wires"),             Qt::yellow,                 Qt::yellow,                 true});
    h.insert(sBoardMeasures,            {tr("Measures"),              QColor("#FF808000"),        QColor("#FFA3B200"),        true});
    h.insert(sBoardAlignment,           {tr("Alignment"),             QColor("#B4E59500"),        QColor("#DCFFBF00"),        true});
    h.insert(sBoardDocumentation,       {tr("Documentation"),         QColor("#B4E59500"),        QColor("#DCFFBF00"),        true});
    h.insert(sBoardComments,            {tr("Comments"),              QColor("#B4E59500"),        QColor("#DCFFBF00"),        true});
    h.insert(sBoardGuide,               {tr("Guide"),                 QColor("#FF808000"),        QColor("#FFA3B200"),        true});
    // board symmetric
    h.insert(sTopPlacement,             {tr("Top Placement"),         QColor("#BBFFFFFF"),        QColor("#FFFFFFFF"),        true});
    h.insert(sBotPlacement,             {tr("Bot Placement"),         QColor("#BBFFFFFF"),        QColor("#FFFFFFFF"),        true});
    h.insert(sTopDocumentation,         {tr("Top Documentation"),     QColor("#96E0E0E0"),        QColor("#DCE0E0E0"),        true});
    h.insert(sBotDocumentation,         {tr("Bot Documentation"),     QColor("#96E0E0E0"),        QColor("#DCE0E0E0"),        true});
    h.insert(sTopGrabAreas,             {tr("Top Grab Areas"),        QColor("#14FFFFFF"),        QColor("#32FFFFFF"),        false});
    h.insert(sBotGrabAreas,             {tr("Bot Grab Areas"),        QColor("#14FFFFFF"),        QColor("#32FFFFFF"),        false});
    h.insert(sTopHiddenGrabAreas,       {tr("Top Hidden Grab Areas"), QColor("#28FFFFFF"),        QColor("#46FFFFFF"),        false});
    h.insert(sBotHiddenGrabAreas,       {tr("Bot Hidden Grab Areas"), QColor("#28FFFFFF"),        QColor("#46FFFFFF"),        false});
    h.insert(sTopReferences,            {tr("Top References"),        QColor("#64FFFFFF"),        QColor("#B4FFFFFF"),        true});
    h.insert(sBotReferences,            {tr("Bot References"),        QColor("#64FFFFFF"),        QColor("#B4FFFFFF"),        true});
    h.insert(sTopNames,                 {tr("Top Names"),             QColor("#96EDFFD8"),        QColor("#DCE0E0E0"),        true});
    h.insert(sBotNames,                 {tr("Bot Names"),             QColor("#96EDFFD8"),        QColor("#DCE0E0E0"),        true});
    h.insert(sTopValues,                {tr("Top Values"),            QColor("#96D8F2FF"),        QColor("#DCE0E0E0"),        true});
    h.insert(sBotValues,                {tr("Bot Values"),            QColor("#96D8F2FF"),        QColor("#DCE0E0E0"),        true});
    h.insert(sTopCourtyard,             {tr("Top Courtyard"),         QColor("#4600FFFF"),        QColor("#5A00FFFF"),        false});
    h.insert(sBotCourtyard,             {tr("Bot Courtyard"),         QColor("#4600FFFF"),        QColor("#5A00FFFF"),        false});
    h.insert(sTopStopMask,              {tr("Top Stop Mask"),         QColor("#30FFFFFF"),        QColor("#60FFFFFF"),        false});
    h.insert(sBotStopMask,              {tr("Bot Stop Mask"),         QColor("#30FFFFFF"),        QColor("#60FFFFFF"),        false});
    h.insert(sTopSolderPaste,           {tr("Top Solder Paste"),      QColor("#20E0E0E0"),        QColor("#40E0E0E0"),        false});
    h.insert(sBotSolderPaste,           {tr("Bot Solder Paste"),      QColor("#20E0E0E0"),        QColor("#40E0E0E0"),        false});
    h.insert(sTopFinish,                {tr("Top Finish"),            QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130),     true});
    h.insert(sBotFinish,                {tr("Bot Finish"),            QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130),     true});
    h.insert(sTopGlue,                  {tr("Top Glue"),              QColor("#64E0E0E0"),        QColor("#78E0E0E0"),        false});
    h.insert(sBotGlue,                  {tr("Bot Glue"),              QColor("#64E0E0E0"),        QColor("#78E0E0E0"),        false});
    // board copper
    h.insert(sTopCopper,                {tr("Top Copper"),            QColor("#96CC0802"),        QColor("#C0FF0800"),        true});
    h.insert(sBotCopper,                {tr("Bot Copper"),            QColor("#964578CC"),        QColor("#C00A66FC"),        true});
    // clang-format on
    for (int i = 1; i <= getInnerLayerCount(); ++i) {
      QString nameTr = QString(tr("Inner Copper %1")).arg(i);
      QColor  color;
      QColor  hlColor;
      switch ((i - 1) % 6) {
        case 0:
          color   = QColor("#96CC57FF");
          hlColor = QColor("#C0DA84FF");
          break;
        case 1:
          color   = QColor("#96E2A1FF");
          hlColor = QColor("#C0E9BAFF");
          break;
        case 2:
          color   = QColor("#96EE5C9B");
          hlColor = QColor("#C0FF4C99");
          break;
        case 3:
          color   = QColor("#96E50063");
          hlColor = QColor("#C0E50063");
          break;
        case 4:
          color   = QColor("#96A70049");
          hlColor = QColor("#C0CC0058");
          break;
        case 5:
          color   = QColor("#967B20A3");
          hlColor = QColor("#C09739BF");
          break;
        default:
          qWarning() << "Invalid remainder!";
          color   = QColor("#FFFF00FF");
          hlColor = QColor("#FFFF00FF");
      }
      h.insert(getInnerLayerName(i), {nameTr, color, hlColor, true});
    }
  }

  Item item = h.value(name, {name, Qt::darkRed, Qt::red, false});
  nameTr    = item.nameTr;
  color     = item.color;
  colorHl   = item.colorHl;
  visible   = item.visible;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
