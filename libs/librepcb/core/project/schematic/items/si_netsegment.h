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

#ifndef LIBREPCB_CORE_SI_NETSEGMENT_H
#define LIBREPCB_CORE_SI_NETSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../serialization/serializableobject.h"
#include "../../../types/point.h"
#include "../../../types/uuid.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;
class SI_NetLabel;
class SI_NetLine;
class SI_NetLineAnchor;
class SI_NetPoint;
class SI_SymbolPin;

/*******************************************************************************
 *  Class SI_NetSegment
 ******************************************************************************/

/**
 * @brief The SI_NetSegment class
 *
 * @todo Do not allow to create empty netsegments!
 */
class SI_NetSegment final : public SI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_NetSegment() = delete;
  SI_NetSegment(const SI_NetSegment& other) = delete;
  SI_NetSegment(Schematic& schematic, const SExpression& node,
                const Version& fileFormat);
  SI_NetSegment(Schematic& schematic, NetSignal& signal);
  ~SI_NetSegment() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  NetSignal& getNetSignal() const noexcept { return *mNetSignal; }
  bool isUsed() const noexcept;
  int getNetPointsAtScenePos(const Point& pos,
                             QList<SI_NetPoint*>& points) const noexcept;
  int getNetLinesAtScenePos(const Point& pos, QList<SI_NetLine*>& lines) const
      noexcept;
  int getNetLabelsAtScenePos(const Point& pos,
                             QList<SI_NetLabel*>& labels) const noexcept;
  QSet<QString> getForcedNetNames() const noexcept;
  QString getForcedNetName() const noexcept;
  Point calcNearestPoint(const Point& p) const noexcept;
  QSet<SI_SymbolPin*> getAllConnectedPins() const noexcept;

  // Setters
  void setNetSignal(NetSignal& netsignal);

  // NetPoint Methods
  const QList<SI_NetPoint*>& getNetPoints() const noexcept {
    return mNetPoints;
  }
  SI_NetPoint* getNetPointByUuid(const Uuid& uuid) const noexcept;

  // NetLine Methods
  const QList<SI_NetLine*>& getNetLines() const noexcept { return mNetLines; }
  SI_NetLine* getNetLineByUuid(const Uuid& uuid) const noexcept;

  // NetPoint+NetLine Methods
  void addNetPointsAndNetLines(const QList<SI_NetPoint*>& netpoints,
                               const QList<SI_NetLine*>& netlines);
  void removeNetPointsAndNetLines(const QList<SI_NetPoint*>& netpoints,
                                  const QList<SI_NetLine*>& netlines);

  // NetLabel Methods
  const QList<SI_NetLabel*>& getNetLabels() const noexcept {
    return mNetLabels;
  }
  SI_NetLabel* getNetLabelByUuid(const Uuid& uuid) const noexcept;
  void addNetLabel(SI_NetLabel& netlabel);
  void removeNetLabel(SI_NetLabel& netlabel);
  void updateAllNetLabelAnchors() noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;
  void selectAll() noexcept;
  void setSelectionRect(const QRectF rectPx) noexcept;
  void clearSelection() const noexcept;

  /// @copydoc ::librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override {
    return SI_Base::Type_t::NetSegment;
  }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  bool isSelected() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  SI_NetSegment& operator=(const SI_NetSegment& rhs) = delete;
  bool operator==(const SI_NetSegment& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const SI_NetSegment& rhs) noexcept { return (this != &rhs); }

private:
  bool checkAttributesValidity() const noexcept;
  bool areAllNetPointsConnectedTogether() const noexcept;
  void findAllConnectedNetPoints(const SI_NetLineAnchor& p,
                                 QSet<const SI_SymbolPin*>& pins,
                                 QSet<const SI_NetPoint*>& points) const
      noexcept;

  // Attributes
  Uuid mUuid;
  NetSignal* mNetSignal;

  // Items
  QList<SI_NetPoint*> mNetPoints;
  QList<SI_NetLine*> mNetLines;
  QList<SI_NetLabel*> mNetLabels;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
