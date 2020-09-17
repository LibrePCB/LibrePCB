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

#ifndef LIBREPCB_PROJECT_BI_STROKETEXT_H
#define LIBREPCB_PROJECT_BI_STROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class StrokeTextGraphicsItem;
class LineGraphicsItem;

namespace project {

class Project;
class Board;
class BI_Footprint;

/*******************************************************************************
 *  Class BI_StrokeText
 ******************************************************************************/

/**
 * @brief The BI_StrokeText class
 */
class BI_StrokeText final : public BI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_StrokeText() = delete;
  BI_StrokeText(const BI_StrokeText& other) = delete;
  BI_StrokeText(Board& board, const BI_StrokeText& other);
  BI_StrokeText(Board& board, const SExpression& node);
  BI_StrokeText(Board& board, const StrokeText& text);
  ~BI_StrokeText() noexcept;

  // Getters
  StrokeText& getText() noexcept { return *mText; }
  const StrokeText& getText() const noexcept { return *mText; }
  const Uuid& getUuid() const
      noexcept;  // convenience function, e.g. for template usage
  bool isSelectable() const noexcept override;

  // General Methods
  BI_Footprint* getFootprint() const noexcept { return mFootprint; }
  void setFootprint(BI_Footprint* footprint) noexcept;
  void updateGraphicsItems() noexcept;
  void addToBoard() override;
  void removeFromBoard() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override {
    return BI_Base::Type_t::StrokeText;
  }
  const Point& getPosition() const noexcept override;
  bool getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_StrokeText& operator=(const BI_StrokeText& rhs) = delete;

private slots:
  void boardOrFootprintAttributesChanged();

private:  // Methods
  void init();
  void updatePaths() noexcept;
  void strokeTextEdited(const StrokeText& text,
                        StrokeText::Event event) noexcept;

private:  // Data
  BI_Footprint* mFootprint;
  QScopedPointer<StrokeText> mText;
  QScopedPointer<StrokeTextGraphicsItem> mGraphicsItem;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Slots
  StrokeText::OnEditedSlot mOnStrokeTextEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BI_STROKETEXT_H
