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

#ifndef LIBREPCB_KICADIMPORT_KICADTYPES_H
#define LIBREPCB_KICADIMPORT_KICADTYPES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class MessageLogger;
class SExpression;

namespace kicadimport {

// File format documentation:
// https://dev-docs.kicad.org/en/file-formats/sexpr-intro/index.html

/*******************************************************************************
 *  Enums
 ******************************************************************************/

enum class KiCadEdge {
  Unknown,  // Parse error
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
};

enum class KiCadStrokeType {
  Unknown,  // Parse error
  Dash,
  DashDot,
  DashDotDOt,
  Dot,
  Default,
  Solid,
};

enum class KiCadSymbolTextType {
  Unknown,  // Parse error
  Reference,
  Value,
  User,
};

enum class KiCadSymbolFillType {
  Unknown,  // Parse error
  None,
  Outline,
  Background,
};

enum class KiCadPinType {
  Unknown,  // Parse error
  Input,
  Output,
  Bidirectional,
  TriState,
  Passive,
  Free,
  Unspecified,
  PowerIn,
  PowerOut,
  OpenCollector,
  OpenEmitter,
  NoConnect,
};

enum class KiCadPinStyle {
  Unknown,  // Parse error
  Line,
  Inverted,
  Clock,
  InvertedClock,
  InputLow,
  ClockLow,
  OutputLow,
  EdgeClockHigh,
  NonLogic,
};

enum class KiCadFootprintFillType {
  Unknown,  // Parse error
  None,
  Solid,
};

enum class KiCadPadType {
  Unknown,  // Parse error
  ThruHole,
  Smd,
  Connect,  // Edge connector
  NpThruHole,
};

enum class KiCadPadShape {
  Unknown,  // Parse error
  Circle,
  Rect,
  Oval,
  Trapezoid,
  RoundRect,
  Custom,
};

enum class KiCadCustomPadAnchor {
  Unknown,  // Parse error
  Unspecified,
  Circle,
  Rect,
};

enum class KiCadPadProperty {
  Unknown,  // Parse error
  Unspecified,
  Bga,
  FiducialGlobal,
  FiducialLocal,
  Testpoint,
  Heatsink,
  Castellated,
};

enum class KiCadZoneConnect {
  Unknown,  // Parse error
  NoConnect,  // Mode 0
  ThermalReliefs,  // Mode 1
  Solid,  // Mode 2
};

enum class KiCadLayer {
  Unknown,  // Parse error
  AllCopper,  // *.Cu
  AllSolderMask,  // *.Mask
  AllSilkscreen,  // *.SilkS
  FrontAndBackCopper,  // F&B.Cu
  FrontAdhesion,
  FrontCopper,
  FrontCourtyard,
  FrontFabrication,
  FrontPaste,
  FrontSilkscreen,
  FrontSolderMask,
  InnerCopper1,
  InnerCopper2,
  InnerCopper3,
  InnerCopper4,
  InnerCopper5,
  InnerCopper6,
  InnerCopper7,
  InnerCopper8,
  InnerCopper9,
  InnerCopper10,
  InnerCopper11,
  InnerCopper12,
  InnerCopper13,
  InnerCopper14,
  InnerCopper15,
  InnerCopper16,
  InnerCopper17,
  InnerCopper18,
  InnerCopper19,
  InnerCopper20,
  InnerCopper21,
  InnerCopper22,
  InnerCopper23,
  InnerCopper24,
  InnerCopper25,
  InnerCopper26,
  InnerCopper27,
  InnerCopper28,
  InnerCopper29,
  InnerCopper30,
  BackAdhesion,
  BackCopper,
  BackCourtyard,
  BackFabrication,
  BackPaste,
  BackSilkscreen,
  BackSolderMask,
  BoardOutline,
  UserComment,
  UserDrawing,
  User1,
  User2,
  User3,
  User4,
  User5,
  User6,
  User7,
  User8,
  User9,
};

/*******************************************************************************
 *  Struct KiCadProperty
 ******************************************************************************/

/**
 * @brief Represents a KiCad property
 */
struct KiCadProperty final {
  Q_DECLARE_TR_FUNCTIONS(KiCadProperty)

public:
  QString key;
  QString value;
  QPointF position;
  qreal rotation = 0;  // May not be set.
  QString layer;  // May not be set.
  QSizeF fontSize;  // May not be set.
  qreal fontThickness = 0;  // May not be set.
  Qt::Alignment alignment = Qt::AlignCenter;  // May not be set.
  bool mirror = false;  // May not be set.
  bool unlocked = false;  // May not be set.
  bool hide = false;  // May not be set.

  static KiCadProperty parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadGraphicalLine
 ******************************************************************************/

/**
 * @brief Represents a KiCad graphical line
 */
struct KiCadGraphicalLine final {
  Q_DECLARE_TR_FUNCTIONS(KiCadGraphicalLine)

public:
  QPointF start;
  QPointF end;
  qreal width = 0;

  static KiCadGraphicalLine parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadGraphicalArc
 ******************************************************************************/

/**
 * @brief Represents a KiCad graphical arc
 */
struct KiCadGraphicalArc final {
  Q_DECLARE_TR_FUNCTIONS(KiCadGraphicalArc)

public:
  QPointF start;
  QPointF mid;
  QPointF end;
  qreal width = 0;

  static KiCadGraphicalArc parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadGraphicalCircle
 ******************************************************************************/

/**
 * @brief Represents a KiCad graphical circle
 */
struct KiCadGraphicalCircle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadGraphicalCircle)

public:
  QPointF center;
  QPointF end;
  qreal width = 0;
  bool fill = false;  // May not be set.

  static KiCadGraphicalCircle parse(const SExpression& node,
                                    MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadGraphicalPolygon
 ******************************************************************************/

/**
 * @brief Represents a KiCad graphical polygon
 */
struct KiCadGraphicalPolygon final {
  Q_DECLARE_TR_FUNCTIONS(KiCadGraphicalPolygon)

public:
  QList<QPointF> coordinates;
  qreal width = 0;
  bool fill = false;  // May not be set.

  static KiCadGraphicalPolygon parse(const SExpression& node,
                                     MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadZone
 ******************************************************************************/

/**
 * @brief Represents a KiCad zone
 */
struct KiCadZone final {
  Q_DECLARE_TR_FUNCTIONS(KiCadZone)

public:
  QList<KiCadLayer> layers;
  bool keepOutTracks = false;
  bool keepOutVias = false;
  bool keepOutPads = false;
  bool keepOutCopperPour = false;
  bool keepOutFootprints = false;
  QList<QPointF> polygon;

  static KiCadZone parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolArc
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol arc
 */
struct KiCadSymbolArc final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolArc)

public:
  QPointF start;
  QPointF mid;
  QPointF end;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;
  KiCadSymbolFillType fillType = KiCadSymbolFillType::None;

  static KiCadSymbolArc parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolCircle
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol circle
 */
struct KiCadSymbolCircle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolCircle)

public:
  QPointF center;
  qreal radius = 0;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;
  KiCadSymbolFillType fillType = KiCadSymbolFillType::None;

  static KiCadSymbolCircle parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolRectangle
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol rectangle
 */
struct KiCadSymbolRectangle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolRectangle)

public:
  QPointF start;
  QPointF end;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;
  KiCadSymbolFillType fillType = KiCadSymbolFillType::None;

  static KiCadSymbolRectangle parse(const SExpression& node,
                                    MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolPolyline
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol polyline
 */
struct KiCadSymbolPolyline final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolPolyline)

public:
  QList<QPointF> coordinates;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;
  KiCadSymbolFillType fillType = KiCadSymbolFillType::None;

  static KiCadSymbolPolyline parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolText
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol text
 */
struct KiCadSymbolText final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolText)

public:
  QString text;
  QPointF position;
  qreal rotation = 0;  // May not be set.
  QSizeF fontSize;  // May not be set.
  qreal fontThickness = 0;  // May not be set.
  Qt::Alignment alignment = Qt::AlignCenter;  // May not be set.

  static KiCadSymbolText parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolPin
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol pin
 */
struct KiCadSymbolPin final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolPin)

public:
  KiCadPinType type;
  KiCadPinStyle shape;
  QPointF position;
  qreal rotation = 0;  // May not be set.
  qreal length;
  QString name;
  QString number;

  static KiCadSymbolPin parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolGate
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol gate
 */
struct KiCadSymbolGate final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolGate)

public:
  enum class Style { Common, Base, DeMorgan };

  QString name;  // Including index and style suffixes.
  int index = 0;  // 0 = common to all gates
  Style style = Style::Common;
  QList<KiCadSymbolArc> arcs;
  QList<KiCadSymbolCircle> circles;
  QList<KiCadSymbolRectangle> rectangles;
  QList<KiCadSymbolPolyline> polylines;
  QList<KiCadSymbolText> texts;
  QList<KiCadSymbolPin> pins;

  static KiCadSymbolGate parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbol
 ******************************************************************************/

/**
 * @brief Represents a KiCad symbol
 */
struct KiCadSymbol final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbol)

public:
  QString name;
  QString extends;  // May not be set.
  qreal pinNamesOffset = 0.508;  // May not be set.
  bool hidePinNumbers = false;  // May not be set.
  bool hidePinNames = false;  // May not be set.
  bool excludeFromSim = false;  // May not be set.
  bool inBom = true;  // May not be set.
  bool onBoard = true;  // May not be set.
  QList<KiCadProperty> properties;
  QList<KiCadSymbolGate> gates;

  static KiCadSymbol parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadSymbolLibrary
 ******************************************************************************/

/**
 * @brief Represents the content of a *.kicad_sym file
 */
struct KiCadSymbolLibrary final {
  Q_DECLARE_TR_FUNCTIONS(KiCadSymbolLibrary)

public:
  int version;
  QString generator;
  QList<KiCadSymbol> symbols;

  static KiCadSymbolLibrary parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintLine
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint line
 */
struct KiCadFootprintLine final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintLine)

public:
  QPointF start;
  QPointF end;
  qreal strokeWidth;
  KiCadStrokeType strokeType;
  KiCadLayer layer;

  static KiCadFootprintLine parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintArc
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint arc
 */
struct KiCadFootprintArc final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintArc)

public:
  QPointF start;
  QPointF mid;
  QPointF end;
  qreal strokeWidth;
  KiCadStrokeType strokeType;
  KiCadLayer layer;

  static KiCadFootprintArc parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintCircle
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint circle
 */
struct KiCadFootprintCircle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintCircle)

public:
  QPointF center;
  QPointF end;
  KiCadLayer layer;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;  // May not be set.
  KiCadFootprintFillType fillType =
      KiCadFootprintFillType::None;  // May not be set.

  static KiCadFootprintCircle parse(const SExpression& node,
                                    MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintRectangle
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint rectangle
 */
struct KiCadFootprintRectangle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintRectangle)

public:
  QPointF start;
  QPointF end;
  KiCadLayer layer;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;  // May not be set.
  KiCadFootprintFillType fillType =
      KiCadFootprintFillType::None;  // May not be set.

  static KiCadFootprintRectangle parse(const SExpression& node,
                                       MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintPolygon
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint polygon
 */
struct KiCadFootprintPolygon final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintPolygon)

public:
  QList<QPointF> coordinates;
  KiCadLayer layer;
  qreal strokeWidth = 0;
  KiCadStrokeType strokeType = KiCadStrokeType::Solid;  // May not be set.
  KiCadFootprintFillType fillType =
      KiCadFootprintFillType::None;  // May not be set.

  static KiCadFootprintPolygon parse(const SExpression& node,
                                     MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintText
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint text
 */
struct KiCadFootprintText final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintText)

public:
  KiCadSymbolTextType type;
  QString text;
  QPointF position;
  qreal rotation = 0;  // May not be set.
  KiCadLayer layer;
  QSizeF fontSize;  // May not be set.
  qreal fontThickness = 0;  // May not be set.
  Qt::Alignment alignment = Qt::AlignCenter;  // May not be set.
  bool mirror = false;  // May not be set.
  bool unlocked = false;  // May not be set.

  static KiCadFootprintText parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintPad
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint pad
 */
struct KiCadFootprintPad final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintPad)

public:
  QString number;
  KiCadPadType type;
  KiCadPadShape shape;
  QPointF position;
  QPointF offset;  // Offset of shape from drill.
  qreal rotation = 0;  // May not be set.
  QSizeF size;
  QSizeF drill;  // May not be set.
  QList<KiCadLayer> layers;
  KiCadPadProperty property = KiCadPadProperty::Unspecified;  // May not be set.
  qreal solderMaskMargin = 0;  // May not be set.
  qreal solderPasteMargin = 0;  // May not be set.
  qreal solderPasteMarginRatio = 0;  // May not be set.
  qreal thermalBridgeAngle = 0;  // May not be set.
  qreal thermalBridgeWidth = 0;  // May not be set.
  qreal clearance = 0;  // May not be set.
  bool removeUnusedLayers = false;  // May not be set.
  qreal roundRectRRatio = 0;  // May not be set.
  QSizeF rectDelta;  // May not be set.
  qreal chamferRatio = 0;  // May not be set.
  QList<KiCadEdge> chamferEdges;  // May not be set.
  KiCadCustomPadAnchor customPadAnchor =
      KiCadCustomPadAnchor::Unspecified;  // May not be set.
  QList<KiCadGraphicalLine> graphicalLines;  // May not be set.
  QList<KiCadGraphicalArc> graphicalArcs;  // May not be set.
  QList<KiCadGraphicalCircle> graphicalCircles;  // May not be set.
  QList<KiCadGraphicalPolygon> graphicalPolygons;  // May not be set.

  static KiCadFootprintPad parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprintModel
 ******************************************************************************/

/**
 * @brief Represents the 3D model of a KiCad footprint
 */
struct KiCadFootprintModel final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprintModel)

public:
  QString path;
  QVector3D offset;
  QVector3D scale;
  QVector3D rotate;

  static KiCadFootprintModel parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadFootprint
 ******************************************************************************/

/**
 * @brief Represents a KiCad footprint
 */
struct KiCadFootprint final {
  Q_DECLARE_TR_FUNCTIONS(KiCadFootprint)

public:
  QString name;
  int version = -1;  // May not be set.
  QString generator;  // May not be set.
  KiCadLayer layer = KiCadLayer::Unknown;
  QString description;  // May not be set.
  QString tags;  // May not be set.
  bool isSmd = false;  // May not be set.
  bool isThroughHole = false;  // May not be set.
  bool boardOnly = false;  // May not be set.
  bool excludeFromPosFiles = false;  // May not be set.
  bool excludeFromBom = false;  // May not be set.
  qreal solderMaskMargin = 0;  // May not be set.
  qreal solderPasteMargin = 0;  // May not be set.
  qreal solderPasteRatio = 0;  // May not be set.
  qreal clearance = 0;  // May not be set.
  std::optional<KiCadZoneConnect> zoneConnect;  // May not be set.
  QList<QStringList> netTiePadGroups;  // May not be set.
  QList<KiCadProperty> properties;
  QList<KiCadFootprintLine> lines;
  QList<KiCadFootprintArc> arcs;
  QList<KiCadFootprintCircle> circles;
  QList<KiCadFootprintRectangle> rectangles;
  QList<KiCadFootprintPolygon> polygons;
  QList<KiCadFootprintText> texts;
  QList<KiCadFootprintPad> pads;
  QList<KiCadZone> zones;
  QList<KiCadFootprintModel> models;

  static KiCadFootprint parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
