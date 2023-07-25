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

#ifndef LIBREPCB_CORE_BOMGENERATOR_H
#define LIBREPCB_CORE_BOMGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Bom;
class Project;
class Uuid;

/*******************************************************************************
 *  Class BomGenerator
 ******************************************************************************/

/**
 * @brief The BomGenerator class
 */
class BomGenerator final {
  Q_DECLARE_TR_FUNCTIONS(BoardBomGenerator)

public:
  // Constructors / Destructor
  BomGenerator() = delete;
  BomGenerator(const BomGenerator& other) = delete;
  explicit BomGenerator(const Project& project) noexcept;
  ~BomGenerator() noexcept;

  // Setters
  void setAdditionalAttributes(const QStringList& attributes) noexcept {
    mAdditionalAttributes = attributes;
  }

  // General Methods
  std::shared_ptr<Bom> generate(const Board* board,
                                const Uuid& assemblyVariant) noexcept;

  // Operator Overloadings
  BomGenerator& operator=(const BomGenerator& rhs) = delete;

private:  // Methods
  static void removeSubString(QString& str, const QString& substr) noexcept;

private:  // Data
  const Project& mProject;
  QStringList mAdditionalAttributes;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
