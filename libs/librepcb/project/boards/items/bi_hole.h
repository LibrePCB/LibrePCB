/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_BI_HOLE_H
#define LIBREPCB_PROJECT_BI_HOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/hole.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class HoleGraphicsItem;

namespace project {

class Project;
class Board;

/*******************************************************************************
 *  Class BI_Hole
 ******************************************************************************/

/**
 * @brief The BI_Hole class
 */
class BI_Hole final : public BI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_Hole()                     = delete;
  BI_Hole(const BI_Hole& other) = delete;
  BI_Hole(Board& board, const BI_Hole& other);
  BI_Hole(Board& board, const SExpression& node);
  BI_Hole(Board& board, const Hole& hole);
  ~BI_Hole() noexcept;

  // Getters
  Hole&       getHole() noexcept { return *mHole; }
  const Hole& getHole() const noexcept { return *mHole; }
  const Uuid& getUuid() const
      noexcept;  // convenience function, e.g. for template usage
  bool isSelectable() const noexcept override;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::Hole; }
  const Point& getPosition() const noexcept override;
  bool         getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void         setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_Hole& operator=(const BI_Hole& rhs) = delete;

private:  // Methods
  void init();

private:  // Data
  QScopedPointer<Hole>             mHole;
  QScopedPointer<HoleGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BI_HOLE_H
