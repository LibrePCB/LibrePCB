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

#ifndef LIBREPCB_PROJECT_BI_NETPOINT_H
#define LIBREPCB_PROJECT_BI_NETPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../erc/if_ercmsgprovider.h"
#include "../graphicsitems/bgi_netpoint.h"
#include "./bi_netline.h"
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/junction.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Class BI_NetPoint
 ******************************************************************************/

/**
 * @brief The BI_NetPoint class
 */
class BI_NetPoint final : public BI_Base,
                          public BI_NetLineAnchor,
                          public SerializableObject,
                          public IF_ErcMsgProvider {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(BI_NetPoint)

public:
  // Constructors / Destructor
  BI_NetPoint() = delete;
  BI_NetPoint(const BI_NetPoint& other) = delete;
  BI_NetPoint(BI_NetSegment& segment, const SExpression& node,
              const Version& fileFormat);
  BI_NetPoint(BI_NetSegment& segment, const Point& position);
  ~BI_NetPoint() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mJunction.getUuid(); }
  const Junction& getJunction() const noexcept { return mJunction; }
  BI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  GraphicsLayer* getLayerOfLines() const noexcept;
  bool isSelectable() const noexcept override;
  TraceAnchor toTraceAnchor() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::NetPoint; }
  const Point& getPosition() const noexcept override {
    return mJunction.getPosition();
  }
  bool getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Inherited from BI_NetLineAnchor
  void registerNetLine(BI_NetLine& netline) override;
  void unregisterNetLine(BI_NetLine& netline) override;
  const QSet<BI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }

  // Operator Overloadings
  BI_NetPoint& operator=(const BI_NetPoint& rhs) = delete;
  bool operator==(const BI_NetPoint& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_NetPoint& rhs) noexcept { return (this != &rhs); }

private:
  void init();

  // General
  QScopedPointer<BGI_NetPoint> mGraphicsItem;
  QMetaObject::Connection mHighlightChangedConnection;

  // Attributes
  BI_NetSegment& mNetSegment;
  Junction mJunction;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;  ///< all registered netlines

  // ERC Messages
  /// @brief The ERC message for dead netpoints
  QScopedPointer<ErcMsg> mErcMsgDeadNetPoint;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
