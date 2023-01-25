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
#include "theme.h"

#include "../exceptions.h"
#include "../graphics/graphicslayer.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const Theme::GridStyle& obj) {
  switch (obj) {
    case Theme::GridStyle::None:
      return SExpression::createToken("none");
    case Theme::GridStyle::Dots:
      return SExpression::createToken("dots");
    case Theme::GridStyle::Lines:
      return SExpression::createToken("lines");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline Theme::GridStyle deserialize(const SExpression& sexpr) {
  const QString str = sexpr.getValue();
  if (str == "none") {
    return Theme::GridStyle::None;
  } else if (str == "dots") {
    return Theme::GridStyle::Dots;
  } else if (str == "lines") {
    return Theme::GridStyle::Lines;
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Unknown grid style: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Theme::Theme(const Uuid& uuid, const QString& name) noexcept
  : mNodes(),
    mUuid(uuid),
    mName(name),
    mColors(),
    mSchematicGridStyle(GridStyle::Lines),
    mBoardGridStyle(GridStyle::Lines) {
  // clang-format off
  addColor(Color::sSchematicBackground,        tr("Schematic Background/Grid"),      Qt::white,                  Qt::gray);
  addColor(Color::sSchematicOverlays,          tr("Schematic Overlays"),             QColor(255, 255, 255, 120), Qt::black);
  addColor(Color::sSchematicInfoBox,           tr("Schematic Info Box"),             QColor(255, 255, 255, 130), Qt::black);
  addColor(Color::sSchematicSelection,         tr("Schematic Selection"),            QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(Color::sSchematicReferences,        tr("Schematic References"),           QColor(0, 0, 0, 50),        QColor(0, 0, 0, 80));
  addColor(Color::sSchematicFrames,            tr("Schematic Frames"),               Qt::black,                  Qt::darkGray);
  addColor(Color::sSchematicWires,             tr("Schematic Wires"),                Qt::darkGreen,              Qt::green);
  addColor(Color::sSchematicNetLabels,         tr("Schematic Net Labels"),           Qt::darkGreen,              Qt::green);
  addColor(Color::sSchematicNetLabelAnchors,   tr("Schematic Net Label Anchors"),    Qt::darkGray,               Qt::gray);
  addColor(Color::sSchematicDocumentation,     tr("Schematic Documentation"),        Qt::darkGray,               Qt::gray);
  addColor(Color::sSchematicComments,          tr("Schematic Comments"),             Qt::darkBlue,               Qt::blue);
  addColor(Color::sSchematicGuide,             tr("Schematic Guide"),                Qt::darkYellow,             Qt::yellow);
  addColor(Color::sSchematicOutlines,          tr("Schematic Outlines"),             Qt::darkRed,                Qt::red);
  addColor(Color::sSchematicGrabAreas,         tr("Schematic Grab Areas"),           QColor(255, 255, 225, 255), QColor(255, 255, 205, 255));
  addColor(Color::sSchematicHiddenGrabAreas,   tr("Schematic Hidden Grab Areas"),    QColor(0, 0, 255, 30),      QColor(0, 0, 255, 50));
  addColor(Color::sSchematicNames,             tr("Schematic Names"),                QColor(32, 32, 32, 255),    Qt::darkGray);
  addColor(Color::sSchematicValues,            tr("Schematic Values"),               QColor(80, 80, 80, 255),    Qt::gray);
  addColor(Color::sSchematicOptionalPins,      tr("Schematic Optional Pins"),        QColor(0, 255, 0, 255),     QColor(0, 255, 0, 127));
  addColor(Color::sSchematicRequiredPins,      tr("Schematic Required Pins"),        QColor(255, 0, 0, 255),     QColor(255, 0, 0, 127));
  addColor(Color::sSchematicPinLines,          tr("Schematic Pin Lines"),            Qt::darkRed,                Qt::red);
  addColor(Color::sSchematicPinNames,          tr("Schematic Pin Names"),            QColor(64, 64, 64, 255),    Qt::gray);
  addColor(Color::sSchematicPinNumbers,        tr("Schematic Pin Numbers"),          QColor(64, 64, 64, 255),    Qt::gray);
  addColor(Color::sBoardBackground,            tr("Board Background/Grid"),          Qt::black,                  Qt::gray);
  addColor(Color::sBoardOverlays,              tr("Board Overlays"),                 QColor(0, 0, 0, 120),       Qt::yellow);
  addColor(Color::sBoardInfoBox,               tr("Board Info Box"),                 QColor(0, 0, 0, 130),       Qt::yellow);
  addColor(Color::sBoardDrcMarker,             tr("Board DRC Marker"),               Qt::transparent,            QColor(255, 127, 0, 255));
  addColor(Color::sBoardSelection,             tr("Board Selection"),                QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(Color::sBoardFrames,                tr("Board Frames"),                   QColor("#96E0E0E0"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardOutlines,              tr("Board Outlines"),                 QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardMilling,               tr("Board Milling"),                  QColor("#C800DDFF"),        QColor("#FF00FFFF"));
  addColor(Color::sBoardHoles,                 tr("Board Holes"),                    QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardPads,                  tr("Board Pads"),                     QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(Color::sBoardVias,                  tr("Board Vias"),                     QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(Color::sBoardAirWires,              tr("Board Air Wires"),                Qt::yellow,                 Qt::yellow        );
  addColor(Color::sBoardMeasures,              tr("Board Measures"),                 QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(Color::sBoardAlignment,             tr("Board Alignment"),                QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(Color::sBoardDocumentation,         tr("Board Documentation"),            QColor("#96E0E0E0"),        QColor("#DCE0E0E0"));
  addColor(Color::sBoardComments,              tr("Board Comments"),                 QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(Color::sBoardGuide,                 tr("Board Guide"),                    QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(Color::sBoardPlacementTop,          tr("Board Placement Top"),            QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(Color::sBoardPlacementBot,          tr("Board Placement Bottom"),         QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(Color::sBoardDocumentationTop,      tr("Board Documentation Top"),        QColor("#96E0E0E0"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardDocumentationBot,      tr("Board Documentation Bottom"),     QColor("#96E0E0E0"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardGrabAreasTop,          tr("Board Grab Areas Top"),           QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(Color::sBoardGrabAreasBot,          tr("Board Grab Areas Bottom"),        QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(Color::sBoardHiddenGrabAreasTop,    tr("Board Hidden Grab Areas Top"),    QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(Color::sBoardHiddenGrabAreasBot,    tr("Board Hidden Grab Areas Bottom"), QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(Color::sBoardReferencesTop,         tr("Board References Top"),           QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(Color::sBoardReferencesBot,         tr("Board References Bottom"),        QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(Color::sBoardNamesTop,              tr("Board Names Top"),                QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardNamesBot,              tr("Board Names Bottom"),             QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardValuesTop,             tr("Board Values Top"),               QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardValuesBot,             tr("Board Values Bottom"),            QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardCourtyardTop,          tr("Board Courtyard Top"),            QColor("#4600FFFF"),        QColor("#5A00FFFF")  );
  addColor(Color::sBoardCourtyardBot,          tr("Board Courtyard Bottom"),         QColor("#4600FFFF"),        QColor("#5A00FFFF")  );
  addColor(Color::sBoardStopMaskTop,           tr("Board Stop Mask Top"   ),         QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(Color::sBoardStopMaskBot,           tr("Board Stop Mask Bottom"),         QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(Color::sBoardSolderPasteTop,        tr("Board Solder Paste Top"),         QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(Color::sBoardSolderPasteBot,        tr("Board Solder Paste Bottom"),      QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(Color::sBoardFinishTop,             tr("Board Finish Top"),               QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(Color::sBoardFinishBot,             tr("Board Finish Bottom"),            QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(Color::sBoardGlueTop,               tr("Board Glue Top"),                 QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(Color::sBoardGlueBot,               tr("Board Glue Bottom"),              QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(Color::sBoardCopperTop,             tr("Board Copper Top"),               QColor("#96CC0802"),        QColor("#C0FF0800"));
  // clang-format on
  for (int i = 1; i <= GraphicsLayer::getInnerLayerCount(); ++i) {
    QColor primary;
    QColor secondary;
    switch ((i - 1) % 6) {
      case 0:
        primary = QColor("#96CC57FF");
        secondary = QColor("#C0DA84FF");
        break;
      case 1:
        primary = QColor("#96E2A1FF");
        secondary = QColor("#C0E9BAFF");
        break;
      case 2:
        primary = QColor("#96EE5C9B");
        secondary = QColor("#C0FF4C99");
        break;
      case 3:
        primary = QColor("#96E50063");
        secondary = QColor("#C0E50063");
        break;
      case 4:
        primary = QColor("#96A70049");
        secondary = QColor("#C0CC0058");
        break;
      case 5:
        primary = QColor("#967B20A3");
        secondary = QColor("#C09739BF");
        break;
      default:
        qWarning() << "Unhandled switch-case in theme color initialization.";
        primary = QColor("#FFFF00FF");
        secondary = QColor("#FFFF00FF");
    }
    addColor(QString(Color::sBoardCopperInner).arg(i),
             tr("Board Copper Inner %1").arg(i), primary, secondary);
  }
  addColor(Color::sBoardCopperBot, tr("Board Copper Bottom"),
           QColor("#964578CC"), QColor("#C00A66FC"));
}

Theme::Theme(const Uuid& uuid, const QString& name,
             const Theme& copyFrom) noexcept
  : Theme(copyFrom) {
  mUuid = uuid;
  mName = name;
}

Theme::Theme(const Theme& other) noexcept
  : mNodes(other.mNodes),
    mUuid(other.mUuid),
    mName(other.mName),
    mColors(other.mColors),
    mSchematicGridStyle(other.mSchematicGridStyle),
    mBoardGridStyle(other.mBoardGridStyle) {
}

Theme::~Theme() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const ThemeColor& Theme::getColor(const QString& identifier) const noexcept {
  foreach (const ThemeColor& color, mColors) {
    if (color.getIdentifier() == identifier) {
      return color;
    }
  }

  qCritical() << "Requested unknown theme color:" << identifier;
  static ThemeColor fallback("", "", QColor(), QColor());
  return fallback;
}

const ThemeColor& Theme::getColorForLayer(const QString& layerName) const
    noexcept {
  static QHash<QString, QString> m;
  if (m.isEmpty()) {
    m.insert(GraphicsLayer::sSchematicReferences, Color::sSchematicReferences);
    m.insert(GraphicsLayer::sSchematicSheetFrames, Color::sSchematicFrames);
    m.insert(GraphicsLayer::sSchematicNetLines, Color::sSchematicWires);
    m.insert(GraphicsLayer::sSchematicNetLabels, Color::sSchematicNetLabels);
    m.insert(GraphicsLayer::sSchematicNetLabelAnchors,
             Color::sSchematicNetLabelAnchors);
    m.insert(GraphicsLayer::sSchematicDocumentation,
             Color::sSchematicDocumentation);
    m.insert(GraphicsLayer::sSchematicComments, Color::sSchematicComments);
    m.insert(GraphicsLayer::sSchematicGuide, Color::sSchematicGuide);
    m.insert(GraphicsLayer::sSymbolOutlines, Color::sSchematicOutlines);
    m.insert(GraphicsLayer::sSymbolGrabAreas, Color::sSchematicGrabAreas);
    m.insert(GraphicsLayer::sSymbolHiddenGrabAreas,
             Color::sSchematicHiddenGrabAreas);
    m.insert(GraphicsLayer::sSymbolNames, Color::sSchematicNames);
    m.insert(GraphicsLayer::sSymbolValues, Color::sSchematicValues);
    m.insert(GraphicsLayer::sSymbolPinCirclesOpt,
             Color::sSchematicOptionalPins);
    m.insert(GraphicsLayer::sSymbolPinCirclesReq,
             Color::sSchematicRequiredPins);
    m.insert(GraphicsLayer::sSymbolPinLines, Color::sSchematicPinLines);
    m.insert(GraphicsLayer::sSymbolPinNames, Color::sSchematicPinNames);
    m.insert(GraphicsLayer::sSymbolPinNumbers, Color::sSchematicPinNumbers);
    m.insert(GraphicsLayer::sBoardSheetFrames, Color::sBoardFrames);
    m.insert(GraphicsLayer::sBoardOutlines, Color::sBoardOutlines);
    m.insert(GraphicsLayer::sBoardMillingPth, Color::sBoardMilling);
    m.insert(GraphicsLayer::sBoardDrillsNpth, Color::sBoardHoles);
    m.insert(GraphicsLayer::sBoardPadsTht, Color::sBoardPads);
    m.insert(GraphicsLayer::sBoardViasTht, Color::sBoardVias);
    m.insert(GraphicsLayer::sBoardAirWires, Color::sBoardAirWires);
    m.insert(GraphicsLayer::sBoardMeasures, Color::sBoardMeasures);
    m.insert(GraphicsLayer::sBoardAlignment, Color::sBoardAlignment);
    m.insert(GraphicsLayer::sBoardDocumentation, Color::sBoardDocumentation);
    m.insert(GraphicsLayer::sBoardComments, Color::sBoardComments);
    m.insert(GraphicsLayer::sBoardGuide, Color::sBoardGuide);
    m.insert(GraphicsLayer::sTopPlacement, Color::sBoardPlacementTop);
    m.insert(GraphicsLayer::sBotPlacement, Color::sBoardPlacementBot);
    m.insert(GraphicsLayer::sTopDocumentation, Color::sBoardDocumentationTop);
    m.insert(GraphicsLayer::sBotDocumentation, Color::sBoardDocumentationBot);
    m.insert(GraphicsLayer::sTopGrabAreas, Color::sBoardGrabAreasTop);
    m.insert(GraphicsLayer::sBotGrabAreas, Color::sBoardGrabAreasBot);
    m.insert(GraphicsLayer::sTopHiddenGrabAreas,
             Color::sBoardHiddenGrabAreasTop);
    m.insert(GraphicsLayer::sBotHiddenGrabAreas,
             Color::sBoardHiddenGrabAreasBot);
    m.insert(GraphicsLayer::sTopReferences, Color::sBoardReferencesTop);
    m.insert(GraphicsLayer::sBotReferences, Color::sBoardReferencesBot);
    m.insert(GraphicsLayer::sTopNames, Color::sBoardNamesTop);
    m.insert(GraphicsLayer::sBotNames, Color::sBoardNamesBot);
    m.insert(GraphicsLayer::sTopValues, Color::sBoardValuesTop);
    m.insert(GraphicsLayer::sBotValues, Color::sBoardValuesBot);
    m.insert(GraphicsLayer::sTopCourtyard, Color::sBoardCourtyardTop);
    m.insert(GraphicsLayer::sBotCourtyard, Color::sBoardCourtyardBot);
    m.insert(GraphicsLayer::sTopStopMask, Color::sBoardStopMaskTop);
    m.insert(GraphicsLayer::sBotStopMask, Color::sBoardStopMaskBot);
    m.insert(GraphicsLayer::sTopSolderPaste, Color::sBoardSolderPasteTop);
    m.insert(GraphicsLayer::sBotSolderPaste, Color::sBoardSolderPasteBot);
    m.insert(GraphicsLayer::sTopFinish, Color::sBoardFinishTop);
    m.insert(GraphicsLayer::sBotFinish, Color::sBoardFinishBot);
    m.insert(GraphicsLayer::sTopGlue, Color::sBoardGlueTop);
    m.insert(GraphicsLayer::sBotGlue, Color::sBoardGlueBot);
    m.insert(GraphicsLayer::sTopCopper, Color::sBoardCopperTop);
    for (int i = 1; i <= GraphicsLayer::getInnerLayerCount(); ++i) {
      m.insert(GraphicsLayer::getInnerLayerName(i),
               QString(Color::sBoardCopperInner).arg(i));
    }
    m.insert(GraphicsLayer::sBotCopper, Color::sBoardCopperBot);
  }
  return getColor(m.value(layerName));
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Theme::setName(const QString& name) noexcept {
  mName = name;
}

void Theme::setColors(const QList<ThemeColor>& colors) noexcept {
  if (colors != mColors) {
    mColors = colors;

    // Create backup of all color settings.
    QMap<QString, SExpression> childs;
    foreach (const SExpression* child,
             mNodes["colors"].getChildren(SExpression::Type::List)) {
      childs[child->getName()] = *child;
    }

    // Merge modified settings into existing settings.
    foreach (const ThemeColor& color, colors) {
      if (color.isEdited()) {
        childs[color.getIdentifier()] = color.serialize();
      }
    }

    // Store result.
    SExpression& node = addNode("colors");
    foreach (const SExpression& child, childs) {
      node.ensureLineBreak();
      node.appendChild(child);
    }
    node.ensureLineBreak();
  }
}

void Theme::setSchematicGridStyle(GridStyle style) noexcept {
  if (style != mSchematicGridStyle) {
    mSchematicGridStyle = style;
    addNode("schematic_grid_style").appendChild(style);
  }
}

void Theme::setBoardGridStyle(GridStyle style) noexcept {
  if (style != mBoardGridStyle) {
    mBoardGridStyle = style;
    addNode("board_grid_style").appendChild(style);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Theme::restoreDefaults() noexcept {
  *this = Theme(mUuid, mName);
}

void Theme::load(const SExpression& root) {
  mUuid = deserialize<Uuid>(root.getChild("@0"));
  mName = root.getChild("@1").getValue();
  foreach (const SExpression* node, root.getChildren(SExpression::Type::List)) {
    mNodes.insert(node->getName(), *node);
  }
  if (const SExpression* child = root.tryGetChild("colors")) {
    for (ThemeColor& color : mColors) {
      if (const SExpression* colorNode =
              child->tryGetChild(color.getIdentifier())) {
        color.load(*colorNode);
      }
    }
  }
  if (const SExpression* value = root.tryGetChild("schematic_grid_style/@0")) {
    mSchematicGridStyle = deserialize<GridStyle>(*value);
  }
  if (const SExpression* value = root.tryGetChild("board_grid_style/@0")) {
    mBoardGridStyle = deserialize<GridStyle>(*value);
  }
}

void Theme::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild(mName);
  foreach (const SExpression& node, mNodes) {
    root.ensureLineBreak();
    root.appendChild(node);
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Theme::operator==(const Theme& rhs) const noexcept {
  return (mNodes == rhs.mNodes)  //
      && (mUuid == rhs.mUuid)  //
      && (mName == rhs.mName)  //
      && (mColors == rhs.mColors)  //
      && (mSchematicGridStyle == rhs.mSchematicGridStyle)  //
      && (mBoardGridStyle == rhs.mBoardGridStyle)  //
      ;
}

Theme& Theme::operator=(const Theme& rhs) noexcept {
  mNodes = rhs.mNodes;
  mUuid = rhs.mUuid;
  mName = rhs.mName;
  mColors = rhs.mColors;
  mSchematicGridStyle = rhs.mSchematicGridStyle;
  mBoardGridStyle = rhs.mBoardGridStyle;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Theme::addColor(const QString& id, const QString& name,
                     const QColor& primary, const QColor& secondary) noexcept {
  mColors.append(ThemeColor(id, name, primary, secondary));
}

SExpression& Theme::addNode(const QString& name) noexcept {
  mNodes[name] = SExpression::createList(name);
  return mNodes[name];
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
