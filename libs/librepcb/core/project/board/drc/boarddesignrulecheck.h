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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULECHECK_H
#define LIBREPCB_CORE_BOARDDESIGNRULECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../rulecheck/rulecheckmessage.h"
#include "../../../utils/transform.h"
#include "boarddesignrulecheckdata.h"

#include <clipper2/clipper.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BoardDesignRuleCheck
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheck class checks a ::librepcb::Board for
 *        design rule violations
 */
class BoardDesignRuleCheck final : public QObject {
  Q_OBJECT

public:
  // Types
  using Data = BoardDesignRuleCheckData;
  struct CalculatedJobData {
    // This structure is filled by stage 1 jobs and read by stage 2 jobs.
    // Each stage 2 job gets a copy of this structure, so no synchronization
    // is needed for thread-safety. Stage 1 jobs however all get the same
    // instance, thus require to lock it with the contained mutex.

    mutable QMutex mutex;  // To be used by stage 1 jobs.

    QHash<const Layer*, Clipper2Lib::Paths64> copperPathsPerLayer;
  };

  struct Result {
    RuleCheckMessageList messages;
    QStringList errors;  // Empty on success.
    bool quick = false;
    qint64 elapsedTimeMs = 0;
  };

  // Constructors / Destructor
  explicit BoardDesignRuleCheck(QObject* parent = nullptr) noexcept;
  ~BoardDesignRuleCheck() noexcept;

  // General Methods
  void start(Board& board, const BoardDesignRuleCheckSettings& settings,
             bool quick) noexcept;

  bool isRunning() const noexcept;

  /**
   * @brief Wait until the asynchronous operation is finished
   *
   * @return All emitted messages and occurred errors
   */
  Result waitForFinished() const noexcept;

  /**
   * @brief Cancel the current asynchronous job
   */
  void cancel() noexcept;

signals:
  void started();
  void progressPercent(int percent);
  void progressStatus(const QString& msg);
  void finished(Result result);

private:  // Methods
  typedef std::function<RuleCheckMessageList()> JobFunc;
  typedef std::function<void(const Data&, CalculatedJobData&)> Stage1Func;
  typedef RuleCheckMessageList (BoardDesignRuleCheck::*Stage2Func)(
      const Data&, const CalculatedJobData&);
  typedef RuleCheckMessageList (BoardDesignRuleCheck::*IndependentStageFunc)(
      const Data&);

  Result tryRunJob(JobFunc function, int weight) noexcept;
  Result run(std::shared_ptr<const Data> data,
             std::shared_ptr<QElapsedTimer> timer) noexcept;
  void prepareCopperPaths(const Data& data, CalculatedJobData& calcData,
                          const Layer& layer);
  RuleCheckMessageList checkCopperCopperClearances(const Data& data);
  RuleCheckMessageList checkCopperBoardClearances(const Data& data);
  RuleCheckMessageList checkCopperHoleClearances(
      const Data& data, const CalculatedJobData& calcData);
  RuleCheckMessageList checkDrillDrillClearances(const Data& data);
  RuleCheckMessageList checkDrillBoardClearances(const Data& data);
  RuleCheckMessageList checkSilkscreenStopmaskClearances(const Data& data);
  RuleCheckMessageList checkMinimumCopperWidth(const Data& data);
  RuleCheckMessageList checkMinimumPthAnnularRing(
      const Data& data, const CalculatedJobData& calcData);
  RuleCheckMessageList checkMinimumNpthDrillDiameter(const Data& data);
  RuleCheckMessageList checkMinimumNpthSlotWidth(const Data& data);
  RuleCheckMessageList checkMinimumPthDrillDiameter(const Data& data);
  RuleCheckMessageList checkMinimumPthSlotWidth(const Data& data);
  RuleCheckMessageList checkMinimumSilkscreenWidth(const Data& data);
  RuleCheckMessageList checkMinimumSilkscreenTextHeight(const Data& data);
  RuleCheckMessageList checkZones(const Data& data);
  RuleCheckMessageList checkVias(const Data& data);
  RuleCheckMessageList checkAllowedNpthSlots(const Data& data);
  RuleCheckMessageList checkAllowedPthSlots(const Data& data);
  RuleCheckMessageList checkInvalidPadConnections(const Data& data);
  RuleCheckMessageList checkDeviceClearances(const Data& data);
  RuleCheckMessageList checkBoardOutline(const Data& data);
  RuleCheckMessageList checkBoardCutouts(const Data& data,
                                         const CalculatedJobData& calcData);
  RuleCheckMessageList checkUsedLayers(const Data& data);
  RuleCheckMessageList checkForUnplacedComponents(const Data& data);
  RuleCheckMessageList checkForMissingConnections(const Data& data);
  RuleCheckMessageList checkForStaleObjects(const Data& data);
  static void checkMinimumWidth(RuleCheckMessageList& messages,
                                const Data& data,
                                const UnsignedLength& minWidth,
                                std::function<bool(const Layer&)> layerFilter);
  static bool requiresHoleSlotWarning(
      const Data::Hole& hole,
      BoardDesignRuleCheckSettings::AllowedSlots allowed);
  static Clipper2Lib::Paths64 getBoardClearanceArea(
      const Data& data, const UnsignedLength& clearance);
  static QVector<Path> getBoardOutlines(
      const Data& data, const QSet<const Layer*>& layers) noexcept;
  static Clipper2Lib::Paths64 getDeviceOutlinePaths(const Data::Device& device,
                                                    const Layer& layer);
  static QVector<Path> getDeviceLocation(const Data::Device& device);
  static QVector<Path> getViaLocation(const Data::Via& via) noexcept;
  static bool isViaUseless(const Data& data, const Data::Segment& ns,
                           const Data::Via& via) noexcept;
  static QVector<Path> getTraceLocation(const Data::Trace& trace) noexcept;
  static QVector<Path> getHoleLocation(
      const Data::Hole& hole,
      const Transform& transform = Transform()) noexcept;
  void emitProgress(int percent) noexcept;
  void emitStatus(const QString& status) noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs.
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  QMutex mMutex;
  int mProgressTotal = 0;  // Only for progress range 20..100%
  int mProgressCounter = 0;  // 0..mProgressTotal
  QFuture<Result> mFuture;
  bool mAbort = false;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
