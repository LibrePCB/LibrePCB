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
#include "boarddesignrulecheckmessages.h"
#include "boarddesignrulechecksettings.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Device;
class Board;
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
  // Constructors / Destructor
  explicit BoardDesignRuleCheck(Board& board,
                                const BoardDesignRuleCheckSettings& settings,
                                QObject* parent = nullptr) noexcept;
  ~BoardDesignRuleCheck() noexcept;

  // Getters
  const QStringList& getProgressStatus() const noexcept {
    return mProgressStatus;
  }
  const RuleCheckMessageList& getMessages() const noexcept { return mMessages; }

  // General Methods
  void execute(bool quick);

signals:
  void started();
  void progressPercent(int percent);
  void progressStatus(const QString& msg);
  void progressMessage(const QString& msg);
  void finished();

private:  // Methods
  void rebuildPlanes(int progressEnd);
  void checkMinimumCopperWidth(int progressEnd);
  void checkCopperCopperClearances(int progressEnd);
  void checkCopperBoardClearances(int progressEnd);
  void checkCopperHoleClearances(int progressEnd);
  void checkDrillDrillClearances(int progressEnd);
  void checkDrillBoardClearances(int progressEnd);
  void checkMinimumPthAnnularRing(int progressEnd);
  void checkMinimumNpthDrillDiameter(int progressEnd);
  void checkMinimumNpthSlotWidth(int progressEnd);
  void checkMinimumPthDrillDiameter(int progressEnd);
  void checkMinimumPthSlotWidth(int progressEnd);
  void checkZones(int progressEnd);
  void checkVias(int progressEnd);
  void checkAllowedNpthSlots(int progressEnd);
  void checkAllowedPthSlots(int progressEnd);
  void checkInvalidPadConnections(int progressEnd);
  void checkCourtyardClearances(int progressEnd);
  void checkBoardOutline(int progressEnd);
  void checkForUnplacedComponents(int progressEnd);
  void checkCircuitDefaultDevices(int progressEnd);
  void checkForMissingConnections(int progressEnd);
  void checkForStaleObjects(int progressEnd);
  template <typename THole>
  bool requiresHoleSlotWarning(
      const THole& hole, BoardDesignRuleCheckSettings::AllowedSlots allowed);
  ClipperLib::Paths getBoardClearanceArea(
      const UnsignedLength& clearance) const;
  const ClipperLib::Paths& getCopperPaths(
      const Layer& layer, const QSet<const NetSignal*>& netsignals);
  ClipperLib::Paths getDeviceCourtyardPaths(const BI_Device& device,
                                            const Layer& layer);
  QVector<Path> getDeviceLocation(const BI_Device& device) const;
  QVector<Path> getViaLocation(const BI_Via& via) const noexcept;
  template <typename THole>
  QVector<Path> getHoleLocation(const THole& hole,
                                const Transform& transform = Transform()) const
      noexcept;
  void emitProgress(int percent) noexcept;
  void emitStatus(const QString& status) noexcept;
  void emitMessage(const std::shared_ptr<const RuleCheckMessage>& msg) noexcept;
  QString formatLength(const Length& length) const noexcept;

  /**
   * Returns the maximum allowed arc tolerance when flattening arcs.
   */
  static PositiveLength maxArcTolerance() noexcept {
    return PositiveLength(5000);
  }

private:  // Data
  Board& mBoard;
  const BoardDesignRuleCheckSettings& mSettings;
  bool mIgnorePlanes;
  int mProgressPercent;
  QStringList mProgressStatus;
  RuleCheckMessageList mMessages;
  QHash<QPair<const Layer*, QSet<const NetSignal*>>, ClipperLib::Paths>
      mCachedPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
