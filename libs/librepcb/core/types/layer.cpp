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
#include "layer.h"

#include "../exceptions.h"
#include "../serialization/sexpression.h"
#include "../workspace/theme.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Layer::Layer(const QString& id, const QString& nameTr,
             const QString& themeColor, Flags flags) noexcept
  : mId(id), mNameTr(nameTr), mThemeColor(themeColor), mFlags(flags) {
}

Layer::Layer(const Layer& other) noexcept
  : mId(other.mId),
    mNameTr(other.mNameTr),
    mThemeColor(other.mThemeColor),
    mFlags(other.mFlags) {
}

Layer::~Layer() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Layer& Layer::mirrored(int innerLayers) const noexcept {
  static QHash<const Layer*, const Layer*> map = {
      {&topPlacement(), &botPlacement()},
      {&topDocumentation(), &botDocumentation()},
      {&topHiddenGrabAreas(), &botHiddenGrabAreas()},
      {&topNames(), &botNames()},
      {&topValues(), &botValues()},
      {&topCourtyard(), &botCourtyard()},
      {&topStopMask(), &botStopMask()},
      {&topSolderPaste(), &botSolderPaste()},
      {&topFinish(), &botFinish()},
      {&topGlue(), &botGlue()},
      {&topCopper(), &botCopper()},
  };
  if (map.contains(this)) {
    return *map.value(this);
  } else if (const Layer* layer = map.key(this)) {
    return *layer;
  } else if ((innerLayers >= 0) && (innerLayers <= innerCopperCount()) &&
             isInner()) {
    const Layer* layer = innerCopper(innerLayers - getCopperNumber() + 1);
    Q_ASSERT(layer);
    return *layer;
  } else {
    return *this;
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const Layer& Layer::schematicSheetFrames() noexcept {
  static Layer layer("sch_frames", tr("Sheet Frames"),
                     Theme::Color::sSchematicFrames, Flag::Schematic);
  return layer;
}

const Layer& Layer::schematicDocumentation() noexcept {
  static Layer layer("sch_documentation", tr("Documentation"),
                     Theme::Color::sSchematicDocumentation, Flag::Schematic);
  return layer;
}

const Layer& Layer::schematicComments() noexcept {
  static Layer layer("sch_comments", tr("Comments"),
                     Theme::Color::sSchematicComments, Flag::Schematic);
  return layer;
}

const Layer& Layer::schematicGuide() noexcept {
  static Layer layer("sch_guide", tr("Guide"), Theme::Color::sSchematicGuide,
                     Flag::Schematic);
  return layer;
}

const Layer& Layer::symbolOutlines() noexcept {
  static Layer layer("sym_outlines", tr("Outlines"),
                     Theme::Color::sSchematicOutlines, Flag::Schematic);
  return layer;
}

const Layer& Layer::symbolHiddenGrabAreas() noexcept {
  static Layer layer("sym_hidden_grab_areas", tr("Hidden Grab Areas"),
                     Theme::Color::sSchematicHiddenGrabAreas, Flag::Schematic);
  return layer;
}

const Layer& Layer::symbolNames() noexcept {
  static Layer layer("sym_names", tr("Names"), Theme::Color::sSchematicNames,
                     Flag::Schematic);
  return layer;
}

const Layer& Layer::symbolValues() noexcept {
  static Layer layer("sym_values", tr("Values"), Theme::Color::sSchematicValues,
                     Flag::Schematic);
  return layer;
}

const Layer& Layer::symbolPinNames() noexcept {
  static Layer layer("sym_pin_names", tr("Pin Names"),
                     Theme::Color::sSchematicPinNames, Flag::Schematic);
  return layer;
}

const Layer& Layer::boardSheetFrames() noexcept {
  static Layer layer("brd_frames", tr("Sheet Frames"),
                     Theme::Color::sBoardFrames, Flag::Board);
  return layer;
}

const Layer& Layer::boardOutlines() noexcept {
  static Layer layer("brd_outlines", tr("Board Outlines"),
                     Theme::Color::sBoardOutlines, Flag::Board);
  return layer;
}

const Layer& Layer::boardMillingPth() noexcept {
  static Layer layer("brd_milling_pth", tr("Milling (PTH)"),
                     Theme::Color::sBoardMilling, Flag::Board);
  return layer;
}

const Layer& Layer::boardMeasures() noexcept {
  static Layer layer("brd_measures", tr("Measures"),
                     Theme::Color::sBoardMeasures, Flag::Board);
  return layer;
}

const Layer& Layer::boardAlignment() noexcept {
  static Layer layer("brd_alignment", tr("Alignment"),
                     Theme::Color::sBoardAlignment, Flag::Board);
  return layer;
}

const Layer& Layer::boardDocumentation() noexcept {
  static Layer layer("brd_documentation", tr("Documentation"),
                     Theme::Color::sBoardDocumentation, Flag::Board);
  return layer;
}

const Layer& Layer::boardComments() noexcept {
  static Layer layer("brd_comments", tr("Comments"),
                     Theme::Color::sBoardComments, Flag::Board);
  return layer;
}

const Layer& Layer::boardGuide() noexcept {
  static Layer layer("brd_guide", tr("Guide"), Theme::Color::sBoardGuide,
                     Flag::Board);
  return layer;
}

const Layer& Layer::topPlacement() noexcept {
  static Layer layer("top_placement", tr("Top Placement"),
                     Theme::Color::sBoardPlacementTop, Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botPlacement() noexcept {
  static Layer layer("bot_placement", tr("Bottom Placement"),
                     Theme::Color::sBoardPlacementBot,
                     Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topDocumentation() noexcept {
  static Layer layer("top_documentation", tr("Top Documentation"),
                     Theme::Color::sBoardDocumentationTop,
                     Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botDocumentation() noexcept {
  static Layer layer("bot_documentation", tr("Bottom Documentation"),
                     Theme::Color::sBoardDocumentationBot,
                     Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topHiddenGrabAreas() noexcept {
  static Layer layer("top_hidden_grab_areas", tr("Top Hidden Grab Areas"),
                     Theme::Color::sBoardHiddenGrabAreasTop,
                     Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botHiddenGrabAreas() noexcept {
  static Layer layer("bot_hidden_grab_areas", tr("Bottom Hidden Grab Areas"),
                     Theme::Color::sBoardHiddenGrabAreasBot,
                     Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topNames() noexcept {
  static Layer layer("top_names", tr("Top Names"), Theme::Color::sBoardNamesTop,
                     Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botNames() noexcept {
  static Layer layer("bot_names", tr("Bottom Names"),
                     Theme::Color::sBoardNamesBot, Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topValues() noexcept {
  static Layer layer("top_values", tr("Top Values"),
                     Theme::Color::sBoardValuesTop, Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botValues() noexcept {
  static Layer layer("bot_values", tr("Bottom Values"),
                     Theme::Color::sBoardValuesBot, Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topCourtyard() noexcept {
  static Layer layer("top_courtyard", tr("Top Courtyard"),
                     Theme::Color::sBoardCourtyardTop, Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botCourtyard() noexcept {
  static Layer layer("bot_courtyard", tr("Bottom Courtyard"),
                     Theme::Color::sBoardCourtyardBot,
                     Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topStopMask() noexcept {
  static Layer layer("top_stop_mask", tr("Top Stop Mask"),
                     Theme::Color::sBoardStopMaskTop,
                     Flag::Board | Flag::Top | Flag::StopMask);
  return layer;
}

const Layer& Layer::botStopMask() noexcept {
  static Layer layer("bot_stop_mask", tr("Bottom Stop Mask"),
                     Theme::Color::sBoardStopMaskBot,
                     Flag::Board | Flag::Bottom | Flag::StopMask);
  return layer;
}

const Layer& Layer::topSolderPaste() noexcept {
  static Layer layer("top_solder_paste", tr("Top Solder Paste"),
                     Theme::Color::sBoardSolderPasteTop,
                     Flag::Board | Flag::Top | Flag::SolderPaste);
  return layer;
}

const Layer& Layer::botSolderPaste() noexcept {
  static Layer layer("bot_solder_paste", tr("Bottom Solder Paste"),
                     Theme::Color::sBoardSolderPasteBot,
                     Flag::Board | Flag::Bottom | Flag::SolderPaste);
  return layer;
}

const Layer& Layer::topFinish() noexcept {
  static Layer layer("top_finish", tr("Top Finish"),
                     Theme::Color::sBoardFinishTop, Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botFinish() noexcept {
  static Layer layer("bot_finish", tr("Bottom Finish"),
                     Theme::Color::sBoardFinishBot, Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topGlue() noexcept {
  static Layer layer("top_glue", tr("Top Glue"), Theme::Color::sBoardGlueTop,
                     Flag::Board | Flag::Top);
  return layer;
}

const Layer& Layer::botGlue() noexcept {
  static Layer layer("bot_glue", tr("Bottom Glue"), Theme::Color::sBoardGlueBot,
                     Flag::Board | Flag::Bottom);
  return layer;
}

const Layer& Layer::topCopper() noexcept {
  static Layer layer("top_cu", tr("Top Copper"), Theme::Color::sBoardCopperTop,
                     Flag::Board | Flag::Top | Flag::Copper | Flag(0));
  return layer;
}

const Layer& Layer::botCopper() noexcept {
  static Layer layer(
      "bot_cu", tr("Bottom Copper"), Theme::Color::sBoardCopperBot,
      Flag::Board | Flag::Bottom | Flag::Copper | Flag(innerCopperCount() + 1));
  return layer;
}

const QVector<const Layer*>& Layer::innerCopper() noexcept {
  auto createThreadSafe = []() {
    QVector<const Layer*> list;
    for (int i = 1; i <= innerCopperCount(); ++i) {
      list.append(
          new Layer(QString("in%1_cu").arg(i), tr("Inner Copper %1").arg(i),
                    QString(Theme::Color::sBoardCopperInner).arg(i),
                    Flag::Board | Flag::Inner | Flag::Copper | Flag(i)));
    }
    return list;
  };
  static QVector<const Layer*> layers = createThreadSafe();
  return layers;
}

const Layer* Layer::innerCopper(int number) noexcept {
  return innerCopper().value(number - 1, nullptr);
}

int Layer::innerCopperCount() noexcept {
  return 62;  // Results in a total of 64 copper layers.
}

const Layer* Layer::copper(int number) noexcept {
  if (number == 0) {
    return &topCopper();
  } else if (number == (innerCopperCount() + 1)) {
    return &botCopper();
  } else {
    return innerCopper(number);
  }
}

const QVector<const Layer*>& Layer::all() noexcept {
  static QVector<const Layer*> list =
      QVector<const Layer*>{
          &schematicSheetFrames(),  //
          &schematicDocumentation(),  //
          &schematicComments(),  //
          &schematicGuide(),  //
          &symbolOutlines(),  //
          &symbolHiddenGrabAreas(),  //
          &symbolNames(),  //
          &symbolValues(),  //
          &symbolPinNames(),  //
          &boardSheetFrames(),  //
          &boardOutlines(),  //
          &boardMillingPth(),  //
          &boardMeasures(),  //
          &boardAlignment(),  //
          &boardDocumentation(),  //
          &boardComments(),  //
          &boardGuide(),  //
          &topPlacement(),  //
          &topDocumentation(),  //
          &topHiddenGrabAreas(),  //
          &topNames(),  //
          &topValues(),  //
          &topCourtyard(),  //
          &topStopMask(),  //
          &topSolderPaste(),  //
          &topFinish(),  //
          &topGlue(),  //
          &topCopper(),  //
      } +
      innerCopper() +
      QVector<const Layer*>{
          &botCopper(),  //
          &botPlacement(),  //
          &botDocumentation(),  //
          &botHiddenGrabAreas(),  //
          &botNames(),  //
          &botValues(),  //
          &botCourtyard(),  //
          &botStopMask(),  //
          &botSolderPaste(),  //
          &botFinish(),  //
          &botGlue(),  //
      };
  return list;
}

const Layer& Layer::get(const QString& id) {
  foreach (const Layer* layer, all()) {
    if (layer->getId() == id) {
      return *layer;
    }
  }
  throw RuntimeError(__FILE__, __LINE__, tr("Unknown layer: '%1'").arg(id));
}

bool Layer::lessThan(const Layer* a, const Layer* b) noexcept {
  const int indexA = all().indexOf(a);
  const int indexB = all().indexOf(b);
  return indexA < indexB;
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
SExpression serialize(const Layer& obj) {
  return SExpression::createToken(obj.getId());
}

template <>
const Layer* deserialize(const SExpression& node) {
  return &Layer::get(node.getValue());
}

template <>
const Layer& deserialize(const SExpression& node) {
  return Layer::get(node.getValue());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
