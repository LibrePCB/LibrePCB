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

#ifndef LIBREPCB_PROJECT_BI_NETLINE_H
#define LIBREPCB_PROJECT_BI_NETLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphicsitems/bgi_netline.h"
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class NetSignal;
class BI_NetSegment;

/*******************************************************************************
 *  Class BI_NetLineAnchor
 ******************************************************************************/

class BI_NetLineAnchor {
public:
  BI_NetLineAnchor() noexcept          = default;
  virtual ~BI_NetLineAnchor() noexcept = default;

  virtual void                     registerNetLine(BI_NetLine& netline)   = 0;
  virtual void                     unregisterNetLine(BI_NetLine& netline) = 0;
  virtual const QSet<BI_NetLine*>& getNetLines() const noexcept           = 0;
  virtual const Point&             getPosition() const noexcept           = 0;

  std::vector<PositiveLength> getLineWidths() const noexcept;
  UnsignedLength              getMaxLineWidth() const noexcept;
  UnsignedLength              getMedianLineWidth() const noexcept;
  BI_NetSegment*              getNetSegmentOfLines() const noexcept;
};

/*******************************************************************************
 *  Class BI_NetLine
 ******************************************************************************/

/**
 * @brief The BI_NetLine class
 */
class BI_NetLine final : public BI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_NetLine()                        = delete;
  BI_NetLine(const BI_NetLine& other) = delete;
  BI_NetLine(BI_NetSegment& segment, const BI_NetLine& other,
             BI_NetLineAnchor& startPoint, BI_NetLineAnchor& endPoint);
  BI_NetLine(BI_NetSegment& segment, const SExpression& node);
  BI_NetLine(BI_NetSegment& segment, BI_NetLineAnchor& startPoint,
             BI_NetLineAnchor& endPoint, GraphicsLayer& layer,
             const PositiveLength& width);
  ~BI_NetLine() noexcept;

  // Getters
  BI_NetSegment&        getNetSegment() const noexcept { return mNetSegment; }
  const Uuid&           getUuid() const noexcept { return mUuid; }
  GraphicsLayer&        getLayer() const noexcept { return *mLayer; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  BI_NetLineAnchor&     getStartPoint() const noexcept { return *mStartPoint; }
  BI_NetLineAnchor&     getEndPoint() const noexcept { return *mEndPoint; }
  BI_NetLineAnchor*     getOtherPoint(const BI_NetLineAnchor& firstPoint) const
      noexcept;
  NetSignal& getNetSignalOfNetSegment() const noexcept;
  bool       isSelectable() const noexcept override;
  Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;

  // Setters
  void setLayer(GraphicsLayer& layer);
  void setWidth(const PositiveLength& width) noexcept;

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;
  void updateLine() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::NetLine; }
  const Point& getPosition() const noexcept override { return mPosition; }
  bool         getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void         setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_NetLine& operator=(const BI_NetLine& rhs) = delete;

private slots:
  void boardAttributesChanged();

private:
  void              init();
  BI_NetLineAnchor* deserializeAnchor(const SExpression& root,
                                      const QString&     key) const;
  void serializeAnchor(SExpression& root, BI_NetLineAnchor* anchor) const;

  // General
  BI_NetSegment&              mNetSegment;
  QScopedPointer<BGI_NetLine> mGraphicsItem;
  Point                   mPosition;  ///< the center of startpoint and endpoint
  QMetaObject::Connection mHighlightChangedConnection;

  // Attributes
  Uuid              mUuid;
  BI_NetLineAnchor* mStartPoint;
  BI_NetLineAnchor* mEndPoint;
  GraphicsLayer*    mLayer;
  PositiveLength    mWidth;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BI_NETLINE_H
