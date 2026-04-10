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
#include "colorrole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Theme::Theme(const Uuid& uuid, const QString& name) noexcept
  : mNodes(), mUuid(uuid), mName(name), mColors() {
  // clang-format off
  const char* sch = QT_TR_NOOP("Schematic");
  const char* brd = QT_TR_NOOP("Board");
  const char* view3d = QT_TR_NOOP("3D View");
  addColor(ColorRole::schematicBackground(),        sch, Qt::white,                  Qt::gray);
  addColor(ColorRole::schematicOverlays(),          sch, QColor(255, 255, 255, 120), Qt::black);
  addColor(ColorRole::schematicInfoBox(),           sch, QColor(255, 255, 255, 130), Qt::black);
  addColor(ColorRole::schematicSelection(),         sch, QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(ColorRole::schematicReferences(),        sch, QColor(0, 0, 0, 50),        QColor(0, 0, 0, 80));
  addColor(ColorRole::schematicFrames(),            sch, Qt::black,                  Qt::darkGray);
  addColor(ColorRole::schematicWires(),             sch, Qt::darkGreen,              Qt::green);
  addColor(ColorRole::schematicNetLabels(),         sch, Qt::darkGreen,              Qt::green);
  addColor(ColorRole::schematicBuses(),             sch, QColor(0, 142, 255, 255),   QColor(114, 192, 255, 255));
  addColor(ColorRole::schematicBusLabels(),         sch, QColor(0, 142, 255, 255),   QColor(114, 192, 255, 255));
  addColor(ColorRole::schematicImageBorders(),      sch, Qt::darkGray,               Qt::gray);
  addColor(ColorRole::schematicDocumentation(),     sch, Qt::darkGray,               Qt::gray);
  addColor(ColorRole::schematicComments(),          sch, Qt::darkBlue,               Qt::blue);
  addColor(ColorRole::schematicGuide(),             sch, Qt::darkYellow,             Qt::yellow);
  addColor(ColorRole::schematicOutlines(),          sch, Qt::darkRed,                Qt::red);
  addColor(ColorRole::schematicGrabAreas(),         sch, QColor(255, 255, 225, 255), QColor(255, 255, 205, 255));
  addColor(ColorRole::schematicHiddenGrabAreas(),   sch, QColor(0, 0, 255, 30),      QColor(0, 0, 255, 50));
  addColor(ColorRole::schematicNames(),             sch, QColor(32, 32, 32, 255),    Qt::darkGray);
  addColor(ColorRole::schematicValues(),            sch, QColor(80, 80, 80, 255),    Qt::gray);
  addColor(ColorRole::schematicOptionalPins(),      sch, QColor(0, 255, 0, 255),     QColor(0, 255, 0, 127));
  addColor(ColorRole::schematicRequiredPins(),      sch, QColor(255, 0, 0, 255),     QColor(255, 0, 0, 127));
  addColor(ColorRole::schematicPinLines(),          sch, Qt::darkRed,                Qt::red);
  addColor(ColorRole::schematicPinNames(),          sch, QColor(64, 64, 64, 255),    Qt::gray);
  addColor(ColorRole::schematicPinNumbers(),        sch, QColor(64, 64, 64, 255),    Qt::gray);
  addColor(ColorRole::boardBackground(),            brd, Qt::black,                  Qt::gray);
  addColor(ColorRole::boardOverlays(),              brd, QColor(0, 0, 0, 120),       Qt::yellow);
  addColor(ColorRole::boardInfoBox(),               brd, QColor(0, 0, 0, 130),       Qt::yellow);
  addColor(ColorRole::boardDrcMarker(),             brd, Qt::transparent,            QColor(255, 127, 0, 255));
  addColor(ColorRole::boardSelection(),             brd, QColor(120, 170, 255, 255), QColor(150, 200, 255, 80));
  addColor(ColorRole::boardFrames(),                brd, QColor("#96E0E0E0"),        QColor("#FFFFFFFF"));
  addColor(ColorRole::boardOutlines(),              brd, QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(ColorRole::boardPlatedCutouts(),         brd, QColor("#C800DDFF"),        QColor("#FF00FFFF"));
  addColor(ColorRole::boardHoles(),                 brd, QColor("#C8FFFFFF"),        QColor("#FFFFFFFF"));
  addColor(ColorRole::boardPads(),                  brd, QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(ColorRole::boardVias(),                  brd, QColor("#966DB515"),        QColor("#B44EFC14"));
  addColor(ColorRole::boardZones(),                 brd, QColor("#80494949"),        QColor("#A0666666"));
  addColor(ColorRole::boardAirWires(),              brd, Qt::yellow,                 Qt::yellow        );
  addColor(ColorRole::boardMeasures(),              brd, QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(ColorRole::boardAlignment(),             brd, QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(ColorRole::boardDocumentation(),         brd, QColor("#76FBC697"),        QColor("#B6FBC697"));
  addColor(ColorRole::boardComments(),              brd, QColor("#B4E59500"),        QColor("#DCFFBF00"));
  addColor(ColorRole::boardGuide(),                 brd, QColor("#FF808000"),        QColor("#FFA3B200"));
  addColor(ColorRole::boardNamesTop(),              brd, QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(ColorRole::boardNamesBot(),              brd, QColor("#96EDFFD8"),        QColor("#DCE0E0E0")  );
  addColor(ColorRole::boardValuesTop(),             brd, QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(ColorRole::boardValuesBot(),             brd, QColor("#96D8F2FF"),        QColor("#DCE0E0E0")  );
  addColor(ColorRole::boardLegendTop(),             brd, QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(ColorRole::boardLegendBot(),             brd, QColor("#BBFFFFFF"),        QColor("#FFFFFFFF")  );
  addColor(ColorRole::boardDocumentationTop(),      brd, QColor("#76FBC697"),        QColor("#B6FBC697")  );
  addColor(ColorRole::boardDocumentationBot(),      brd, QColor("#76FBC697"),        QColor("#B6FBC697")  );
  addColor(ColorRole::boardPackageOutlinesTop(),    brd, QColor("#C000FFFF"),        QColor("#FF00FFFF")  );
  addColor(ColorRole::boardPackageOutlinesBot(),    brd, QColor("#C000FFFF"),        QColor("#FF00FFFF")  );
  addColor(ColorRole::boardCourtyardTop(),          brd, QColor("#C0FF00FF"),        QColor("#FFFF00FF")  );
  addColor(ColorRole::boardCourtyardBot(),          brd, QColor("#C0FF00FF"),        QColor("#FFFF00FF")  );
  addColor(ColorRole::boardGrabAreasTop(),          brd, QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(ColorRole::boardGrabAreasBot(),          brd, QColor("#14FFFFFF"),        QColor("#32FFFFFF")  );
  addColor(ColorRole::boardHiddenGrabAreasTop(),    brd, QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(ColorRole::boardHiddenGrabAreasBot(),    brd, QColor("#28FFFFFF"),        QColor("#46FFFFFF")  );
  addColor(ColorRole::boardReferencesTop(),         brd, QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(ColorRole::boardReferencesBot(),         brd, QColor("#64FFFFFF"),        QColor("#B4FFFFFF")  );
  addColor(ColorRole::boardStopMaskTop(),           brd, QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(ColorRole::boardStopMaskBot(),           brd, QColor("#30FFFFFF"),        QColor("#60FFFFFF")  );
  addColor(ColorRole::boardSolderPasteTop(),        brd, QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(ColorRole::boardSolderPasteBot(),        brd, QColor("#20E0E0E0"),        QColor("#40E0E0E0")  );
  addColor(ColorRole::boardFinishTop(),             brd, QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(ColorRole::boardFinishBot(),             brd, QColor(255, 0, 0, 130),     QColor(255, 0, 0, 130));
  addColor(ColorRole::boardGlueTop(),               brd, QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(ColorRole::boardGlueBot(),               brd, QColor("#64E0E0E0"),        QColor("#78E0E0E0")  );
  addColor(ColorRole::boardCopperTop(),             brd, QColor("#96CC0802"),        QColor("#C0FF0800"));
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
        primary = QColor("#96E50063");
        secondary = QColor("#C0E50063");
        break;
      case 2:
        primary = QColor("#96EE5C9B");
        secondary = QColor("#C0FF4C99");
        break;
      case 3:
        primary = QColor("#96E2A1FF");
        secondary = QColor("#C0E9BAFF");
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
    if (const ColorRole* role = ColorRole::boardCopperInner(i)) {
      addColor(*role, brd, primary, secondary);
    }
  }
  addColor(ColorRole::boardCopperBot(), brd, QColor("#964578CC"),
           QColor("#C00A66FC"));
  // Use a background color which ensures good contrast to both black and white
  // STEP models. The secondary color is used e.g. for overlay buttons.
  addColor(ColorRole::board3dBackground(), view3d, QColor(230, 242, 255),
           Qt::black);
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
    mColors(other.mColors) {
}

Theme::~Theme() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const ThemeColor& Theme::getColor(const ColorRole& role) const noexcept {
  return getColor(role.getId());
}

const ThemeColor& Theme::getColor(const QString& role) const noexcept {
  foreach (const ThemeColor& color, mColors) {
    if (color.getRole().getId() == role) {
      return color;
    }
  }

  qCritical() << "Requested unknown theme color:" << role;
  static ThemeColor fallback(ColorRole::boardBackground(), "", QColor(),
                             QColor());
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
        std::unique_ptr<const SExpression> sexpr = color.serialize();
        childs[color.getRole().getId()] = *sexpr;
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
              child->tryGetChild(color.getRole().getId())) {
        color.load(*colorNode);
      }
    }
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
      ;
}

Theme& Theme::operator=(const Theme& rhs) noexcept {
  mNodes = rhs.mNodes;
  mUuid = rhs.mUuid;
  mName = rhs.mName;
  mColors = rhs.mColors;
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Theme::addColor(const ColorRole& role, const char* category,
                     const QColor& primary, const QColor& secondary) noexcept {
  mColors.append(ThemeColor(role, category, primary, secondary));
}

SExpression& Theme::addNode(const QString& name) noexcept {
  std::unique_ptr<const SExpression> sexpr = SExpression::createList(name);
  mNodes[name] = *sexpr;
  return mNodes[name];
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
