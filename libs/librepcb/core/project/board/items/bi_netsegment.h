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

#ifndef LIBREPCB_CORE_BI_NETSEGMENT_H
#define LIBREPCB_CORE_BI_NETSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/point.h"
#include "../../../types/uuid.h"
#include "bi_base.h"
#include "bi_netline.h"
#include "bi_netpoint.h"
#include "bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class BI_NetLineAnchor;
class BI_Pad;
class NetSignal;

/*******************************************************************************
 *  Class BI_NetSegment
 ******************************************************************************/

/**
 * @brief The BI_NetSegment class
 */
class BI_NetSegment final : public BI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  BI_NetSegment() = delete;
  BI_NetSegment(const BI_NetSegment& other) = delete;
  BI_NetSegment(Board& board, const Uuid& uuid, NetSignal* signal);
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

  // Setters
  void setNetSignal(NetSignal* netsignal);

  // Element Getters
  const QMap<Uuid, BI_Via*>& getVias() const noexcept { return mVias; }
  const QMap<Uuid, BI_NetPoint*>& getNetPoints() const noexcept {
    return mNetPoints;
  }
  const QMap<Uuid, BI_NetLine*>& getNetLines() const noexcept {
    return mNetLines;
  }

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

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BI_NetSegment& operator=(const BI_NetSegment& rhs) = delete;
  bool operator==(const BI_NetSegment& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const BI_NetSegment& rhs) noexcept { return (this != &rhs); }

signals:
  void elementsAdded(const QList<BI_Via*>& vias,
                     const QList<BI_NetPoint*>& netPoints,
                     const QList<BI_NetLine*>& netLines);
  void elementsRemoved(const QList<BI_Via*>& vias,
                       const QList<BI_NetPoint*>& netPoints,
                       const QList<BI_NetLine*>& netLines);

private:
  bool checkAttributesValidity() const noexcept;
  bool areAllNetPointsConnectedTogether() const noexcept;
  void findAllConnectedNetPoints(const BI_NetLineAnchor& p,
                                 QSet<const BI_Via*>& vias,
                                 QSet<const BI_Pad*>& pads,
                                 QSet<const BI_NetPoint*>& points,
                                 QSet<const BI_NetLine*>& lines) const noexcept;

  // Attributes
  Uuid mUuid;

  /// The net signal this segment belongs to
  ///
  /// This is nullptr if not connected!
  NetSignal* mNetSignal;

  // Items
  QMap<Uuid, BI_Via*> mVias;
  QMap<Uuid, BI_NetPoint*> mNetPoints;
  QMap<Uuid, BI_NetLine*> mNetLines;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
