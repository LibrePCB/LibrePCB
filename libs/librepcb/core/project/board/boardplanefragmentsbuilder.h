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
#include "../../geometry/zone.h"
#include "../../types/uuid.h"
#include "../../utils/transform.h"
#include "items/bi_plane.h"

#include <polyclipping/clipper.hpp>

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
  // Types
  struct Result {
    QPointer<Board> board;  ///< The board of the calculated planes.
    QSet<const Layer*> layers;  ///< All processed layers.
    QHash<Uuid, QVector<Path>> planes;  ///< The calculated plane fragments.
    QStringList errors;  ///< Any occurred errors (empty on success)
    bool finished = false;  ///< Whether the run completed or was aborted.

    /// Convenience error handling
    void throwOnError() const;

    /// Apply the results to the board
    ///
    /// @return Whether any plane has been modified or not
    bool applyToBoard() noexcept;
  };

  // Constructors / Destructor
  explicit BoardPlaneFragmentsBuilder(QObject* parent = nullptr) noexcept;
  BoardPlaneFragmentsBuilder(const BoardPlaneFragmentsBuilder& other) = delete;
  ~BoardPlaneFragmentsBuilder() noexcept;

  // General Methods

  /**
   * @brief Build and apply plane fragments (blocking)
   *
   * @param board   The board to rebuild the planes of.
   * @param layers  If not `nullptr`, rebuild only planes which are scheduled
   *                to rebuild and located on the given layers (quick rebuild).
   *                If `nullptr` (default), rebuild all planes (more reliable,
   *                but slower).
   *
   * @return All calculated plane fragments
   *
   * @throws Exception if any error occurred.
   */
  QHash<Uuid, QVector<Path>> runAndApply(
      Board& board, const QSet<const Layer*>* layers = nullptr);

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
  bool start(Board& board, const QSet<const Layer*>* layers = nullptr) noexcept;

  /**
   * @brief Wait until the asynchronous operation is finished
   *
   * @return See ::librepcb::BoardPlaneFragmentsBuilder::Result
   */
  Result waitForFinished() const noexcept;

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
  void finished(Result result);

private:  // Methods
  struct PlaneData {
    Uuid uuid;
    const Layer* layer;
    tl::optional<Uuid> netSignal;
    Path outline;
    UnsignedLength minWidth;
    UnsignedLength minClearance;
    bool keepIslands;
    int priority;
    BI_Plane::ConnectStyle connectStyle;
    PositiveLength thermalGap;
    PositiveLength thermalSpokeWidth;
  };

  struct KeepoutZoneData {
    Transform transform;  // Applied to outline after preprocessing.
    Zone::Layers layers;  // Converted to boardLayers after preprocessing.
    QSet<const Layer*> boardLayers;
    Path outline;
  };

  struct PolygonData {
    Transform transform;  // Applied to path after preprocessing.
    const Layer* layer;
    tl::optional<Uuid> netSignal;
    Path path;
    UnsignedLength width;
    bool filled;
  };

  struct ViaData {
    tl::optional<Uuid> netSignal;
    Point position;
    PositiveLength diameter;
    const Layer* startLayer;
    const Layer* endLayer;
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
    // NOTE: We create a `const` copy of this structure for each thread to
    // ensure thread-safety. For the implicitly shared Qt containers this is
    // a lightweight operation, but for ClipperLib::Paths we share it with a
    // shared_ptr to avoid deep copying the whole container. This is safe
    // because the underlying std::vector is thread-safe for read-only
    // operations.

    QList<const Layer*> layers;
    QList<PlaneData> planes;
    QList<KeepoutZoneData> keepoutZones;
    QList<PolygonData> polygons;
    QList<ViaData> vias;
    QList<PadData> pads;
    QList<std::tuple<Transform, PositiveLength, NonEmptyPath>> holes;
    QList<TraceData> traces;  // Converted to polygons after preprocessing.
    std::shared_ptr<ClipperLib::Paths> boardArea;  // Populated in preprocessing
  };

  struct LayerJobResult {
    QHash<Uuid, QVector<Path>> planes;
    QStringList errors;  // Empty on success.
  };

  std::shared_ptr<JobData> createJob(Board& board,
                                     const QSet<const Layer*>* filter) noexcept;
  Result run(QPointer<Board> board, std::shared_ptr<JobData> data) noexcept;
  LayerJobResult runLayer(std::shared_ptr<const JobData> data,
                          const Layer* layer) noexcept;
  static QVector<std::pair<Point, Angle>> determineThermalSpokes(
      const PadGeometry& geometry) noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs. Do not
   * change this if you don't know exactly what you're doing (it affects all
   * planes in all existing boards)!
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  QFuture<Result> mFuture;
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
