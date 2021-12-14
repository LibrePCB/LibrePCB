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

#ifndef LIBREPCB_PROJECT_BOARDDESIGNRULECHECK_H
#define LIBREPCB_PROJECT_BOARDDESIGNRULECHECK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boarddesignrulecheckmessage.h"

#include <polyclipping/clipper.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class BI_Device;
class Board;
class NetSignal;

/*******************************************************************************
 *  Class BoardDesignRuleCheck
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheck class checks a ::librepcb::project::Board for
 *        design rule violations
 */
class BoardDesignRuleCheck final : public QObject {
  Q_OBJECT

public:
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

    bool checkPthRestring;
    UnsignedLength minPthRestring;

    bool checkNpthDrillDiameter;
    UnsignedLength minNpthDrillDiameter;

    bool checkPthDrillDiameter;
    UnsignedLength minPthDrillDiameter;

    bool checkCourtyardClearance;
    Length courtyardOffset;

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
        checkPthRestring(true),
        minPthRestring(150000),  // 150um
        checkNpthDrillDiameter(true),
        minNpthDrillDiameter(250000),  // 250um
        checkPthDrillDiameter(true),
        minPthDrillDiameter(250000),  // 250um
        checkCourtyardClearance(true),
        courtyardOffset(0),  // 0um
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
  void checkMinimumPthRestring(int progressStart, int progressEnd);
  void checkMinimumPthDrillDiameter(int progressStart, int progressEnd);
  void checkMinimumNpthDrillDiameter(int progressStart, int progressEnd);
  const ClipperLib::Paths& getCopperPaths(const GraphicsLayer* layer,
                                          const NetSignal* netsignal);
  ClipperLib::Paths getDeviceCourtyardPaths(const BI_Device& device,
                                            const GraphicsLayer* layer);
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
  QHash<const GraphicsLayer*, QHash<const NetSignal*, ClipperLib::Paths>>
      mCachedPaths;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
