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

#ifndef LIBREPCB_CORE_PROJECTJSONEXPORT_H
#define LIBREPCB_CORE_PROJECTJSONEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/point.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AssemblyVariant;
class Board;
class PcbColor;
class Project;

/*******************************************************************************
 *  Class ProjectJsonExport
 ******************************************************************************/

/**
 * @brief Project data export to JSON
 *
 * @note  To be extended with new JSON nodes as needed, but increment the
 *        version number on each change and keep it backwards compatible
 *        within each major release of LibrePCB!
 */
class ProjectJsonExport final {
public:
  // Types
  struct BoundingBox {
    tl::optional<std::pair<Point, Point>> points;
  };
  struct ToolList {
    QList<Length> diameters;
  };

  // Constructors / Destructor
  ProjectJsonExport() noexcept;
  ProjectJsonExport(const ProjectJsonExport& other) = delete;
  ~ProjectJsonExport() noexcept;

  // General Methods
  QJsonArray toJson(const QStringList& obj) const;
  QJsonValue toJson(const Length& obj) const;
  QJsonValue toJson(const tl::optional<Length>& obj) const;
  QJsonArray toJson(const QSet<Length>& obj) const;
  QJsonValue toJson(const PcbColor* obj) const;
  QJsonObject toJson(const AssemblyVariant& obj) const;
  QJsonValue toJson(const BoundingBox& obj) const;
  QJsonObject toJson(const ToolList& obj) const;
  QJsonObject toJson(const Board& obj) const;
  QJsonObject toJson(const Project& obj) const;
  QByteArray toUtf8(const Project& obj) const;

  // Operator Overloadings
  ProjectJsonExport& operator=(const ProjectJsonExport& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
