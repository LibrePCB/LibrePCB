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

#ifndef LIBREPCB_CORE_BI_STROKETEXT_H
#define LIBREPCB_CORE_BI_STROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/stroketext.h"
#include "../../../utils/signalslot.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeProvider;
class BI_Device;
class Board;

/*******************************************************************************
 *  Class BI_StrokeText
 ******************************************************************************/

/**
 * @brief The BI_StrokeText class
 */
class BI_StrokeText final : public BI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    LayerNameChanged,
    StrokeWidthChanged,
    TextChanged,
    PathsChanged,
  };
  Signal<BI_StrokeText, Event> onEdited;
  typedef Slot<BI_StrokeText, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_StrokeText() = delete;
  BI_StrokeText(const BI_StrokeText& other) = delete;
  BI_StrokeText(Board& board, const StrokeText& text);
  ~BI_StrokeText() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept;  // for convenience, e.g. template usage
  const Point& getPosition() const noexcept;
  const Angle& getRotation() const noexcept;
  bool getMirrored() const noexcept;
  const QString& getText() const noexcept { return mText; }
  StrokeText& getTextObj() noexcept { return *mTextObj; }
  const StrokeText& getTextObj() const noexcept { return *mTextObj; }
  const StrokeFont& getFont() const noexcept { return mFont; }
  const QVector<Path>& getPaths() const noexcept { return mPaths; }

  // General Methods
  BI_Device* getDevice() const noexcept { return mDevice; }
  void setDevice(BI_Device* device) noexcept;
  const AttributeProvider* getAttributeProvider() const noexcept;
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_StrokeText& operator=(const BI_StrokeText& rhs) = delete;

private:  // Methods
  void strokeTextEdited(const StrokeText& text,
                        StrokeText::Event event) noexcept;
  void updateText() noexcept;
  void updatePaths() noexcept;

private:  // Data
  BI_Device* mDevice;
  QScopedPointer<StrokeText> mTextObj;
  const StrokeFont& mFont;

  // Cached Attributes
  QString mText;
  QVector<Path> mPaths;

  // Slots
  StrokeText::OnEditedSlot mOnStrokeTextEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
