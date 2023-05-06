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

#ifndef LIBREPCB_CORE_BOARDPLANEFRAGMENTSBUILDER_H
#define LIBREPCB_CORE_BOARDPLANEFRAGMENTSBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/path.h"
#include "../../types/uuid.h"
#include "../../utils/transform.h"
#include "items/bi_plane.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Layer;
class NetSignal;
class PadGeometry;

/*******************************************************************************
 *  Class BoardPlaneFragmentsBuilder
 ******************************************************************************/

/**
 * @brief Plane fragments builder working on a ::librepcb::Board
 */
class BoardPlaneFragmentsBuilder final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BoardPlaneFragmentsBuilder(bool rebuildAirWires = false,
                                      QObject* parent = nullptr) noexcept;
  BoardPlaneFragmentsBuilder(const BoardPlaneFragmentsBuilder& other) = delete;
  ~BoardPlaneFragmentsBuilder() noexcept;

  // General Methods

  /**
   * @brief Build and apply plane fragments synchronously (blocking)
   *
   * @param board   The board to rebuild the planes of.
   * @param layers  If not `nullptr`, rebuild only planes which are scheduled
   *                to rebuild and located on the given layers (quick rebuild).
   *                If `nullptr` (default), rebuild all planes (more reliable,
   *                but slower).
   *
   * @throws Exception if any error occurred.
   */
  void runSynchronously(Board& board,
                        const QSet<const Layer*>* layers = nullptr);

  /**
   * @brief Start building plane fragments asynchronously
   *
   * The calculated fragments will automatically be applied to the board once
   * the rebuild is finished.
   *
   * @param board   The board to rebuild the planes of.
   * @param layers  If not `nullptr`, rebuild only planes which are scheduled
   *                to rebuild and located on the given layers (quick rebuild).
   *                If `nullptr` (default), rebuild all planes (more reliable,
   *                but slower).
   *
   * @retval true   If the build started.
   * @retval false  If none of the planes need a rebuild, thus did not start
   *                a rebuild.
   */
  bool startAsynchronously(Board& board,
                           const QSet<const Layer*>* layers = nullptr) noexcept;

  /**
   * @brief Check if there is currently a build in progress
   *
   * @retval true if a build is in progress.
   * @retval false if idle.
   */
  bool isBusy() const noexcept;

  /**
   * @brief Cancel the current asynchronous job
   */
  void cancel() noexcept;

  // Operator Overloadings
  BoardPlaneFragmentsBuilder& operator=(const BoardPlaneFragmentsBuilder& rhs) =
      delete;

signals:
  void started();
  void finished();

private:  // Methods
  struct PlaneData {
    Uuid uuid;
    const Layer* layer;
    Uuid netSignal;
    Path outline;
    UnsignedLength minWidth;
    UnsignedLength minClearance;
    bool keepOrphans;
    int priority;
    BI_Plane::ConnectStyle connectStyle;
  };

  struct PolygonData {
    Transform transform;  // Applied to path after preprocessing.
    const Layer* layer;
    tl::optional<Uuid> netSignal;
    Path path;
    UnsignedLength width;
    bool filled;
  };

  struct PadData {
    Transform transform;
    tl::optional<Uuid> netSignal;
    UnsignedLength clearance;
    QHash<const Layer*, QList<PadGeometry>> geometries;
  };

  struct TraceData {
    const Layer* layer;
    tl::optional<Uuid> netSignal;
    Point startPos;
    Point endPos;
    PositiveLength width;
  };

  struct JobData {
    QPointer<Board> board;
    QSet<const Layer*> layers;
    QList<PlaneData> planes;
    QList<PolygonData> polygons;
    QList<std::tuple<tl::optional<Uuid>, Point, PositiveLength>> vias;
    QList<PadData> pads;
    QList<std::tuple<Transform, PositiveLength, NonEmptyPath>> holes;
    QList<TraceData> traces;  // Converted to polygons after preprocessing.
    QHash<Uuid, QVector<Path>> result;
    bool finished = false;
  };

  std::shared_ptr<JobData> createJob(Board& board,
                                     const QSet<const Layer*>* filter) noexcept;
  std::shared_ptr<JobData> run(std::shared_ptr<JobData> data,
                               bool exceptionOnError);
  bool applyToBoard(std::shared_ptr<JobData> data) noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs. Do not
   * change this if you don't know exactly what you're doing (it affects all
   * planes in all existing boards)!
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  const bool mRebuildAirWires;
  QFuture<std::shared_ptr<JobData>> mFuture;
  QFutureWatcher<std::shared_ptr<JobData>> mWatcher;
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
