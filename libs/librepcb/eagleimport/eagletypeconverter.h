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

#ifndef LIBREPCB_EAGLEIMPORT_EAGLETYPECONVERTER_H
#define LIBREPCB_EAGLEIMPORT_EAGLETYPECONVERTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/circuitidentifier.h>
#include <librepcb/common/elementname.h>
#include <librepcb/common/geometry/circle.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/geometry/text.h>
#include <librepcb/common/graphics/graphicslayername.h>
#include <librepcb/common/units/angle.h>
#include <librepcb/common/units/length.h>
#include <librepcb/common/units/point.h>
#include <librepcb/library/cmp/componentsymbolvariantitemsuffix.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/library/pkg/packagepad.h>
#include <librepcb/library/sym/symbolpin.h>
#include <optional/tl/optional.hpp>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace parseagle {
class Circle;
class DeviceSet;
class Gate;
class Hole;
class Library;
class Package;
class Pin;
class Polygon;
class Rectangle;
class SmtPad;
class Symbol;
class Text;
class ThtPad;
class Vertex;
class Wire;
struct Point;
}  // namespace parseagle

namespace librepcb {
namespace eagleimport {

/*******************************************************************************
 *  Class EagleTypeConverter
 ******************************************************************************/

/**
 * @brief Helper class to convert EAGLE types to LibrePCB types
 */
class EagleTypeConverter final {
  Q_DECLARE_TR_FUNCTIONS(EagleTypeConverter)

public:
  // Constructors / Destructor
  EagleTypeConverter() = delete;
  EagleTypeConverter(const EagleTypeConverter& other) = delete;
  ~EagleTypeConverter() = delete;

  /**
   * @brief Convert an element (e.g. symbol) name
   *
   * Removes all invalid characters from an EAGLe element name and convert it
   * to the corresponding LibrePCB type. If completely invalid, "Unnamed" will
   * be returned (no error).
   *
   * @param n   EAGLE element name (e.g. "R-0805")
   *
   * @return LibrePCB element name
   */
  static ElementName convertElementName(const QString& n);

  /**
   * @brief Convert an element (e.g. symbol) description
   *
   * Removes HTML tags and returns only the plain text.
   *
   * @param d   EAGLE element description (might contain HTML)
   *
   * @return LibrePCB element description (no HTML)
   */
  static QString convertElementDescription(const QString& d);

  /**
   * @brief Convert a component gate name
   *
   * Removes all invalid characters and returns the gate name as component
   * symbol variant item suffix.
   *
   * @param n   EAGLE gate name
   *
   * @return LibrePCB component symbol variant item suffix
   */
  static library::ComponentSymbolVariantItemSuffix convertGateName(
      const QString& n);

  /**
   * @brief Convert a pin or pad name
   *
   * Removes all invalid characters and returns the name as a circuit
   * identifier.
   *
   * @param n   EAGLE pin or pad name
   *
   * @return LibrePCB circuit identifier
   */
  static CircuitIdentifier convertPinOrPadName(const QString& n);

  /**
   * @brief Convert a layer ID
   *
   * @param id  EAGLE layer ID
   *
   * @return LibrePCB graphics layer
   *
   * @throw Exception   If the layer is unknown or not supported
   */
  static GraphicsLayerName convertLayer(int id);

  /**
   * @brief Convert a length
   *
   * @param l   EAGLE length [mm]
   *
   * @return LibrePCB length
   */
  static Length convertLength(double l);

  /**
   * @brief Convert a point
   *
   * @param p   EAGLE point [mm]
   *
   * @return LibrePCB point
   */
  static Point convertPoint(const parseagle::Point& p);

  /**
   * @brief Convert an angle
   *
   * @param a   EAGLE angle [°]
   *
   * @return LibrePCB angle
   */
  static Angle convertAngle(double a);

  /**
   * @brief Convert a vertex
   *
   * @param v   EAGLE vertex
   *
   * @return LibrePCB vertex
   */
  static Vertex convertVertex(const parseagle::Vertex& v);

  /**
   * @brief Convert vertices
   *
   * @param v       EAGLE vertices
   * @param close   If true, the returned path will always be closed
   *
   * @return LibrePCB path
   */
  static Path convertVertices(const QList<parseagle::Vertex>& v, bool close);

  /**
   * @brief Convert a wire
   *
   * @param w   EAGLE wire (line segment)
   *
   * @return LibrePCB polygon containing 1 line segment
   */
  static std::shared_ptr<Polygon> convertWire(const parseagle::Wire& w);

  /**
   * @brief Convert a rectangle
   *
   * @param r           EAGLE rectangle
   * @param isGrabArea  If the returned polygon should be a grab area
   *
   * @return LibrePCB polygon containing 4 line segments
   */
  static std::shared_ptr<Polygon> convertRectangle(
      const parseagle::Rectangle& r, bool isGrabArea);

  /**
   * @brief Convert a polygon
   *
   * @param p           EAGLE polygon
   * @param isGrabArea  If the returned polygon should be a grab area
   *
   * @return LibrePCB polygon (always closed)
   */
  static std::shared_ptr<Polygon> convertPolygon(const parseagle::Polygon& p,
                                                 bool isGrabArea);

  /**
   * @brief Convert a circle
   *
   * @param c           EAGLE circle
   * @param isGrabArea  If the returned circle should be a grab area
   *
   * @return LibrePCB circle
   */
  static std::shared_ptr<Circle> convertCircle(const parseagle::Circle& c,
                                               bool isGrabArea);

  /**
   * @brief Convert a hole
   *
   * @param h   EAGLE hole
   *
   * @return LibrePCB hole
   */
  static std::shared_ptr<Hole> convertHole(const parseagle::Hole& h);

  /**
   * @brief Convert a text value
   *
   * @param v   EAGLE text value (e.g. ">NAME")
   *
   * @return LibrePCB text value (e.g. "{{NAME}}")
   */
  static QString convertTextValue(const QString& v);

  /**
   * @brief Convert a schematic/symbol text
   *
   * @param t   EAGLE text
   *
   * @return LibrePCB text
   */
  static std::shared_ptr<Text> convertSchematicText(const parseagle::Text& t);

  /**
   * @brief Convert a board/footprint text
   *
   * @param t   EAGLE text
   *
   * @return LibrePCB text
   */
  static std::shared_ptr<StrokeText> convertBoardText(const parseagle::Text& t);

  /**
   * @brief Convert a symbol pin
   *
   * @param p   EAGLE pin
   *
   * @return LibrePCB pin
   */
  static std::shared_ptr<library::SymbolPin> convertSymbolPin(
      const parseagle::Pin& p);

  /**
   * @brief Convert a THT pad
   *
   * @param p   EAGLE pad
   *
   * @return LibrePCB package pad + footprint pad
   */
  static std::pair<std::shared_ptr<library::PackagePad>,
                   std::shared_ptr<library::FootprintPad> >
      convertThtPad(const parseagle::ThtPad& p);

  /**
   * @brief Convert an SMT pad
   *
   * @param p   EAGLE pad
   *
   * @return LibrePCB package pad + footprint pad
   */
  static std::pair<std::shared_ptr<library::PackagePad>,
                   std::shared_ptr<library::FootprintPad> >
      convertSmtPad(const parseagle::SmtPad& p);

  // Operator Overloadings
  EagleTypeConverter& operator=(const EagleTypeConverter& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
