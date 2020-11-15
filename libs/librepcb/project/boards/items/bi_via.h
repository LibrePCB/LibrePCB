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

#ifndef LIBREPCB_PROJECT_BI_VIA_H
#define LIBREPCB_PROJECT_BI_VIA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphicsitems/bgi_via.h"
#include "./bi_netline.h"
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/geometry/via.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Class BI_Via
 ******************************************************************************/

/**
 * @brief The BI_Via class
 */
class BI_Via final : public BI_Base,
                     public BI_NetLineAnchor,
                     public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_Via() = delete;
  BI_Via(const BI_Via& other) = delete;
  BI_Via(BI_NetSegment& netsegment, const BI_Via& other);
  BI_Via(BI_NetSegment& netsegment, const Via& via);
  BI_Via(BI_NetSegment& netsegment, const SExpression& node,
         const Version& fileFormat);
  ~BI_Via() noexcept;

  // Getters
  BI_NetSegment& getNetSegment() const noexcept { return mNetSegment; }
  NetSignal& getNetSignalOfNetSegment() const noexcept;
  const Via& getVia() const noexcept { return mVia; }
  const Uuid& getUuid() const noexcept { return mVia.getUuid(); }
  Via::Shape getShape() const noexcept { return mVia.getShape(); }
  const PositiveLength& getDrillDiameter() const noexcept {
    return mVia.getDrillDiameter();
  }
  const PositiveLength& getSize() const noexcept { return mVia.getSize(); }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  bool isOnLayer(const QString& layerName) const noexcept;
  bool isSelectable() const noexcept override;
  TraceAnchor toTraceAnchor() const noexcept override;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setShape(Via::Shape shape) noexcept;
  void setSize(const PositiveLength& size) noexcept;
  void setDrillDiameter(const PositiveLength& diameter) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::Via; }
  const Point& getPosition() const noexcept override {
    return mVia.getPosition();
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
  BI_Via& operator=(const BI_Via& rhs) = delete;
  bool operator==(const BI_Via& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_Via& rhs) noexcept { return (this != &rhs); }

private:
  void init();
  void boardOrNetAttributesChanged();

  // General
  Via mVia;
  BI_NetSegment& mNetSegment;
  QScopedPointer<BGI_Via> mGraphicsItem;
  QMetaObject::Connection mHighlightChangedConnection;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BI_VIA_H
