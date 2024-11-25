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

#ifndef LIBREPCB_KICADIMPORT_KICADTYPECONVERTER_H
#define LIBREPCB_KICADIMPORT_KICADTYPECONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "kicadtypes.h"

#include <librepcb/core/geometry/path.h>
#include <librepcb/core/library/resource.h>
#include <librepcb/core/types/circuitidentifier.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>
#include <optional/tl/optional.hpp>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class FootprintPad;
class Hole;
class Layer;
class MessageLogger;
class PackagePad;
class Polygon;
class StrokeText;
class SymbolPin;
class Text;
class Zone;

namespace kicadimport {

/*******************************************************************************
 *  Class KiCadTypeConverter
 ******************************************************************************/

/**
 * @brief Helper class to convert KiCad types to LibrePCB types
 */
class KiCadTypeConverter final {
  Q_DECLARE_TR_FUNCTIONS(KiCadTypeConverter)

public:
  // Types
  struct Line {
    const Layer* layer;  // Not nullptr.
    UnsignedLength width;
    Point start;
    Point end;
    Angle angle;
  };
  struct LineGroup {
    const Layer* layer;  // Not nullptr.
    UnsignedLength width;
    QVector<Path> paths;
  };
  struct Pad {
    std::shared_ptr<FootprintPad> fptPad;
    std::shared_ptr<Hole> hole;
    QList<std::shared_ptr<Polygon>> polygons;
  };

  // Constructors / Destructor
  KiCadTypeConverter() = delete;
  KiCadTypeConverter(const KiCadTypeConverter& other) = delete;
  ~KiCadTypeConverter() = delete;

  // General Methods
  static ElementName convertElementName(const QString& s);
  static QString convertElementDescription(const FilePath& libFp,
                                           const QString& elemName,
                                           const QList<KiCadProperty>& props);
  static QString convertElementKeywords(const QString& commonKeywords,
                                        const QList<KiCadProperty>& props);
  static ResourceList convertResources(const QList<KiCadProperty>& props);
  static Point convertSymbolPoint(const QPointF& p);
  static Point convertFootprintPoint(const QPointF& p);
  static Angle convertArc(const Point& start, const Point& mid,
                          const Point& end);
  static UnsignedLength convertSymbolStrokeWidth(qreal width);
  static PositiveLength convertSymbolTextHeight(qreal height);
  static UnsignedLength convertFootprintStrokeWidth(qreal width,
                                                    const Layer& layer);
  static std::shared_ptr<Polygon> convertSymbolArc(const KiCadSymbolArc& a);
  static std::shared_ptr<Circle> convertSymbolCircle(
      const KiCadSymbolCircle& c);
  static std::shared_ptr<Polygon> convertSymbolRectangle(
      const KiCadSymbolRectangle& r);
  static std::shared_ptr<Polygon> convertSymbolPolyline(
      const KiCadSymbolPolyline& p);
  static std::shared_ptr<Text> convertSymbolText(const KiCadSymbolText& t);
  static std::shared_ptr<Text> convertSymbolPropertyToText(
      const KiCadProperty& p);
  static QList<std::pair<QString, QStringList>> convertSymbolPinNames(
      const QList<KiCadSymbolPin>& pins);
  static CircuitIdentifier convertSymbolPinName(const QString& name,
                                                const QStringList& numbers);
  static CircuitIdentifier convertCircuitIdentifier(const QString& text);
  static std::shared_ptr<SymbolPin> convertSymbolPin(const KiCadSymbolPin& p,
                                                     const QString& name,
                                                     qreal pinNamesOffset);
  static const Layer& convertFootprintGeometryLayer(const KiCadLayer& l);
  static Line convertFootprintLine(const KiCadFootprintLine& l);
  static Line convertFootprintArc(const KiCadFootprintArc& a);
  static QList<LineGroup> groupLinesByLayerAndWidth(const QList<Line>& lines);
  static std::shared_ptr<Circle> convertFootprintCircle(
      const KiCadFootprintCircle& c);
  static std::shared_ptr<Polygon> convertFootprintRectangle(
      const KiCadFootprintRectangle& r);
  static std::shared_ptr<Polygon> convertFootprintPolygon(
      const KiCadFootprintPolygon& p);
  static std::shared_ptr<Zone> convertFootprintZone(const KiCadZone& z,
                                                    MessageLogger& log);
  static std::shared_ptr<StrokeText> convertFootprintText(
      const KiCadFootprintText& t);
  static std::shared_ptr<StrokeText> convertFootprintPropertyToText(
      const KiCadProperty& p);
  static Pad convertPad(const KiCadFootprintPad& p, qreal fptSolderMaskMargin,
                        qreal fptSolderPasteMargin, qreal fptSolderPasteRatio,
                        qreal fptClearance, MessageLogger& log);
  static tl::optional<KiCadProperty> findProperty(
      const QList<KiCadProperty>& props, const QString& key);

  // Operator Overloadings
  KiCadTypeConverter& operator=(const KiCadTypeConverter& rhs) = delete;

private:
  /**
   * Returns the maximum allowed arc tolerance when flattening arcs.
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace kicadimport
}  // namespace librepcb

#endif
