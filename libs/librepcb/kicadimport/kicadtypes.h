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

enum class KiCadStrokeType {
  Unknown,  // Parse error
  Default,
};

enum class KiCadFillType {
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
  QVector3D position;
  QString layer;  // May be empty.
  QSizeF fontSize;  // May be invalid.
  qreal fontThickness;  // May be 0 (not set).
  bool hide;

  static KiCadProperty parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  Struct KiCadRectangle
 ******************************************************************************/

/**
 * @brief Represents a KiCad rectangle
 */
struct KiCadRectangle final {
  Q_DECLARE_TR_FUNCTIONS(KiCadRectangle)

public:
  QPointF start;
  QPointF end;
  qreal strokeWidth;
  KiCadStrokeType strokeType;
  KiCadFillType fillType;

  static KiCadRectangle parse(const SExpression& node, MessageLogger& log);
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
  QString layer;

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
  QString layer;

  static KiCadFootprintArc parse(const SExpression& node, MessageLogger& log);
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
  QVector3D pos;
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
  QString name;  // Including index and style suffixes.
  int index;  // 0 = common to all gates
  QList<KiCadRectangle> rectangles;
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
  bool excludeFromSim;
  bool inBom;
  bool onBoard;
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
  int version;  // -1 if unknown
  QString generator;
  bool isSmd;
  bool isThroughHole;
  bool boardOnly;
  bool excludeFromPosFiles;
  bool excludeFromBom;
  QList<KiCadProperty> properties;
  QList<KiCadFootprintLine> lines;
  QList<KiCadFootprintArc> arcs;
  QList<KiCadFootprintModel> models;

  static KiCadFootprint parse(const SExpression& node, MessageLogger& log);
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
