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

#ifndef LIBREPCB_AUTOMATION_TOOLHELPERS_H
#define LIBREPCB_AUTOMATION_TOOLHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <functional>
#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class ComponentInstance;
class Footprint;
class Project;
class Transform;
class WorkspaceLibraryDb;

namespace automation {

/*******************************************************************************
 *  Argument Parsing
 ******************************************************************************/

/**
 * @brief Helpers to parse tool arguments and to build JSON schemas
 *
 * All parse functions throw ::librepcb::RuntimeError with a message suitable
 * for the MCP client if an argument is missing or invalid.
 */
QString requiredString(const QJsonObject& args, const QString& key);
Length lengthMm(const QJsonValue& value, const QString& key);
Point pointMm(const QJsonValue& value, const QString& key);
Angle angleDeg(const QJsonValue& value, const QString& key);
Path closedPathMm(const QJsonValue& value, const QString& key);
QJsonArray toJson(const Point& p) noexcept;

/**
 * @brief Vertices of a path as [x, y] or [x, y, arc angle], without the
 *        closing vertex
 */
QJsonArray verticesToJson(const Path& path) noexcept;
QJsonArray zoneRulesToJson(Zone::Rules rules) noexcept;
QStringList zoneRuleNames() noexcept;
Zone::Rules zoneRules(const QJsonValue& value);

/**
 * @brief Copper layers of a footprint keep-out ('top', 'inner', 'bottom'),
 *        swapped for parts on the bottom side
 */
QJsonArray footprintZoneLayers(const Zone& zone, bool mirrored) noexcept;

/**
 * @brief Courtyard, keep-outs and holes of a footprint, transformed
 */
QJsonObject footprintGeometry(const Footprint& footprint,
                              const Transform& transform) noexcept;
QJsonObject toJson(const RuleCheckMessageList& messages,
                   const QSet<SExpression>& approvals) noexcept;

/**
 * @brief Center and size of the bounding box of an area, in mm
 */
QJsonObject locationToJson(const Path& location) noexcept;

/**
 * @brief Stable ID of a rule check message, derived from its approval
 */
QString messageId(const RuleCheckMessage& msg) noexcept;

/**
 * @brief Approve or unapprove rule check messages by their IDs
 *
 * @return Number of messages whose approval state changed
 */
int approveMessages(const RuleCheckMessageList& messages,
                    const QSet<SExpression>& approvals, const QJsonValue& ids,
                    bool approved,
                    const std::function<void(const SExpression&, bool)>& set);
/**
 * @brief Path given by a client, which must be absolute since the server's
 *        working directory is unknown to the client
 */
FilePath absoluteFilePath(const QString& path);

QJsonObject objectSchema(const QJsonObject& properties,
                         const QStringList& required = {}) noexcept;
QJsonObject stringProp(const QString& description) noexcept;
QJsonObject numberProp(const QString& description) noexcept;
QJsonObject boolProp(const QString& description) noexcept;
QJsonObject pointProp(const QString& description) noexcept;
QJsonObject verticesProp(const QString& description) noexcept;
QJsonObject enumProp(const QStringList& values,
                     const QString& description) noexcept;
QJsonObject arrayProp(const QJsonObject& items,
                      const QString& description) noexcept;

/*******************************************************************************
 *  Lookups
 ******************************************************************************/

/**
 * @brief Find workspace library elements whose name contains a keyword
 */
template <typename ElementType>
QList<Uuid> findLibraryElements(const WorkspaceLibraryDb& db,
                                const QString& keyword);

/**
 * @brief Find a workspace library element by UUID or by its exact name
 *
 * @return UUID and directory of the latest version of the element
 */
template <typename ElementType>
std::pair<Uuid, FilePath> findLibraryElement(const WorkspaceLibraryDb& db,
                                             const QString& nameOrUuid);

template <typename ElementType>
std::unique_ptr<ElementType> openElement(const FilePath& fp) {
  return ElementType::open(std::make_unique<TransactionalDirectory>(
      TransactionalFileSystem::openRO(fp)));  // can throw
}

/**
 * @brief The value of a part, using the part number of its first assembly
 *        option (as for `{{MPN or DEVICE}}`)
 */
QString partValue(const ComponentInstance& cmp) noexcept;

ComponentInstance& findComponentInstance(Project& project,
                                         const QString& designator);

void rebuildPlanes(Board& board);

/**
 * @brief Split a pin reference like "U1.VCC" into designator and pin name
 */
std::pair<QString, QString> splitPinRef(const QString& ref);

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb

#endif
