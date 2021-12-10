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

#ifndef LIBREPCB_PROJECT_BI_NETSEGMENT_H
#define LIBREPCB_PROJECT_BI_NETSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bi_base.h"

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class BI_Device;
class BI_FootprintPad;
class BI_NetLine;
class BI_NetLineAnchor;
class BI_NetPoint;
class BI_Via;
class NetSignal;

/*******************************************************************************
 *  Class BI_NetSegment
 ******************************************************************************/

/**
 * @brief The BI_NetSegment class
 */
class BI_NetSegment final : public BI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_NetSegment() = delete;
  BI_NetSegment(const BI_NetSegment& other) = delete;
  BI_NetSegment(Board& board, const BI_NetSegment& other,
                const QHash<const BI_Device*, BI_Device*>& devMap);
  BI_NetSegment(Board& board, const SExpression& node,
                const Version& fileFormat);
  BI_NetSegment(Board& board, NetSignal* signal);
  ~BI_NetSegment() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }

  /// Get the net signal this segment belongs to
  ///
  /// @note If the net segment is not connected to any net (which is allowed),
  ///       nullptr is returned.
  ///
  /// @return Pointer to the net signal (nullptr if unconnected)
  NetSignal* getNetSignal() const noexcept { return mNetSignal; }

  /// Get the net name to display
  ///
  /// If connected to a net, the net name is returned. Otherwise a fallback
  /// string is returned (either an empty string, or something like "(no net)"
  /// in the user's locale). This is just for convenience to avoid implementing
  /// exactly the same logic in many different modules.
  ///
  /// @param fallback   If the segment has no net, this determines whether an
  ///                   empty string should be returned (false, default) or
  ///                   something like "(no net)".
  /// @return The net name or the fallback.
  QString getNetNameToDisplay(bool fallback = false) const noexcept;

  bool isUsed() const noexcept;
  int getViasAtScenePos(const Point& pos, QList<BI_Via*>& vias) const noexcept;
  int getNetPointsAtScenePos(const Point& pos, const GraphicsLayer* layer,
                             QList<BI_NetPoint*>& points) const noexcept;
  int getNetLinesAtScenePos(const Point& pos, const GraphicsLayer* layer,
                            QList<BI_NetLine*>& lines) const noexcept;

  BI_NetPoint* getNetPointNextToScenePos(const Point& pos,
                                         const GraphicsLayer* layer,
                                         UnsignedLength& maxDistance) const
      noexcept;
  BI_Via* getViaNextToScenePos(const Point& pos,
                               UnsignedLength& maxDistance) const noexcept;

  // Setters
  void setNetSignal(NetSignal* netsignal);

  // Via Methods
  const QList<BI_Via*>& getVias() const noexcept { return mVias; }
  BI_Via* getViaByUuid(const Uuid& uuid) const noexcept;

  // NetPoint Methods
  const QList<BI_NetPoint*>& getNetPoints() const noexcept {
    return mNetPoints;
  }
  BI_NetPoint* getNetPointByUuid(const Uuid& uuid) const noexcept;

  // NetLine Methods
  const QList<BI_NetLine*>& getNetLines() const noexcept { return mNetLines; }
  BI_NetLine* getNetLineByUuid(const Uuid& uuid) const noexcept;

  // NetPoint+NetLine Methods
  void addElements(const QList<BI_Via*>& vias,
                   const QList<BI_NetPoint*>& netpoints,
                   const QList<BI_NetLine*>& netlines);
  void removeElements(const QList<BI_Via*>& vias,
                      const QList<BI_NetPoint*>& netpoints,
                      const QList<BI_NetLine*>& netlines);

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;
  void selectAll() noexcept;
  void setSelectionRect(const QRectF rectPx) noexcept;
  void clearSelection() const noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override {
    return BI_Base::Type_t::NetSegment;
  }
  const Point& getPosition() const noexcept override {
    static Point p(0, 0);
    return p;
  }
  bool getIsMirrored() const noexcept override { return false; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  bool isSelectable() const noexcept override { return false; }
  bool isSelected() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_NetSegment& operator=(const BI_NetSegment& rhs) = delete;
  bool operator==(const BI_NetSegment& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_NetSegment& rhs) noexcept { return (this != &rhs); }

private:
  bool checkAttributesValidity() const noexcept;
  bool areAllNetPointsConnectedTogether() const noexcept;
  void findAllConnectedNetPoints(const BI_NetLineAnchor& p,
                                 QSet<const BI_Via*>& vias,
                                 QSet<const BI_FootprintPad*>& pads,
                                 QSet<const BI_NetPoint*>& points) const
      noexcept;

  // Attributes
  Uuid mUuid;

  /// The net signal this segment belongs to
  ///
  /// This is nullptr if not connected!
  NetSignal* mNetSignal;

  // Items
  QList<BI_Via*> mVias;
  QList<BI_NetPoint*> mNetPoints;
  QList<BI_NetLine*> mNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
