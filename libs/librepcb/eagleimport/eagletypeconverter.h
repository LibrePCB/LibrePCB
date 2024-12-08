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
#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/cmp/componentprefix.h>
#include <librepcb/core/library/cmp/componentsymbolvariantitemsuffix.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/packagepad.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/types/boundedunsignedratio.h>
#include <librepcb/core/types/circuitidentifier.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/simplestring.h>
#include <parseagle/common/enums.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace parseagle {
class Attribute;
class Circle;
class DeviceSet;
class Frame;
class Gate;
class Grid;
class Hole;
class Library;
class Package;
class Param;
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

class Layer;
class LengthUnit;
class MessageLogger;

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
  // Types

  /**
   * @brief Intermediate geometry type used for converting polygon-like
   *        EAGLE elements
   */
  struct Geometry {
    int layerId;
    UnsignedLength lineWidth;
    bool filled;
    bool grabArea;
    Path path;
    std::optional<std::pair<Point, PositiveLength>> circle;
  };

  /**
   * @brief LibrePCB data structure to represent an EAGLE symbol pin
   */
  struct Pin {
    std::shared_ptr<SymbolPin> pin;
    std::shared_ptr<Circle> circle;
    std::shared_ptr<Polygon> polygon;
  };

  // Constructors / Destructor
  EagleTypeConverter() = delete;
  EagleTypeConverter(const EagleTypeConverter& other) = delete;
  ~EagleTypeConverter() = delete;

  /**
   * @brief Convert an element (e.g. symbol) name
   *
   * Removes all invalid characters from an EAGLE element name and convert it
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
   * @brief Convert a component name
   *
   * Like #convertElementName(), but also removes trailing separation
   * characters.
   *
   * @param n   EAGLE component name (e.g. "R-0805-")
   *
   * @return LibrePCB component name (e.g. "R-0805")
   */
  static ElementName convertComponentName(QString n);

  /**
   * @brief Convert a device name
   *
   * Like #convertElementName(), but concatenating the EAGLE device set name
   * with the EAGLE device name.
   *
   * @param deviceSetName   EAGLE device set name
   * @param deviceName      EAGLE device name
   *
   * @return LibrePCB device name
   */
  static ElementName convertDeviceName(const QString& deviceSetName,
                                       const QString& deviceName);

  /**
   * @brief Convert a component prefix
   *
   * Removes all invalid characters and returns the component prefix in the
   * corresponding LibrePCB type.
   *
   * @param p   EAGLE device set prefix
   *
   * @return LibrePCB component prefix
   */
  static ComponentPrefix convertComponentPrefix(const QString& p);

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
  static ComponentSymbolVariantItemSuffix convertGateName(const QString& n);

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
   * @brief Convert the inversion syntax of a text
   *
   * @param s   EAGLE text possibly containing inversion signs (e.g. "!RST!/EN")
   *
   * @return Same text but with LibrePCB inversion syntax (e.g. "!RST/EN")
   */
  static QString convertInversionSyntax(const QString& s) noexcept;

  /**
   * @brief Try converting an attribute
   *
   * @param a           EAGLE attribute
   * @param log         Message logger
   *
   * @return LibrePCB attribute (nullptr on failure)
   */
  static std::shared_ptr<Attribute> tryConvertAttribute(
      const parseagle::Attribute& a, MessageLogger& log);

  /**
   * @brief Try converting a list of attributes
   *
   * @note Attributes are only added to the output if #tryConvertAttribute()
   *       was able to convert them, and if no attribute with the same key
   *       exists yet (no duplicates).
   *
   * @param in          EAGLE attribute list
   * @param out         List where converted attributes are appended to
   * @param log         Message logger
   *
   * @see #tryConvertAttribute()
   */
  static void tryConvertAttributes(const QList<parseagle::Attribute>& in,
                                   AttributeList& out, MessageLogger& log);

  /**
   * @brief Try extracting MPN and manufacturer from a list of attributes
   *
   * @param attributes    Arbitrary attributes. Found MPN and manufacturer
   *                      attributes will be removed from it.
   * @param mpn           The found MPN (unmodified if not found)
   * @param manufacturer  The found manufacturer name (unmodified if not found)
   */
  static void tryExtractMpnAndManufacturer(AttributeList& attributes,
                                           SimpleString& mpn,
                                           SimpleString& manufacturer) noexcept;

  /**
   * @brief Try to convert a layer ID to a schematic layer
   *
   * @param id  EAGLE layer ID
   *
   * @return LibrePCB schematic/symbol layer (`nullptr` to discard object)
   */
  static const Layer* tryConvertSchematicLayer(int id) noexcept;

  /**
   * @brief Try to convert a layer ID to a board layer
   *
   * @param id  EAGLE layer ID
   *
   * @return LibrePCB board/footprint layer (`nullptr` to discard object)
   */
  static const Layer* tryConvertBoardLayer(int id) noexcept;

  /**
   * @brief Convert a layer setup string
   *
   * @param s   EAGLE layer setup string (e.g. "[2:1+((2*3)+(14*15))+16:15]")
   *
   * @return Map to move all inner copper layers to the top (remove gaps)
   */
  static QHash<const Layer*, const Layer*> convertLayerSetup(const QString& s);

  /**
   * @brief Convert an alignment
   *
   * @param a   EAGLE alignment
   *
   * @return LibrePCB alignment
   */
  static Alignment convertAlignment(parseagle::Alignment a);

  /**
   * @brief Convert a length
   *
   * @param l   EAGLE length [mm]
   *
   * @return LibrePCB length
   */
  static Length convertLength(double l);

  /**
   * @brief Convert a line width for a given layer
   *
   * Applies line width rules for some special purpose layers (like board
   * outline).
   *
   * @param w         EAGLE line width [mm]
   * @param layerId   EAGLE layer ID
   *
   * @return LibrePCB line width length
   */
  static UnsignedLength convertLineWidth(double w, int layerId);

  /**
   * @brief Convert a parameter value to a LibrePCB type
   *
   * @tparam  Return type, e.g. :librepcb::Length, ::librepcb::PositiveLength
   *          or ::librepcb::Ratio
   * @param p EAGLE parameter
   *
   * @return LibrePCB value
   *
   * @throws If the value could not be converted
   */
  template <typename T>
  static T convertParamTo(const parseagle::Param& p);

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
   * @brief Convert grid settings
   *
   * @note  In case of unknown/unsupported grid settings, output parameters
   *        are not overwritten.
   *
   * @param g         EAGLE grid settings
   * @param interval  LibrePCB grid interval (output)
   * @param unit      LibrePCB grid unit (output)
   */
  static void convertGrid(const parseagle::Grid& g, PositiveLength& interval,
                          LengthUnit& unit);

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
   * @brief Try to join and convert multiple wires to polygons
   *
   * @param wires                 EAGLE wires
   * @param isGrabAreaIfClosed    If true, grab area will be enabled on
   *                              closed polygons
   * @param log                   Logging message handler
   *
   * @return Joined polygons as intermediate geometries
   */
  static QList<Geometry> convertAndJoinWires(
      const QList<parseagle::Wire>& wires, bool isGrabAreaIfClosed,
      MessageLogger& log);

  /**
   * @brief Convert a rectangle
   *
   * @param r           EAGLE rectangle
   * @param isGrabArea  If the returned geometry should be a grab area
   *
   * @return Intermediate geometry containing 4 line segments
   */
  static Geometry convertRectangle(const parseagle::Rectangle& r,
                                   bool isGrabArea);

  /**
   * @brief Convert a polygon
   *
   * @param p           EAGLE polygon
   * @param isGrabArea  If the returned geometry should be a grab area
   *
   * @return Intermediate geometry (always closed)
   */
  static Geometry convertPolygon(const parseagle::Polygon& p, bool isGrabArea);

  /**
   * @brief Convert a circle
   *
   * @param c           EAGLE circle
   * @param isGrabArea  If the returned geometry should be a grab area
   *
   * @return Intermediate geometry
   */
  static Geometry convertCircle(const parseagle::Circle& c, bool isGrabArea);

  /**
   * @brief Convert a hole
   *
   * @param h   EAGLE hole
   *
   * @return LibrePCB hole
   */
  static std::shared_ptr<Hole> convertHole(const parseagle::Hole& h);

  /**
   * @brief Convert a frame
   *
   * @param f   EAGLE frame
   *
   * @return Intermediate geometry containing 4 line segments
   */
  static Geometry convertFrame(const parseagle::Frame& f);

  /**
   * @brief Convert a text value
   *
   * @param v   EAGLE text value (e.g. ">NAME")
   *
   * @return LibrePCB text value (e.g. "{{NAME}}")
   */
  static QString convertTextValue(const QString& v);

  /**
   * @brief Convert the size (height) of a schematic text
   *
   * @param s   EAGLE text size [mm]
   *
   * @return LibrePCB text size
   */
  static PositiveLength convertSchematicTextSize(double s);

  /**
   * @brief Try to convert a schematic/symbol text
   *
   * @param t   EAGLE text
   *
   * @return LibrePCB text if the layer is supported, otherwise `nullptr`
   */
  static std::shared_ptr<Text> tryConvertSchematicText(
      const parseagle::Text& t);

  /**
   * @brief Try to convert a schematic/symbol attribute text
   *
   * @param t   EAGLE attribute text
   *
   * @return LibrePCB text if the layer is supported, otherwise `nullptr`
   */
  static std::shared_ptr<Text> tryConvertSchematicAttribute(
      const parseagle::Attribute& t);

  /**
   * @brief Convert the size (height) of a board text
   *
   * @param layerId EAGLE layer ID
   * @param size    EAGLE text size [mm]
   *
   * @return LibrePCB text size
   */
  static PositiveLength convertBoardTextSize(int layerId, double size);

  /**
   * @brief Convert the stroke width of a board text
   *
   * @param layerId EAGLE layer ID
   * @param size    EAGLE text size [mm]
   * @param ratio   EAGLE text ratio [%]
   *
   * @return LibrePCB stroke text width
   */
  static UnsignedLength convertBoardTextStrokeWidth(int layerId, double size,
                                                    int ratio);

  /**
   * @brief Try to cnvert a board/footprint text
   *
   * @param t   EAGLE text
   *
   * @return LibrePCB text if the layer is supported, otherwise `nullptr`
   */
  static std::shared_ptr<StrokeText> tryConvertBoardText(
      const parseagle::Text& t);

  /**
   * @brief Try to convert a board/footprint attribute text
   *
   * @param t   EAGLE attribute text
   *
   * @return LibrePCB text if the layer is supported, otherwise `nullptr`
   */
  static std::shared_ptr<StrokeText> tryConvertBoardAttribute(
      const parseagle::Attribute& t);

  /**
   * @brief Convert a symbol pin
   *
   * @param p   EAGLE pin
   *
   * @return LibrePCB objects to represent the pin
   */
  static Pin convertSymbolPin(const parseagle::Pin& p);

  /**
   * @brief Convert a THT pad
   *
   * @param p                 EAGLE pad
   * @param autoAnnularWidth  How to calculate the annular width (and thus the
   *                          outer pad size) if it is set to 'auto' in EAGLE.
   *                          See #getDefaultAutoThtAnnularWidth().
   *
   * @return LibrePCB package pad + footprint pad
   */
  static std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad>>
      convertThtPad(const parseagle::ThtPad& p,
                    const BoundedUnsignedRatio& autoAnnularWidth);

  /**
   * @brief Convert an SMT pad
   *
   * @param p   EAGLE pad
   *
   * @return LibrePCB package pad + footprint pad
   */
  static std::pair<std::shared_ptr<PackagePad>, std::shared_ptr<FootprintPad>>
      convertSmtPad(const parseagle::SmtPad& p);

  /**
   * @brief Try to convert an intermediate geometry to a schematic circle
   *
   * @param g   intermediate geometry
   *
   * @return    A circle if the geometry represents a circle on a valid
   *            schematic layer, otherwise `nullptr`
   */
  static std::shared_ptr<Circle> tryConvertToSchematicCircle(const Geometry& g);

  /**
   * @brief Try to convert an intermediate geometry to a schematic polygon
   *
   * @param g   intermediate geometry
   *
   * @return A polygon if the layer is valid for schematics, otherwise `nullptr`
   */
  static std::shared_ptr<Polygon> tryConvertToSchematicPolygon(
      const Geometry& g);

  /**
   * @brief Convert the outline of a board zone
   *
   * Our zones do not support setting aline width, so we have to offet the
   * outline to get the same keepout area.
   *
   * @param outline   EAGLE keepout zone outline
   * @param lineWidth EAGLE keepout zone line width
   *
   * @return Possibly multiple paths with the LibrePCB zone outline(s)
   */
  static QVector<Path> convertBoardZoneOutline(const Path& outline,
                                               const Length& lineWidth);

  /**
   * @brief Try to convert an intermediate geometry to board keepout zones
   *
   * @param g   intermediate geometry
   *
   * @return    A keepout zone(s) if the geometry represents a zone,
   *            otherwise an empty vector
   */
  static QVector<std::shared_ptr<Zone>> tryConvertToBoardZones(
      const Geometry& g);

  /**
   * @brief Try to convert an intermediate geometry to a board circle
   *
   * @param g   intermediate geometry
   *
   * @return    A circle if the geometry represents a circle on a valid
   *            board layer, otherwise `nullptr`
   */
  static std::shared_ptr<Circle> tryConvertToBoardCircle(const Geometry& g);

  /**
   * @brief Try to convert an intermediate geometry to a board polygon
   *
   * @param g   intermediate geometry
   *
   * @return A polygon if the layer is valid for boards, otherwise `nullptr`
   */
  static std::shared_ptr<Polygon> tryConvertToBoardPolygon(const Geometry& g);

  /**
   * @brief Get the EAGLE layer name for a given layer ID
   *
   * @param id        EAGLE layer ID
   * @param fallback  The string to return if the given layer is unknown
   *
   * @return EAGLE layer name (fallback if unknown)
   */
  static QString getLayerName(int id,
                              const QString& fallback = "unknown") noexcept;

  /**
   * @brief Get the default annular width of THT pads with 'auto' size
   *
   * This is the value used by the EAGLE footprint editor.
   *
   * @return Ratio to calculate the annular width from the drill diameter
   */
  static BoundedUnsignedRatio getDefaultAutoThtAnnularWidth() noexcept;

  // Operator Overloadings
  EagleTypeConverter& operator=(const EagleTypeConverter& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace eagleimport
}  // namespace librepcb

#endif
