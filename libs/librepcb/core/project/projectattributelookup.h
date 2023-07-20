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

#ifndef LIBREPCB_CORE_PROJECTATTRIBUTELOOKUP_H
#define LIBREPCB_CORE_PROJECTATTRIBUTELOOKUP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <functional>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Board;
class ComponentInstance;
class Part;
class Project;
class SI_Symbol;
class Schematic;

/*******************************************************************************
 *  Class ProjectAttributeLookup
 ******************************************************************************/

/**
 * @brief Determine an attribute value of an object within a ::librepcb::Project
 *
 * Provides access to built-in and user-defined attributes
 * (::librepcb::Attribute) of objects within a project (e.g. symbols).
 *
 * Please read the documentation about the @ref doc_attributes_system to get an
 * idea how the @ref doc_attributes_system works in detail.
 *
 * Usage:
 *
 *   1. Call the constructor with passing the object to query attributes from.
 *   2. Call #operator()() to get the value of a specific attribute key.
 *
 * @see ::librepcb::AttributeSubstitutor
 * @see @ref doc_attributes_system
 */
class ProjectAttributeLookup final {
  typedef std::function<QString(const QString&)> LookupFunction;

public:
  // Constructors / Destructor
  ProjectAttributeLookup() = delete;
  ProjectAttributeLookup(const ProjectAttributeLookup& other) noexcept;
  explicit ProjectAttributeLookup(const Project& obj) noexcept;
  ProjectAttributeLookup(const ComponentInstance& obj,
                         QPointer<const BI_Device> device,
                         std::shared_ptr<const Part> part) noexcept;
  explicit ProjectAttributeLookup(const Schematic& obj) noexcept;
  explicit ProjectAttributeLookup(const Board& obj) noexcept;
  ProjectAttributeLookup(const SI_Symbol& obj, QPointer<const BI_Device> device,
                         std::shared_ptr<const Part> part) noexcept;
  ProjectAttributeLookup(const BI_Device& obj,
                         std::shared_ptr<const Part> part) noexcept;
  ~ProjectAttributeLookup() noexcept;

  // Operator Overloadings

  /**
   * @brief Get the value of a specific attribute key
   *
   * @param key   Attribute key (built-in or user-defined)
   *
   * @return Attribute value. A null (empty) QString is returned if the
   *         requested attribute does not exist.
   */
  QString operator()(const QString& key) const noexcept;

  ProjectAttributeLookup& operator=(const ProjectAttributeLookup& rhs) noexcept;

private:  // Methods
  static bool query(const Project& project, const QString& key,
                    QString& value) noexcept;
  static bool query(const ComponentInstance& cmp, const QString& key,
                    QString& value) noexcept;
  static bool query(const Schematic& schematic, const QString& key,
                    QString& value) noexcept;
  static bool query(const Board& board, const QString& key,
                    QString& value) noexcept;
  static bool query(const SI_Symbol& symbol, const QString& key,
                    QString& value) noexcept;
  static bool query(const BI_Device& device, const QString& key,
                    QString& value) noexcept;
  static bool query(const Part& part, const QString& key,
                    QString& value) noexcept;

private:  // Data
  LookupFunction mFunction;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
