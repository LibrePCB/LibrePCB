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

#ifndef LIBREPCB_CORE_BI_PLANE_H
#define LIBREPCB_CORE_BI_PLANE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../exceptions.h"
#include "../../../geometry/path.h"
#include "../../../types/uuid.h"
#include "bi_base.h"

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Layer;
class NetSignal;
class Project;

/*******************************************************************************
 *  Class BI_Plane
 ******************************************************************************/

/**
 * @brief The BI_Plane class
 */
class BI_Plane final : public BI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    OutlineChanged,
    LayerChanged,
    IsLockedChanged,
    VisibilityChanged,
    FragmentsChanged,
  };
  Signal<BI_Plane, Event> onEdited;
  typedef Slot<BI_Plane, Event> OnEditedSlot;

  // Types
  enum class ConnectStyle {
    None,  ///< Do not connect pads to plane
    ThermalRelief,  ///< Add thermal spokes to connect pads to plane
    Solid,  ///< Completely connect pads to plane
  };

  // Constructors / Destructor
  BI_Plane() = delete;
  BI_Plane(const BI_Plane& other) = delete;
  BI_Plane(Board& board, const Uuid& uuid, const Layer& layer,
           NetSignal* netsignal, const Path& outline);
  ~BI_Plane() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getLayer() const noexcept { return *mLayer; }
  NetSignal* getNetSignal() const noexcept { return mNetSignal; }
  const UnsignedLength& getMinWidth() const noexcept { return mMinWidth; }
  const UnsignedLength& getMinClearanceToCopper() const noexcept {
    return mMinClearanceToCopper;
  }
  const UnsignedLength& getMinClearanceToBoard() const noexcept {
    return mMinClearanceToBoard;
  }
  const UnsignedLength& getMinClearanceToNpth() const noexcept {
    return mMinClearanceToNpth;
  }
  bool getKeepIslands() const noexcept { return mKeepIslands; }
  int getPriority() const noexcept { return mPriority; }
  ConnectStyle getConnectStyle() const noexcept { return mConnectStyle; }
  const PositiveLength& getThermalGap() const noexcept { return mThermalGap; }
  const PositiveLength& getThermalSpokeWidth() const noexcept {
    return mThermalSpokeWidth;
  }
  const Path& getOutline() const noexcept { return mOutline; }
  const QVector<Path>& getFragments() const noexcept { return mFragments; }
  bool isLocked() const noexcept { return mLocked; }
  bool isVisible() const noexcept { return mIsVisible; }

  // Setters
  void setOutline(const Path& outline) noexcept;
  void setLayer(const Layer& layer) noexcept;
  void setNetSignal(NetSignal* netsignal);
  void setMinWidth(const UnsignedLength& minWidth) noexcept;
  void setMinClearanceToCopper(const UnsignedLength& minClearance) noexcept;
  void setMinClearanceToBoard(const UnsignedLength& minClearance) noexcept;
  void setMinClearanceToNpth(const UnsignedLength& minClearance) noexcept;
  void setConnectStyle(ConnectStyle style) noexcept;
  void setThermalGap(const PositiveLength& gap) noexcept;
  void setThermalSpokeWidth(const PositiveLength& width) noexcept;
  void setPriority(int priority) noexcept;
  void setKeepIslands(bool keep) noexcept;
  void setLocked(bool locked) noexcept;
  void setVisible(bool visible) noexcept;
  void setCalculatedFragments(const QVector<Path>& fragments) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BI_Plane& operator=(const BI_Plane& rhs) = delete;

private:  // Data
  Uuid mUuid;
  const Layer* mLayer;  ///< Mandatory (never `nullptr`)
  NetSignal* mNetSignal;  ///< Optional (`nullptr` = no net)
  Path mOutline;
  UnsignedLength mMinWidth;
  UnsignedLength mMinClearanceToCopper;
  UnsignedLength mMinClearanceToBoard;
  UnsignedLength mMinClearanceToNpth;
  bool mKeepIslands;
  int mPriority;
  ConnectStyle mConnectStyle;
  PositiveLength mThermalGap;
  PositiveLength mThermalSpokeWidth;
  bool mLocked;
  bool mIsVisible;  // volatile, not saved to file

  QVector<Path> mFragments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
