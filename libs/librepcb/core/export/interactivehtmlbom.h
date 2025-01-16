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

#ifndef LIBREPCB_CORE_INTERACTIVEHTMLBOM_H
#define LIBREPCB_CORE_INTERACTIVEHTMLBOM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/padgeometry.h"
#include "../geometry/padhole.h"
#include "../geometry/path.h"
#include "../utils/rusthandle.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace rs {
struct InteractiveHtmlBom;
}

/*******************************************************************************
 *  Class InteractiveHtmlBom
 ******************************************************************************/

/**
 * @brief Zip file reader
 *
 * @note This is just a wrapper around its Rust implementation.
 */
class InteractiveHtmlBom final {
public:
  enum class ViewMode { BomOnly, LeftRight, TopBottom };
  enum class HighlightPin1Mode { None, Selected, All };
  enum class Layer { Top, Bottom };
  enum class Sides { Top, Bottom, Both };
  enum class DrawingKind { Polygon, ReferenceText, ValueText };
  enum class DrawingLayer {
    Edge,
    SilkscreenFront,
    SilkscreenBack,
    FabricationFront,
    FabricationBack,
  };
  struct Pad {
    bool onTop;
    bool onBottom;
    Point position;
    Angle rotation;
    bool mirrorGeometry;
    QList<PadGeometry> geometries;
    PadHoleList holes;
    std::optional<QString> netName;
    bool pin1;
  };

  InteractiveHtmlBom() = delete;
  InteractiveHtmlBom(const InteractiveHtmlBom& other) = delete;
  InteractiveHtmlBom& operator=(const InteractiveHtmlBom& rhs) = delete;

  /**
   * @brief Constructor
   *
   * @param title       Project title
   * @param company     Company/author name
   * @param revision    Project revision
   * @param date        Export date/time
   * @param topLeft     Top left coordinate of PCB bounding box
   * @param bottomRight Bottom right coordinate of PCB bounding box
   */
  InteractiveHtmlBom(const QString& title, const QString& company,
                     const QString& revision, const QString& date,
                     const Point& topLeft, const Point& bottomRight);

  /**
   * @brief Set view configuration
   *
   * @param mode          View mode
   * @param highlightPin1 Highlight pin-1 mode
   * @param dark          Dark or not
   */
  void setViewConfig(ViewMode mode, HighlightPin1Mode highlightPin1,
                     bool dark) noexcept;

  /**
   * @brief Set board rotation
   *
   * @param angle       Rotation angle
   * @param offsetBack  Offset back side or not
   */
  void setBoardRotation(const Angle& angle, bool offsetBack) noexcept;

  /**
   * @brief Set silkscreen visibility
   *
   * @param show  Show or not
   */
  void setShowSilkscreen(bool show) noexcept;

  /**
   * @brief Set fabrication visibility
   *
   * @param show  Show or not
   */
  void setShowFabrication(bool show) noexcept;

  /**
   * @brief Set pads visibility
   *
   * @param show  Show or not
   */
  void setShowPads(bool show) noexcept;

  /**
   * @brief Set the BOM checkbox columns
   *
   * @param names   Checkbox names
   */
  void setCheckBoxes(const QStringList& names) noexcept;

  /**
   * @brief Set the fields of BOM lines
   *
   * @param fields  Field names
   */
  void setFields(const QStringList& fields) noexcept;

  /**
   * @brief Add a PCB drawing
   *
   * @param kind    Drawing kind
   * @param layer   Drawing layer
   * @param path    Drawing path
   * @param width   Line width
   * @param filled  Fill or not the shape
   */
  void addDrawing(DrawingKind kind, DrawingLayer layer, const Path& path,
                  const UnsignedLength& width, bool filled) noexcept;

  /**
   * @brief Add a track
   *
   * @param layer   Layer
   * @param start   Start position
   * @param end     End position
   * @param width   Width
   * @param netName Net name (if any)
   */
  void addTrack(Layer layer, const Point& start, const Point& end,
                const PositiveLength& width,
                const std::optional<QString>& netName) noexcept;

  /**
   * @brief Add via
   *
   * @param layers        Layers this via exists on
   * @param pos           Position
   * @param diameter      Outer diameter
   * @param drillDiameter Drill diameter
   * @param netName       net name (if any)
   */
  void addVia(QSet<Layer> layers, const Point& pos,
              const PositiveLength& diameter,
              const PositiveLength& drillDiameter,
              const std::optional<QString>& netName) noexcept;

  /**
   * @brief Add plane fragment
   *
   * @param layer         Layer
   * @param outline       Outline
   * @param netName       Net name (if any)
   */
  void addPlaneFragment(Layer layer, const Path& outline,
                        const std::optional<QString>& netName) noexcept;

  /**
   * @brief Add footprint
   *
   * @param layer         Mount layer
   * @param pos           Position
   * @param rot           Rotation
   * @param topLeft       Top left of bounding box
   * @param bottomRight   Bottom right of bounding box
   * @param mount         Whether it will be mounted or not
   * @param fields        Field values
   * @param pads          Pads
   * @return Footprint ID
   */
  std::size_t addFootprint(Layer layer, const Point& pos, const Angle& rot,
                           const Point& topLeft, const Point& bottomRight,
                           bool mount, const QStringList& fields,
                           const QList<Pad>& pads) noexcept;

  /**
   * @brief Add a BOM row
   *
   * @param sides   BOM sides
   * @param parts   Parts (designators & footprint IDs)
   */
  void addBomRow(Sides sides,
                 const QList<std::pair<QString, std::size_t>>& parts) noexcept;

  /**
   * @brief Generate the HTML
   *
   * @return HTML file content
   *
   * @throws If some of the data is invalid
   */
  QString generateHtml() const;

private:
  RustHandle<rs::InteractiveHtmlBom> mHandle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
