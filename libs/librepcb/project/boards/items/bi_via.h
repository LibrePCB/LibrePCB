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

#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/uuid.h>

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
  // Public Types
  enum class Shape { Round, Square, Octagon };

  // Constructors / Destructor
  BI_Via()                    = delete;
  BI_Via(const BI_Via& other) = delete;
  BI_Via(BI_NetSegment& netsegment, const BI_Via& other);
  BI_Via(BI_NetSegment& netsegment, const SExpression& node);
  BI_Via(BI_NetSegment& netsegment, const Point& position, BI_Via::Shape shape,
         const PositiveLength& size, const PositiveLength& drillDiameter,
         const QString& startLayerName, const QString& stopLayerName);
  BI_Via(BI_NetSegment& netsegment, const Point& position, BI_Via::Shape shape,
         const PositiveLength& size, const PositiveLength& drillDiameter,
         GraphicsLayer* startLayer, GraphicsLayer* stopLayer);
  ~BI_Via() noexcept;

  // Getters
  BI_NetSegment&        getNetSegment() const noexcept { return mNetSegment; }
  NetSignal&            getNetSignalOfNetSegment() const noexcept;
  const Uuid&           getUuid() const noexcept { return mUuid; }
  Shape                 getShape() const noexcept { return mShape; }
  const PositiveLength& getDrillDiameter() const noexcept {
    return mDrillDiameter;
  }
  GraphicsLayer*        getStartLayer() const noexcept {
    return mBoard.getLayerStack().getLayer(*mStartLayerName);
  }
  GraphicsLayer*        getStopLayer() const noexcept {
    return mBoard.getLayerStack().getLayer(*mStopLayerName);
  }
  const QString&        getStartLayerName() const noexcept {
    return *mStartLayerName;
  }
  const QString&        getStopLayerName() const noexcept {
    return *mStopLayerName;
  }
  int             getStartLayerIndex() const noexcept;
  int             getStopLayerIndex() const noexcept;

  const PositiveLength& getSize() const noexcept { return mSize; }
  bool isUsed() const noexcept { return (mRegisteredNetLines.count() > 0); }
  bool isOnLayer(const QString& layerName) const noexcept;
  bool isOnLayer(GraphicsLayer* layer) const noexcept;
  bool isSelectable() const noexcept override;
  Path getOutline(const Length& expansion = Length(0)) const noexcept;
  Path getSceneOutline(const Length& expansion = Length(0)) const noexcept;
  QPainterPath toQPainterPathPx(const Length& expansion = Length(0)) const
      noexcept;

  // Setters
  void setPosition(const Point& position) noexcept;
  void setShape(Shape shape) noexcept;
  void setSize(const PositiveLength& size) noexcept;
  void setDrillDiameter(const PositiveLength& diameter) noexcept;
  void setLayers(const GraphicsLayerName& startLayer,
                 const GraphicsLayerName& stopLayer);
  void setLayers(GraphicsLayer* startLayer, GraphicsLayer* stopLayer);

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::Via; }
  const Point& getPosition() const noexcept override { return mPosition; }
  bool         getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void         setSelected(bool selected) noexcept override;

  // Inherited from BI_NetLineAnchor
  void                     registerNetLine(BI_NetLine& netline) override;
  void                     unregisterNetLine(BI_NetLine& netline) override;
  const QSet<BI_NetLine*>& getNetLines() const noexcept override {
    return mRegisteredNetLines;
  }

  // Operator Overloadings
  BI_Via& operator=(const BI_Via& rhs) = delete;
  bool    operator==(const BI_Via& rhs) noexcept { return (this == &rhs); }
  bool    operator!=(const BI_Via& rhs) noexcept { return (this != &rhs); }

private:
  void init();
  void boardAttributesChanged();

  // General
  BI_NetSegment&          mNetSegment;
  QScopedPointer<BGI_Via> mGraphicsItem;
  QMetaObject::Connection mHighlightChangedConnection;

  // Attributes
  Uuid                  mUuid;
  Point                 mPosition;
  Shape                 mShape;
  PositiveLength        mSize;
  PositiveLength        mDrillDiameter;
  GraphicsLayerName     mStartLayerName;
  GraphicsLayerName     mStopLayerName;

  // Registered Elements
  QSet<BI_NetLine*> mRegisteredNetLines;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

}  // namespace project

template <>
inline SExpression serializeToSExpression(const project::BI_Via::Shape& obj) {
  switch (obj) {
    case project::BI_Via::Shape::Round:
      return SExpression::createToken("round");
    case project::BI_Via::Shape::Square:
      return SExpression::createToken("square");
    case project::BI_Via::Shape::Octagon:
      return SExpression::createToken("octagon");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline project::BI_Via::Shape deserializeFromSExpression(
    const SExpression& sexpr, bool throwIfEmpty) {
  QString str = sexpr.getStringOrToken(throwIfEmpty);
  if (str == "round")
    return project::BI_Via::Shape::Round;
  else if (str == "square")
    return project::BI_Via::Shape::Square;
  else if (str == "octagon")
    return project::BI_Via::Shape::Octagon;
  else
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(project::BI_Via::tr("Unknown via shape: \"%1\"")).arg(str));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BI_VIA_H
