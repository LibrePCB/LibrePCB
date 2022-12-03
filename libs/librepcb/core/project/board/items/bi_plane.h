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
#include "../../../graphics/graphicslayername.h"
#include "../../../types/uuid.h"
#include "bi_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BGI_Plane;
class Board;
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
  // Types
  enum class ConnectStyle {
    None,  ///< do not connect pads/vias to plane
    // Thermal,    ///< add thermals to connect pads/vias to plane
    Solid,  ///< completely connect pads/vias to plane
  };

  // Constructors / Destructor
  BI_Plane() = delete;
  BI_Plane(const BI_Plane& other) = delete;
  BI_Plane(Board& board, const Uuid& uuid, const GraphicsLayerName& layerName,
           NetSignal& netsignal, const Path& outline);
  ~BI_Plane() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayerName() const noexcept { return mLayerName; }
  NetSignal& getNetSignal() const noexcept { return *mNetSignal; }
  const UnsignedLength& getMinWidth() const noexcept { return mMinWidth; }
  const UnsignedLength& getMinClearance() const noexcept {
    return mMinClearance;
  }
  bool getKeepOrphans() const noexcept { return mKeepOrphans; }
  int getPriority() const noexcept { return mPriority; }
  ConnectStyle getConnectStyle() const noexcept { return mConnectStyle; }
  // const Length& getThermalGapWidth() const noexcept {return
  // mThermalGapWidth;} const Length& getThermalSpokeWidth() const noexcept
  // {return mThermalSpokeWidth;}
  const Path& getOutline() const noexcept { return mOutline; }
  const QVector<Path>& getFragments() const noexcept { return mFragments; }
  BGI_Plane& getGraphicsItem() noexcept { return *mGraphicsItem; }
  bool isSelectable() const noexcept override;
  bool isVisible() const noexcept { return mIsVisible; }

  // Setters
  void setOutline(const Path& outline) noexcept;
  void setLayerName(const GraphicsLayerName& layerName) noexcept;
  void setNetSignal(NetSignal& netsignal);
  void setMinWidth(const UnsignedLength& minWidth) noexcept;
  void setMinClearance(const UnsignedLength& minClearance) noexcept;
  void setConnectStyle(ConnectStyle style) noexcept;
  void setPriority(int priority) noexcept;
  void setKeepOrphans(bool keepOrphans) noexcept;
  void setVisible(bool visible) noexcept;
  void setCalculatedFragments(const QVector<Path>& fragments) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;
  void clear() noexcept;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::Plane; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_Plane& operator=(const BI_Plane& rhs) = delete;
  bool operator<(const BI_Plane& rhs) const noexcept;

private slots:
  void boardAttributesChanged();

private:  // Data
  Uuid mUuid;
  GraphicsLayerName mLayerName;
  NetSignal* mNetSignal;
  Path mOutline;
  UnsignedLength mMinWidth;
  UnsignedLength mMinClearance;
  bool mKeepOrphans;
  int mPriority;
  ConnectStyle mConnectStyle;
  // Length mThermalGapWidth;
  // Length mThermalSpokeWidth;
  // style [round square miter] ?
  QScopedPointer<BGI_Plane> mGraphicsItem;
  bool mIsVisible;  // volatile, not saved to file

  QVector<Path> mFragments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
