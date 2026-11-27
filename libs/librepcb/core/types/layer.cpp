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
#include "../serialization/serialization.h"
#include "../serialization/sexpression.h"
#include "../workspace/colorrole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Layer::Layer(const QString& id, const QString& nameTr,
             const ColorRole& colorRole, Flags flags) noexcept
  : mId(id), mNameTr(nameTr), mColorRole(colorRole), mFlags(flags) {
}

Layer::Layer(const Layer& other) noexcept
  : mId(other.mId),
    mNameTr(other.mNameTr),
    mColorRole(other.mColorRole),
    mFlags(other.mFlags) {
}

Layer::~Layer() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Layer& Layer::mirrored(int innerLayers) const noexcept {
  static QHash<const Layer*, const Layer*> sMap = {
      {&topNames(), &botNames()},
      {&topValues(), &botValues()},
      {&topLegend(), &botLegend()},
      {&topDocumentation(), &botDocumentation()},
      {&topPackageOutlines(), &botPackageOutlines()},
      {&topCourtyard(), &botCourtyard()},
      {&topHiddenGrabAreas(), &botHiddenGrabAreas()},
      {&topStopMask(), &botStopMask()},
      {&topSolderPaste(), &botSolderPaste()},
      {&topFinish(), &botFinish()},
      {&topGlue(), &botGlue()},
      {&topCopper(), &botCopper()},
  };
  if (sMap.contains(this)) {
    return *sMap.value(this);
  } else if (const Layer* layer = sMap.key(this)) {
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
  static Layer sObj("sch_frames", tr("Sheet Frames"),
                    ColorRole::schematicFrames(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::schematicDocumentation() noexcept {
  static Layer sObj("sch_documentation", tr("Documentation"),
                    ColorRole::schematicDocumentation(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::schematicComments() noexcept {
  static Layer sObj("sch_comments", tr("Comments"),
                    ColorRole::schematicComments(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::schematicGuide() noexcept {
  static Layer sObj("sch_guide", tr("Guide"), ColorRole::schematicGuide(),
                    Flag::Schematic);
  return sObj;
}

const Layer& Layer::symbolOutlines() noexcept {
  static Layer sObj("sym_outlines", tr("Outlines"),
                    ColorRole::schematicOutlines(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::symbolHiddenGrabAreas() noexcept {
  static Layer sObj("sym_hidden_grab_areas", tr("Hidden Grab Areas"),
                    ColorRole::schematicHiddenGrabAreas(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::symbolNames() noexcept {
  static Layer sObj("sym_names", tr("Names"), ColorRole::schematicNames(),
                    Flag::Schematic);
  return sObj;
}

const Layer& Layer::symbolValues() noexcept {
  static Layer sObj("sym_values", tr("Values"), ColorRole::schematicValues(),
                    Flag::Schematic);
  return sObj;
}

const Layer& Layer::symbolPinNames() noexcept {
  static Layer sObj("sym_pin_names", tr("Pin Names"),
                    ColorRole::schematicPinNames(), Flag::Schematic);
  return sObj;
}

const Layer& Layer::boardSheetFrames() noexcept {
  static Layer sObj("brd_frames", tr("Sheet Frames"), ColorRole::boardFrames(),
                    Flag::Board);
  return sObj;
}

const Layer& Layer::boardOutlines() noexcept {
  static Layer sObj("brd_outlines", tr("Board Outlines"),
                    ColorRole::boardOutlines(), Flag::Board | Flag::BoardEdge);
  return sObj;
}

const Layer& Layer::boardCutouts() noexcept {
  static Layer sObj("brd_cutouts", tr("Board Cutouts"),
                    ColorRole::boardOutlines(),  // No dedicated color ID.
                    Flag::Board | Flag::BoardEdge);
  return sObj;
}

const Layer& Layer::boardPlatedCutouts() noexcept {
  static Layer sObj("brd_plated_cutouts", tr("Plated Board Cutouts"),
                    ColorRole::boardPlatedCutouts(),
                    Flag::Board | Flag::BoardEdge);
  return sObj;
}

const Layer& Layer::boardMeasures() noexcept {
  static Layer sObj("brd_measures", tr("Measures"), ColorRole::boardMeasures(),
                    Flag::Board);
  return sObj;
}

const Layer& Layer::boardAlignment() noexcept {
  static Layer sObj("brd_alignment", tr("Alignment"),
                    ColorRole::boardAlignment(), Flag::Board);
  return sObj;
}

const Layer& Layer::boardDocumentation() noexcept {
  static Layer sObj("brd_documentation", tr("Documentation"),
                    ColorRole::boardDocumentation(), Flag::Board);
  return sObj;
}

const Layer& Layer::boardComments() noexcept {
  static Layer sObj("brd_comments", tr("Comments"), ColorRole::boardComments(),
                    Flag::Board);
  return sObj;
}

const Layer& Layer::boardGuide() noexcept {
  static Layer sObj("brd_guide", tr("Guide"), ColorRole::boardGuide(),
                    Flag::Board);
  return sObj;
}

const Layer& Layer::topNames() noexcept {
  static Layer sObj("top_names", tr("Top Names"), ColorRole::boardNamesTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botNames() noexcept {
  static Layer sObj("bot_names", tr("Bottom Names"), ColorRole::boardNamesBot(),
                    Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topValues() noexcept {
  static Layer sObj("top_values", tr("Top Values"), ColorRole::boardValuesTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botValues() noexcept {
  static Layer sObj("bot_values", tr("Bottom Values"),
                    ColorRole::boardValuesBot(), Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topLegend() noexcept {
  static Layer sObj("top_legend", tr("Top Legend"), ColorRole::boardLegendTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botLegend() noexcept {
  static Layer sObj("bot_legend", tr("Bottom Legend"),
                    ColorRole::boardLegendBot(), Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topDocumentation() noexcept {
  static Layer sObj("top_documentation", tr("Top Documentation"),
                    ColorRole::boardDocumentationTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botDocumentation() noexcept {
  static Layer sObj("bot_documentation", tr("Bottom Documentation"),
                    ColorRole::boardDocumentationBot(),
                    Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topPackageOutlines() noexcept {
  static Layer sObj("top_package_outlines", tr("Top Package Outlines"),
                    ColorRole::boardPackageOutlinesTop(),
                    Flag::Board | Flag::Top | Flag::PackageOutline |
                        Flag::PolygonsRepresentAreas);
  return sObj;
}

const Layer& Layer::botPackageOutlines() noexcept {
  static Layer sObj("bot_package_outlines", tr("Bottom Package Outlines"),
                    ColorRole::boardPackageOutlinesBot(),
                    Flag::Board | Flag::Bottom | Flag::PackageOutline |
                        Flag::PolygonsRepresentAreas);
  return sObj;
}

const Layer& Layer::topCourtyard() noexcept {
  static Layer sObj("top_courtyard", tr("Top Courtyard"),
                    ColorRole::boardCourtyardTop(),
                    Flag::Board | Flag::Top | Flag::PackageCourtyard |
                        Flag::PolygonsRepresentAreas);
  return sObj;
}

const Layer& Layer::botCourtyard() noexcept {
  static Layer sObj("bot_courtyard", tr("Bottom Courtyard"),
                    ColorRole::boardCourtyardBot(),
                    Flag::Board | Flag::Bottom | Flag::PackageCourtyard |
                        Flag::PolygonsRepresentAreas);
  return sObj;
}

const Layer& Layer::topHiddenGrabAreas() noexcept {
  static Layer sObj("top_hidden_grab_areas", tr("Top Hidden Grab Areas"),
                    ColorRole::boardHiddenGrabAreasTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botHiddenGrabAreas() noexcept {
  static Layer sObj("bot_hidden_grab_areas", tr("Bottom Hidden Grab Areas"),
                    ColorRole::boardHiddenGrabAreasBot(),
                    Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topStopMask() noexcept {
  static Layer sObj("top_stop_mask", tr("Top Stop Mask"),
                    ColorRole::boardStopMaskTop(),
                    Flag::Board | Flag::Top | Flag::StopMask);
  return sObj;
}

const Layer& Layer::botStopMask() noexcept {
  static Layer sObj("bot_stop_mask", tr("Bottom Stop Mask"),
                    ColorRole::boardStopMaskBot(),
                    Flag::Board | Flag::Bottom | Flag::StopMask);
  return sObj;
}

const Layer& Layer::topSolderPaste() noexcept {
  static Layer sObj("top_solder_paste", tr("Top Solder Paste"),
                    ColorRole::boardSolderPasteTop(),
                    Flag::Board | Flag::Top | Flag::SolderPaste);
  return sObj;
}

const Layer& Layer::botSolderPaste() noexcept {
  static Layer sObj("bot_solder_paste", tr("Bottom Solder Paste"),
                    ColorRole::boardSolderPasteBot(),
                    Flag::Board | Flag::Bottom | Flag::SolderPaste);
  return sObj;
}

const Layer& Layer::topFinish() noexcept {
  static Layer sObj("top_finish", tr("Top Finish"), ColorRole::boardFinishTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botFinish() noexcept {
  static Layer sObj("bot_finish", tr("Bottom Finish"),
                    ColorRole::boardFinishBot(), Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topGlue() noexcept {
  static Layer sObj("top_glue", tr("Top Glue"), ColorRole::boardGlueTop(),
                    Flag::Board | Flag::Top);
  return sObj;
}

const Layer& Layer::botGlue() noexcept {
  static Layer sObj("bot_glue", tr("Bottom Glue"), ColorRole::boardGlueBot(),
                    Flag::Board | Flag::Bottom);
  return sObj;
}

const Layer& Layer::topCopper() noexcept {
  static Layer sObj("top_cu", tr("Top Copper"), ColorRole::boardCopperTop(),
                    Flag::Board | Flag::Top | Flag::Copper);
  return sObj;
}

const Layer& Layer::botCopper() noexcept {
  static Layer sObj("bot_cu", tr("Bottom Copper"), ColorRole::boardCopperBot(),
                    Flag::Board | Flag::Bottom | Flag::Copper |
                        static_cast<Flag>(innerCopperCount() + 1));  // NOLINT
  return sObj;
}

const QVector<const Layer*>& Layer::innerCopper() noexcept {
  // Thread-safe static initialization.
  static QVector<const Layer*> sLayers = []() {
    QVector<const Layer*> list;
    for (int i = 1; i <= innerCopperCount(); ++i) {
      const ColorRole* role = ColorRole::boardCopperInner(i);
      Q_ASSERT(role);
      list.append(new Layer(QString("in%1_cu").arg(i),
                            tr("Inner Copper %1").arg(i), *role,
                            Flag::Board | Flag::Inner | Flag::Copper |
                                static_cast<Flag>(i)));  // NOLINT
    }
    return list;
  }();
  return sLayers;
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
  static QVector<const Layer*> sList =
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
          &boardCutouts(),  //
          &boardPlatedCutouts(),  //
          &boardMeasures(),  //
          &boardAlignment(),  //
          &boardDocumentation(),  //
          &boardComments(),  //
          &boardGuide(),  //
          &topNames(),  //
          &topValues(),  //
          &topLegend(),  //
          &topDocumentation(),  //
          &topPackageOutlines(),  //
          &topCourtyard(),  //
          &topHiddenGrabAreas(),  //
          &topStopMask(),  //
          &topSolderPaste(),  //
          &topFinish(),  //
          &topGlue(),  //
          &topCopper(),  //
      } +
      innerCopper() +
      QVector<const Layer*>{
          &botCopper(),  //
          &botNames(),  //
          &botValues(),  //
          &botLegend(),  //
          &botDocumentation(),  //
          &botPackageOutlines(),  //
          &botCourtyard(),  //
          &botHiddenGrabAreas(),  //
          &botStopMask(),  //
          &botSolderPaste(),  //
          &botFinish(),  //
          &botGlue(),  //
      };
  return sList;
}

const Layer& Layer::get(QStringView id) {
  if (auto l = find(id)) {
    return *l;
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Unknown layer: '%1'").arg(id));
}

const Layer* Layer::find(QStringView id) {
  foreach (const Layer* layer, all()) {
    if (layer->getId() == id) {
      return layer;
    }
  }
  return nullptr;
}

bool Layer::lessThan(const Layer* a, const Layer* b) noexcept {
  const int indexA = all().indexOf(a);
  const int indexB = all().indexOf(b);
  return indexA < indexB;
}

QVector<const Layer*> Layer::sorted(const QSet<const Layer*>& set) noexcept {
  QVector<const Layer*> list = set.values();
  std::sort(list.begin(), list.end(), Layer::lessThan);
  return list;
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
QString serializeToString(const Layer& obj) noexcept {
  return obj.getId();
}

template <>
std::optional<const Layer*> deserialize(QStringView str) noexcept {
  if (auto l = Layer::find(str)) {
    return l;
  }
  return std::nullopt;
}

template <>
std::unique_ptr<SExpression> serialize(const Layer& obj) {
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
