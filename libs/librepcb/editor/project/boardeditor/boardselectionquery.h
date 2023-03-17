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

#ifndef LIBREPCB_EDITOR_BOARDSELECTIONQUERY_H
#define LIBREPCB_EDITOR_BOARDSELECTIONQUERY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class BI_FootprintPad;
class BI_Hole;
class BI_NetLine;
class BI_NetPoint;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Via;
class Uuid;

namespace editor {

class BoardGraphicsScene;

/*******************************************************************************
 *  Class BoardSelectionQuery
 ******************************************************************************/

/**
 * @brief The BoardSelectionQuery class
 */
class BoardSelectionQuery final : public QObject {
  Q_OBJECT

public:
  // Types
  struct NetSegmentItems {
    QSet<BI_Via*> vias;
    QSet<BI_NetPoint*> netpoints;
    QSet<BI_NetLine*> netlines;
  };

  // Constructors / Destructor
  BoardSelectionQuery() = delete;
  BoardSelectionQuery(const BoardSelectionQuery& other) = delete;
  explicit BoardSelectionQuery(BoardGraphicsScene& scene,
                               QObject* parent = nullptr);
  ~BoardSelectionQuery() noexcept;

  // Getters
  const QSet<BI_Device*>& getDeviceInstances() const noexcept {
    return mResultDeviceInstances;
  }
  const QSet<BI_NetPoint*>& getNetPoints() const noexcept {
    return mResultNetPoints;
  }
  const QSet<BI_NetLine*>& getNetLines() const noexcept {
    return mResultNetLines;
  }
  const QSet<BI_Via*>& getVias() const noexcept { return mResultVias; }
  const QSet<BI_Plane*>& getPlanes() const noexcept { return mResultPlanes; }
  const QSet<BI_Polygon*>& getPolygons() const noexcept {
    return mResultPolygons;
  }
  const QSet<BI_StrokeText*>& getStrokeTexts() const noexcept {
    return mResultStrokeTexts;
  }
  const QSet<BI_Hole*>& getHoles() const noexcept { return mResultHoles; }

  /**
   * @brief Get vias, net points and net lines grouped by net segment
   *
   * Same as #getVias(), #getNetPoints() and #getNetLines(), but grouped
   * by their corresponding net segments. Only net segments containing selected
   * items are returned.
   *
   * @return List of net segments containing the selected items
   */
  QHash<BI_NetSegment*, NetSegmentItems> getNetSegmentItems() const noexcept;
  int getResultCount() const noexcept;
  bool isResultEmpty() const noexcept { return (getResultCount() == 0); }

  // General Methods
  void addDeviceInstancesOfSelectedFootprints() noexcept;
  void addSelectedVias() noexcept;
  void addSelectedNetPoints() noexcept;
  void addSelectedNetLines() noexcept;
  void addSelectedPlanes() noexcept;
  void addSelectedPolygons() noexcept;
  void addSelectedBoardStrokeTexts() noexcept;
  void addSelectedFootprintStrokeTexts() noexcept;
  void addSelectedHoles() noexcept;
  /**
   * @brief Add net points of the selected net lines
   *
   * @param onlyIfAllNetLinesSelected   If true, net points are added only if
   *                                    *all* connected net lines are selected.
   *                                    If false, net points are added if at
   *                                    least one of the connected net lines
   *                                    is selected.
   */
  void addNetPointsOfNetLines(bool onlyIfAllNetLinesSelected = false) noexcept;

  // Operator Overloadings
  BoardSelectionQuery& operator=(const BoardSelectionQuery& rhs) = delete;

private:  // Data
  BoardGraphicsScene& mScene;

  // query result
  QSet<BI_Device*> mResultDeviceInstances;
  QSet<BI_NetPoint*> mResultNetPoints;
  QSet<BI_NetLine*> mResultNetLines;
  QSet<BI_Via*> mResultVias;
  QSet<BI_Plane*> mResultPlanes;
  QSet<BI_Polygon*> mResultPolygons;
  QSet<BI_StrokeText*> mResultStrokeTexts;
  QSet<BI_Hole*> mResultHoles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
