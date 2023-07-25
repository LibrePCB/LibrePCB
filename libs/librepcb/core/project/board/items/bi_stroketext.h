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
#include "../../../utils/signalslot.h"
#include "../boardstroketextdata.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Board;
class Path;
class StrokeFont;

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
    LayerChanged,
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    StrokeWidthChanged,
    PathsChanged,
  };
  Signal<BI_StrokeText, Event> onEdited;
  typedef Slot<BI_StrokeText, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_StrokeText() = delete;
  BI_StrokeText(const BI_StrokeText& other) = delete;
  BI_StrokeText(Board& board, const BoardStrokeTextData& data);
  ~BI_StrokeText() noexcept;

  // Getters
  const BoardStrokeTextData& getData() const noexcept { return mData; }
  const StrokeFont& getFont() const noexcept { return mFont; }
  const QString& getSubstitutedText() const noexcept {
    return mSubstitutedText;
  }
  const QVector<Path>& getPaths() const noexcept { return mPaths; }

  // Setters
  bool setLayer(const Layer& layer) noexcept;
  bool setText(const QString& text) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setStrokeWidth(const UnsignedLength& strokeWidth) noexcept;
  bool setLetterSpacing(const StrokeTextSpacing& spacing) noexcept;
  bool setLineSpacing(const StrokeTextSpacing& spacing) noexcept;
  bool setAlign(const Alignment& align) noexcept;
  bool setMirrored(bool mirrored) noexcept;
  bool setAutoRotate(bool autoRotate) noexcept;
  bool setLocked(bool locked) noexcept;

  // General Methods
  BI_Device* getDevice() const noexcept { return mDevice; }
  void setDevice(BI_Device* device) noexcept;
  void addToBoard() override;
  void removeFromBoard() override;

  // Operator Overloadings
  BI_StrokeText& operator=(const BI_StrokeText& rhs) = delete;

private:  // Methods
  void updateText() noexcept;
  void updatePaths() noexcept;
  void invalidatePlanes(const Layer& layer) noexcept;

private:  // Data
  BoardStrokeTextData mData;
  const StrokeFont& mFont;
  BI_Device* mDevice;

  // Cached Attributes
  QString mSubstitutedText;
  QVector<Path> mPaths;  ///< Without transformation (position/rotation/mirror)
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
