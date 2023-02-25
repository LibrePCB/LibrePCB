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
#include "../../../utils/transform.h"
#include "boarddesignrulecheckmessage.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Board;
class GraphicsLayer;
class Hole;
class NetSignal;

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
  enum class SlotsWarningLevel : int {
    Curved = 0,  ///< Only warn about slots with curves.
    MultiSegment = 1,  ///< Warn about slots with multiple segments or curves.
    All = 2,  ///< Warn about all slots.
  };

  struct Options {
    bool rebuildPlanes;

    bool checkCopperWidth;
    UnsignedLength minCopperWidth;

    bool checkCopperCopperClearance;
    UnsignedLength minCopperCopperClearance;

    bool checkCopperBoardClearance;
    UnsignedLength minCopperBoardClearance;

    bool checkCopperNpthClearance;
    UnsignedLength minCopperNpthClearance;

    bool checkPthAnnularRing;
    UnsignedLength minPthAnnularRing;

    bool checkNpthDrillDiameter;
    UnsignedLength minNpthDrillDiameter;

    bool checkNpthSlotWidth;
    UnsignedLength minNpthSlotWidth;

    bool checkPthDrillDiameter;
    UnsignedLength minPthDrillDiameter;

    bool checkPthSlotWidth;
    UnsignedLength minPthSlotWidth;

    bool checkNpthSlotsWarning;
    SlotsWarningLevel npthSlotsWarning;

    bool checkPthSlotsWarning;
    SlotsWarningLevel pthSlotsWarning;

    bool checkCourtyardClearance;
    Length courtyardOffset;

    bool checkBrokenPadConnections;

    bool checkMissingConnections;

    Options()
      : rebuildPlanes(true),
        checkCopperWidth(true),
        minCopperWidth(200000),  // 200um
        checkCopperCopperClearance(true),
        minCopperCopperClearance(200000),  // 200um
        checkCopperBoardClearance(true),
        minCopperBoardClearance(300000),  // 300um
        checkCopperNpthClearance(true),
        minCopperNpthClearance(200000),  // 200um
        checkPthAnnularRing(true),
        minPthAnnularRing(150000),  // 150um
        checkNpthDrillDiameter(true),
        minNpthDrillDiameter(250000),  // 250um
        checkNpthSlotWidth(true),
        minNpthSlotWidth(1000000),  // 1mm
        checkPthDrillDiameter(true),
        minPthDrillDiameter(250000),  // 250um
        checkPthSlotWidth(true),
        minPthSlotWidth(700000),  // 0.7mm
        checkNpthSlotsWarning(true),
        npthSlotsWarning(SlotsWarningLevel::MultiSegment),
        checkPthSlotsWarning(true),
        pthSlotsWarning(SlotsWarningLevel::MultiSegment),
        checkCourtyardClearance(true),
        courtyardOffset(0),  // 0um
        checkBrokenPadConnections(true),
        checkMissingConnections(true) {}
  };

  // Constructors / Destructor
  explicit BoardDesignRuleCheck(Board& board, const Options& options,
                                QObject* parent = nullptr) noexcept;
  ~BoardDesignRuleCheck() noexcept;

  // Getters
  const QStringList& getProgressStatus() const noexcept {
    return mProgressStatus;
  }
  const QList<BoardDesignRuleCheckMessage>& getMessages() const noexcept {
    return mMessages;
  }

  // General Methods
  void execute();

signals:
  void started();
  void progressPercent(int percent);
  void progressStatus(const QString& msg);
  void progressMessage(const QString& msg);
  void finished();

private:  // Methods
  void rebuildPlanes(int progressStart, int progressEnd);
  void checkForMissingConnections(int progressStart, int progressEnd);
  void checkCopperBoardClearances(int progressStart, int progressEnd);
  void checkCopperCopperClearances(int progressStart, int progressEnd);
  void checkCourtyardClearances(int progressStart, int progressEnd);
  void checkMinimumCopperWidth(int progressStart, int progressEnd);
  void checkMinimumPthAnnularRing(int progressStart, int progressEnd);
  void checkMinimumNpthDrillDiameter(int progressStart, int progressEnd);
  void checkMinimumNpthSlotWidth(int progressStart, int progressEnd);
  void checkMinimumPthDrillDiameter(int progressStart, int progressEnd);
  void checkMinimumPthSlotWidth(int progressStart, int progressEnd);
  void checkWarnNpthSlots(int progressStart, int progressEnd);
  void checkWarnPthSlots(int progressStart, int progressEnd);
  void checkInvalidPadConnections(int progressStart, int progressEnd);
  template <typename THole>
  void processHoleSlotWarning(const THole& hole, SlotsWarningLevel level,
                              const Transform& transform1 = Transform(),
                              const Transform& transform2 = Transform());
  const ClipperLib::Paths& getCopperPaths(
      const GraphicsLayer& layer, const QSet<const NetSignal*>& netsignals);
  ClipperLib::Paths getDeviceCourtyardPaths(const BI_Device& device,
                                            const GraphicsLayer* layer);
  template <typename THole>
  QVector<Path> getHoleLocation(const THole& hole,
                                const Transform& transform1 = Transform(),
                                const Transform& transform2 = Transform()) const
      noexcept;
  void emitStatus(const QString& status) noexcept;
  void emitMessage(const BoardDesignRuleCheckMessage& msg) noexcept;
  QString formatLength(const Length& length) const noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs.
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  Board& mBoard;
  Options mOptions;
  QStringList mProgressStatus;
  QList<BoardDesignRuleCheckMessage> mMessages;
  QHash<QPair<const GraphicsLayer*, QSet<const NetSignal*>>, ClipperLib::Paths>
      mCachedPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::BoardDesignRuleCheck::SlotsWarningLevel)

#endif
