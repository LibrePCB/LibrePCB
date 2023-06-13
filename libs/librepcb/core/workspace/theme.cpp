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
#include "../types/layer.h"

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
  const QString sch = tr("Schematic");
  const QString brd = tr("Board");
  addColor(Color::sSchematicBackground,        sch, tr("Background/Grid"),      Qt::white,                  Qt::gray);
  addColor(Color::sSchematicOverlays,          sch, tr("Overlays"),             QColor(255, 255, 255, 120), Qt::black);
  addColor(Color::sSchematicInfoBox,           sch, tr("Info Box"),             QColor(255, 255, 255, 130), Qt::black);
  addColor(Color::sSchematicSelection,         sch, tr("Selection"),            QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(Color::sSchematicReferences,        sch, tr("References"),           QColor(0, 0, 0, 50),        QColor(0, 0, 0, 80));
  addColor(Color::sSchematicFrames,            sch, tr("Frames"),               Qt::black,                  Qt::darkGray);
  addColor(Color::sSchematicWires,             sch, tr("Wires"),                Qt::darkGreen,              Qt::green);
  addColor(Color::sSchematicNetLabels,         sch, tr("Net Labels"),           Qt::darkGreen,              Qt::green);
  addColor(Color::sSchematicNetLabelAnchors,   sch, tr("Net Label Anchors"),    Qt::darkGray,               Qt::gray);
  addColor(Color::sSchematicDocumentation,     sch, tr("Documentation"),        Qt::darkGray,               Qt::gray);
  addColor(Color::sSchematicComments,          sch, tr("Comments"),             Qt::darkBlue,               Qt::blue);
  addColor(Color::sSchematicGuide,             sch, tr("Guide"),                Qt::darkYellow,             Qt::yellow);
  addColor(Color::sSchematicOutlines,          sch, tr("Outlines"),             Qt::darkRed,                Qt::red);
  addColor(Color::sSchematicGrabAreas,         sch, tr("Grab Areas"),           QColor(255, 255, 225, 255), QColor(255, 255, 205, 255));
  addColor(Color::sSchematicHiddenGrabAreas,   sch, tr("Hidden Grab Areas"),    QColor(0, 0, 255, 30),      QColor(0, 0, 255, 50));
  addColor(Color::sSchematicNames,             sch, tr("Names"),                QColor(32, 32, 32, 255),    Qt::darkGray);
  addColor(Color::sSchematicValues,            sch, tr("Values"),               QColor(80, 80, 80, 255),    Qt::gray);
  addColor(Color::sSchematicOptionalPins,      sch, tr("Optional Pins"),        QColor(0, 255, 0, 255),     QColor(0, 255, 0, 127));
  addColor(Color::sSchematicRequiredPins,      sch, tr("Required Pins"),        QColor(255, 0, 0, 255),     QColor(255, 0, 0, 127));
  addColor(Color::sSchematicPinLines,          sch, tr("Pin Lines"),            Qt::darkRed,                Qt::red);
  addColor(Color::sSchematicPinNames,          sch, tr("Pin Names"),            QColor(64, 64, 64, 255),    Qt::gray);
  addColor(Color::sSchematicPinNumbers,        sch, tr("Pin Numbers"),          QColor(64, 64, 64, 255),    Qt::gray);
  addColor(Color::sBoardBackground,            brd, tr("Background/Grid"),          Qt::black,                  Qt::gray);
  addColor(Color::sBoardOverlays,              brd, tr("Overlays"),                 QColor(0, 0, 0, 120),       Qt::yellow);
  addColor(Color::sBoardInfoBox,               brd, tr("Info Box"),                 QColor(0, 0, 0, 130),       Qt::yellow);
  addColor(Color::sBoardDrcMarker,             brd, tr("DRC Marker"),               Qt::transparent,            QColor(255, 127, 0, 255));
  addColor(Color::sBoardSelection,             brd, tr("Selection"),                QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(Color::sBoardFrames,                brd, tr("Frames"),                   QColor("#96E0E0E0"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardOutlines,              brd, tr("Outlines"),                 QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardPlatedCutouts,         brd, tr("Plated Cutouts"),           QColor("#C800DDFF"),        QColor("#FF00FFFF"));
  addColor(Color::sBoardHoles,                 brd, tr("Holes"),                    QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(Color::sBoardPads,                  brd, tr("Pads"),                     QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(Color::sBoardVias,                  brd, tr("Vias"),                     QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(Color::sBoardZones,                 brd, tr("Zones"),                    QColor("#80494949"),        QColor("#A0666666"));
  addColor(Color::sBoardAirWires,              brd, tr("Air Wires"),                Qt::yellow,                 Qt::yellow        );
  addColor(Color::sBoardMeasures,              brd, tr("Measures"),                 QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(Color::sBoardAlignment,             brd, tr("Alignment"),                QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(Color::sBoardDocumentation,         brd, tr("Documentation"),            QColor("#96E0E0E0"),        QColor("#DCE0E0E0"));
  addColor(Color::sBoardComments,              brd, tr("Comments"),                 QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(Color::sBoardGuide,                 brd, tr("Guide"),                    QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(Color::sBoardLegendTop,             brd, tr("Legend Top"),               QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(Color::sBoardLegendBot,             brd, tr("Legend Bottom"),            QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(Color::sBoardDocumentationTop,      brd, tr("Documentation Top"),        QColor("#96E0E0E0"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardDocumentationBot,      brd, tr("Documentation Bottom"),     QColor("#96E0E0E0"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardGrabAreasTop,          brd, tr("Grab Areas Top"),           QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(Color::sBoardGrabAreasBot,          brd, tr("Grab Areas Bottom"),        QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(Color::sBoardHiddenGrabAreasTop,    brd, tr("Hidden Grab Areas Top"),    QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(Color::sBoardHiddenGrabAreasBot,    brd, tr("Hidden Grab Areas Bottom"), QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(Color::sBoardReferencesTop,         brd, tr("References Top"),           QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(Color::sBoardReferencesBot,         brd, tr("References Bottom"),        QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(Color::sBoardNamesTop,              brd, tr("Names Top"),                QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardNamesBot,              brd, tr("Names Bottom"),             QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardValuesTop,             brd, tr("Values Top"),               QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardValuesBot,             brd, tr("Values Bottom"),            QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(Color::sBoardCourtyardTop,          brd, tr("Courtyard Top"),            QColor("#4600FFFF"),        QColor("#5A00FFFF")  );
  addColor(Color::sBoardCourtyardBot,          brd, tr("Courtyard Bottom"),         QColor("#4600FFFF"),        QColor("#5A00FFFF")  );
  addColor(Color::sBoardStopMaskTop,           brd, tr("Stop Mask Top"   ),         QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(Color::sBoardStopMaskBot,           brd, tr("Stop Mask Bottom"),         QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(Color::sBoardSolderPasteTop,        brd, tr("Solder Paste Top"),         QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(Color::sBoardSolderPasteBot,        brd, tr("Solder Paste Bottom"),      QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(Color::sBoardFinishTop,             brd, tr("Finish Top"),               QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(Color::sBoardFinishBot,             brd, tr("Finish Bottom"),            QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(Color::sBoardGlueTop,               brd, tr("Glue Top"),                 QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(Color::sBoardGlueBot,               brd, tr("Glue Bottom"),              QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(Color::sBoardCopperTop,             brd, tr("Copper Top"),               QColor("#96CC0802"),        QColor("#C0FF0800"));
  // clang-format on
  for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
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
    addColor(QString(Color::sBoardCopperInner).arg(i), brd,
             tr("Copper Inner %1").arg(i), primary, secondary);
  }
  addColor(Color::sBoardCopperBot, brd, tr("Copper Bottom"),
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
  static ThemeColor fallback("", "", "", QColor(), QColor());
  return fallback;
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
 *  Static Methods
 ******************************************************************************/

const QSet<QString>& Theme::getCopperColorNames() noexcept {
  static QSet<QString> names;
  if (names.isEmpty()) {
    names.insert(Color::sBoardCopperTop);
    names.insert(Color::sBoardCopperBot);
    for (int i = 1; i <= Layer::innerCopperCount(); ++i) {
      names.insert(QString(Color::sBoardCopperInner).arg(i));
    }
  }
  return names;
}

QString Theme::getGrabAreaColorName(const QString& outlineColorName) noexcept {
  if (outlineColorName == Color::sBoardLegendTop) {
    return Color::sBoardGrabAreasTop;
  } else if (outlineColorName == Color::sBoardLegendBot) {
    return Color::sBoardGrabAreasBot;
  } else if (outlineColorName == Color::sSchematicOutlines) {
    return Color::sSchematicGrabAreas;
  } else {
    return QString();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Theme::addColor(const QString& id, const QString& category,
                     const QString& name, const QColor& primary,
                     const QColor& secondary) noexcept {
  mColors.append(ThemeColor(id, category, name, primary, secondary));
}

SExpression& Theme::addNode(const QString& name) noexcept {
  mNodes[name] = SExpression::createList(name);
  return mNodes[name];
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
