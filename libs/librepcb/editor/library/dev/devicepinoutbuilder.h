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

#ifndef LIBREPCB_EDITOR_DEVICEPINOUTBUILDER_H
#define LIBREPCB_EDITOR_DEVICEPINOUTBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoStack;

/*******************************************************************************
 *  Class DevicePinoutBuilder
 ******************************************************************************/

/**
 * @brief The DevicePinoutBuilder class
 */
class DevicePinoutBuilder final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  DevicePinoutBuilder() = delete;
  DevicePinoutBuilder(const DevicePinoutBuilder& other) = delete;
  DevicePinoutBuilder(DevicePadSignalMap& list, UndoStack& undoStack,
                      QObject* parent = nullptr) noexcept;
  ~DevicePinoutBuilder() noexcept;

  // General Methods
  void setPads(const PackagePadList& list) noexcept;
  void setSignals(const ComponentSignalList& list) noexcept;

  // State Query
  bool hasUnconnectedPadsAndSignals() const noexcept;
  bool hasAutoConnectablePads() const noexcept;
  bool areAllPadsUnconnected() const noexcept;

  // One-Shot Actions
  void resetAll() noexcept;
  void autoConnect() noexcept;
  void loadFromFile() noexcept;

  // Interactive Mode
  void startInteractiveMode() noexcept;
  void commitInteractiveMode() noexcept;
  void exitInteractiveMode() noexcept;
  int getCurrentPadNumber() const noexcept { return mCurrentPadIndex + 1; }
  QString getCurrentPadName() const noexcept;
  const QString& getSignalsFilter() const noexcept { return mSignalsFilter; }
  void setSignalsFilter(const QString& filter) noexcept;
  const auto& getFilteredSignals() noexcept { return mFilteredSignalsModel; }
  int getCurrentSignalIndex() const noexcept { return mCurrentSignalIndex; }
  void setCurrentSignalIndex(int index) noexcept;

  // Operator Overloadings
  DevicePinoutBuilder& operator=(const DevicePinoutBuilder& rhs) = delete;

private:
  void loadNextPad() noexcept;
  bool commitCurrentPad() noexcept;
  void updateFilteredSignals() noexcept;
  QMap<Uuid, Uuid> getMap() const noexcept;
  void setMap(const QString& cmdText, const QMap<Uuid, Uuid>& map);
  bool askForResetFirst() noexcept;

private:
  typedef std::pair<std::shared_ptr<const ComponentSignal>, bool> SignalChoice;

  // References
  DevicePadSignalMap& mList;
  UndoStack& mUndoStack;
  PackagePadList mPadsSorted;
  ComponentSignalList mSignals;

  // Interactive mode state
  int mCurrentPadIndex;  // -1 = not in interactive mode
  QString mSignalsFilter;
  QVector<SignalChoice> mFilteredSignals;
  std::shared_ptr<slint::VectorModel<ui::DeviceInteractivePinoutSignalData>>
      mFilteredSignalsModel;
  int mCurrentSignalIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
