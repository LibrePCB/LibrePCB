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
  enum class Layer { Top, Bottom };
  enum class Sides { Top, Bottom, Both };
  struct Pad {
    bool onTop;
    bool onBottom;
    Point position;
    Angle rotation;
    bool mirrorGeometry;
    QList<PadGeometry> geometries;
    PadHoleList holes;
    std::optional<QString> netName;
  };

  InteractiveHtmlBom() = delete;
  InteractiveHtmlBom(const InteractiveHtmlBom& other) = delete;
  InteractiveHtmlBom& operator=(const InteractiveHtmlBom& rhs) = delete;

  /**
   * @brief Constructor
   */
  InteractiveHtmlBom(const QString& title, const QString& revision,
                     const QString& company, const QString& date,
                     const Length& minX, const Length& maxX, const Length& minY,
                     const Length& maxY);

  /**
   * @brief Add PCB edge path
   *
   * @param path  Drawing path
   */
  void addEdge(const Path& path) noexcept;

  /**
   * @brief Add legend on top
   *
   * @param path  Drawing path
   * @param width Line width
   */
  void addLegendTop(const Path& path, const UnsignedLength& width,
                    bool fill) noexcept;

  /**
   * @brief Add legend on bottom
   *
   * @param path  Drawing path
   * @param width Line width
   */
  void addLegendBot(const Path& path, const UnsignedLength& width,
                    bool fill) noexcept;

  /**
   * @brief Add documentation on top
   *
   * @param path  Drawing path
   * @param width Line width
   */
  void addDocumentationTop(const Path& path, const UnsignedLength& width,
                           bool fill) noexcept;

  /**
   * @brief Add documentation on bottom
   *
   * @param path  Drawing path
   * @param width Line width
   */
  void addDocumentationBot(const Path& path, const UnsignedLength& width,
                           bool fill) noexcept;

  std::size_t addFootprint(const QString& name, bool mirror, const Point& pos,
                           const Angle& rot, const Length& minX,
                           const Length& maxX, const Length& minY,
                           const Length& maxY, bool mount,
                           const QList<Pad>& pads) noexcept;

  void addBomRow(Sides sides,
                 const QList<std::pair<QString, std::size_t>>& parts) noexcept;

  void addTrack(Layer layer, const Point& start, const Point& end,
                const PositiveLength& width,
                const std::optional<QString>& netName) noexcept;

  void addVia(Layer layer, const Point& pos, const PositiveLength& diameter,
              const PositiveLength& drillDiameter,
              const std::optional<QString>& netName) noexcept;

  void addPlaneFragment(Layer layer, const Path& fragment,
                        const std::optional<QString>& netName) noexcept;

  /**
   * @brief Generate the HTML
   *
   * @return HTML file content
   */
  QString generate() const noexcept;

private:
  RustHandle<rs::InteractiveHtmlBom> mHandle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
