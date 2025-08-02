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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "devicepinoutbuilder.h"

#include "../../dialogs/filedialog.h"
#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"

#include <librepcb/core/fileio/fileutils.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DevicePinoutBuilder::DevicePinoutBuilder(DevicePadSignalMap& list,
                                         UndoStack& undoStack,
                                         QObject* parent) noexcept
  : QObject(parent),
    mList(list),
    mUndoStack(undoStack),
    mPadsSorted(),
    mSignals(),
    mCurrentPadIndex(-1),
    mSignalsFilter(),
    mFilteredSignals(),
    mFilteredSignalsModel(
        new slint::VectorModel<ui::DeviceInteractivePinoutSignalData>()),
    mCurrentSignalIndex(0) {
  exitInteractiveMode();
}

DevicePinoutBuilder::~DevicePinoutBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DevicePinoutBuilder::setPads(const PackagePadList& list) noexcept {
  exitInteractiveMode();

  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  mPadsSorted =
      list.sorted([&collator](const PackagePad& lhs, const PackagePad& rhs) {
        return collator(*lhs.getName(), *rhs.getName());
      });
}

void DevicePinoutBuilder::setSignals(const ComponentSignalList& list) noexcept {
  exitInteractiveMode();
  mSignals = list;
}

bool DevicePinoutBuilder::hasUnconnectedPadsAndSignals() const noexcept {
  bool unconnectedPads = false;
  QSet<Uuid> unusedSignals = mSignals.getUuidSet();
  for (auto& item : mList) {
    if (auto signal = item.getSignalUuid()) {
      unusedSignals.remove(*signal);
    } else {
      unconnectedPads = true;
    }
  }
  return unconnectedPads && (!unusedSignals.isEmpty());
}

bool DevicePinoutBuilder::hasAutoConnectablePads() const noexcept {
  // Get names of all signals.
  QSet<QString> signalNames;
  for (const auto& sig : mSignals) {
    signalNames.insert(sig.getName()->toLower());
  }

  // Check if any unconnected pad name matches a signal name.
  for (auto& item : mList) {
    if (!item.getSignalUuid()) {
      if (const auto pad = mPadsSorted.find(item.getPadUuid())) {
        if (signalNames.contains(pad->getName()->toLower())) {
          return true;
        }
      }
    }
  }
  return false;
}

bool DevicePinoutBuilder::areAllPadsUnconnected() const noexcept {
  for (auto& item : mList) {
    if (item.getSignalUuid()) {
      return false;
    }
  }
  return (!mList.isEmpty()) && (!mSignals.isEmpty());
}

void DevicePinoutBuilder::resetAll() noexcept {
  try {
    exitInteractiveMode();
    setMap(tr("Reset Pinout"), QMap<Uuid, Uuid>());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void DevicePinoutBuilder::autoConnect() noexcept {
  try {
    exitInteractiveMode();

    // Get initial pinout.
    QMap<Uuid, Uuid> map = getMap();
    if ((!map.isEmpty()) && askForResetFirst()) {
      map.clear();
    }

    // Connect.
    for (const auto& pad : mPadsSorted) {
      if (map.contains(pad.getUuid())) continue;  // Already connected.
      if (auto s = mSignals.find(*pad.getName(), Qt::CaseSensitive)) {
        map.insert(pad.getUuid(), s->getUuid());
      } else if (auto s = mSignals.find(*pad.getName(), Qt::CaseInsensitive)) {
        map.insert(pad.getUuid(), s->getUuid());
      }
    }

    // Save pinout.
    setMap(tr("Auto-Connect Pads To Signals"), map);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void DevicePinoutBuilder::loadFromFile() noexcept {
  try {
    exitInteractiveMode();

    // Select file.
    QSettings cs;
    const QString csKey = "library_editor/device_editor/load_pinout_file";
    const QString defaultFp = cs.value(csKey, QDir::homePath()).toString();
    const FilePath fp(FileDialog::getOpenFileName(
        nullptr, tr("Choose Pinout File"), defaultFp,
        "Comma-Separated Values (*.csv)"));
    if (!fp.isValid()) return;
    cs.setValue(csKey, fp.toStr());

    // Parse file.
    QMap<QString, QString> pinout;  // pad name -> signal name
    const QStringList lines =
        QString(FileUtils::readFile(fp)).remove("\r").split("\n");
    const QStringList cols = lines.value(0).toLower().split(',');
    const int padCol = cols.contains("pad") ? cols.indexOf("pad") : 0;
    const int signalCol = cols.contains("signal") ? cols.indexOf("signal") : 1;
    foreach (const QString& line, lines) {
      const QStringList values = line.split(',');
      if (values.count() > std::max(padCol, signalCol)) {
        pinout.insert(values.at(padCol).trimmed(),
                      values.at(signalCol).trimmed());
      }
    }

    // Get initial pinout.
    QMap<Uuid, Uuid> map = getMap();
    if ((!map.isEmpty()) && askForResetFirst()) {
      map.clear();
    }

    // Map pad and signal names.
    QMap<QString, Uuid> padMap;
    QMap<QString, Uuid> signalMap;
    for (auto it = pinout.begin(); it != pinout.end(); it++) {
      if (auto pad = mPadsSorted.find(it.key(), Qt::CaseSensitive)) {
        padMap.insert(it.key(), pad->getUuid());
      } else if (auto pad = mPadsSorted.find(it.key(), Qt::CaseInsensitive)) {
        padMap.insert(it.key(), pad->getUuid());
      }
      if (auto sig = mSignals.find(it.value(), Qt::CaseSensitive)) {
        signalMap.insert(it.value(), sig->getUuid());
      } else if (auto sig = mSignals.find(it.value(), Qt::CaseInsensitive)) {
        signalMap.insert(it.value(), sig->getUuid());
      }
    }

    // Connect.
    for (auto it = pinout.begin(); it != pinout.end(); it++) {
      auto padIt = padMap.find(it.key());
      auto signalIt = signalMap.find(it.value());
      if ((padIt != padMap.end()) && (signalIt != signalMap.end()) &&
          (!map.contains(*padIt)) && (mList.contains(*padIt))) {
        map.insert(*padIt, *signalIt);
      }
    }

    // Save pinout.
    setMap(tr("Load Pinout From File"), map);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void DevicePinoutBuilder::startInteractiveMode() noexcept {
  mCurrentPadIndex = -1;
  loadNextPad();
}

void DevicePinoutBuilder::commitInteractiveMode() noexcept {
  if (commitCurrentPad()) {
    loadNextPad();
  }
}

void DevicePinoutBuilder::exitInteractiveMode() noexcept {
  mCurrentPadIndex = -1;
}

QString DevicePinoutBuilder::getCurrentPadName() const noexcept {
  if (auto pad = mPadsSorted.value(mCurrentPadIndex)) {
    return *pad->getName();
  } else {
    return QString();
  }
}

void DevicePinoutBuilder::setSignalsFilter(const QString& filter) noexcept {
  if (filter == mSignalsFilter) return;

  mSignalsFilter = filter;
  updateFilteredSignals();
}

void DevicePinoutBuilder::setCurrentSignalIndex(int index) noexcept {
  if (mFilteredSignals.isEmpty()) {
    mCurrentSignalIndex = 0;
    return;  // Avoid division by zero!
  }

  if (index == -2) {
    // Select "unconnected", no matter which index it is.
    for (int i = 0; i < mFilteredSignals.count(); ++i) {
      if (!mFilteredSignals.at(i).first) {
        mCurrentSignalIndex = i;
        break;
      }
    }
  } else {
    index += mFilteredSignals.count();  // Wrap around to negative.
    index %= mFilteredSignals.count();  // Wrap around to positive.
    mCurrentSignalIndex = std::max(index, 0);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DevicePinoutBuilder::loadNextPad() noexcept {
  while (mCurrentPadIndex < (mPadsSorted.count() - 1)) {
    ++mCurrentPadIndex;
    if (auto pad = mPadsSorted.value(mCurrentPadIndex)) {
      auto item = mList.find(pad->getUuid());
      if (item && (!item->getSignalUuid())) {
        mSignalsFilter.clear();
        updateFilteredSignals();
        return;
      }
    }
  }

  // All pads assigned -> finish.
  exitInteractiveMode();
}

bool DevicePinoutBuilder::commitCurrentPad() noexcept {
  // If no signal is selected, do not allow to commit (this happens if a
  // filter is set but no signals match the filter).
  if ((mCurrentSignalIndex < 0) ||
      (mCurrentSignalIndex >= mFilteredSignals.count())) {
    return false;
  }

  if (auto pad = mPadsSorted.value(mCurrentPadIndex)) {
    if (auto item = mList.find(pad->getUuid())) {
      if (auto sig = mFilteredSignals.value(mCurrentSignalIndex).first) {
        try {
          std::unique_ptr<CmdDevicePadSignalMapItemEdit> cmd(
              new CmdDevicePadSignalMapItemEdit(item));
          cmd->setSignalUuid(sig->getUuid());
          mUndoStack.execCmd(cmd.release());
          return true;  // Successfully connected.
        } catch (const Exception& e) {
          QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
        }
      } else {
        return true;  // Leave unconnected.
      }
    }
  }

  return false;
}

void DevicePinoutBuilder::updateFilteredSignals() noexcept {
  const QString filter = mSignalsFilter.trimmed().toLower();

  mFilteredSignals.clear();
  QSet<const ComponentSignal*> usedSignals;
  if (filter.isEmpty()) {
    mFilteredSignals.append(std::make_pair(nullptr, false));
  }
  for (int i = 0; i < mSignals.count(); ++i) {
    auto signal = mSignals.at(i);
    if (filter.isEmpty() ||
        signal->getName()->toLower().contains(filter.toLower())) {
      for (const auto& mapItem : mList) {
        if (mapItem.getSignalUuid() == signal->getUuid()) {
          usedSignals.insert(signal.get());
          break;
        }
      }
      mFilteredSignals.append(
          std::make_pair(signal, usedSignals.contains(signal.get())));
    }
  }

  const auto pad = mPadsSorted.value(mCurrentPadIndex);
  const QString padName = pad ? pad->getName()->toLower() : QString();
  Toolbox::sortNumeric(
      mFilteredSignals,
      [&usedSignals, &filter, &padName](const QCollator& collator,
                                        const SignalChoice& a,
                                        const SignalChoice& b) {
        const QString aName = a.first ? *a.first->getName() : QString();
        const QString bName = b.first ? *b.first->getName() : QString();
        if (!padName.isEmpty()) {
          const bool aMatch = aName.toLower() == padName;
          const bool bMatch = bName.toLower() == padName;
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        if (!filter.isEmpty()) {
          const bool aMatch = aName.toLower() == filter;
          const bool bMatch = bName.toLower() == filter;
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        const bool aUsed = usedSignals.contains(a.first.get());
        const bool bUsed = usedSignals.contains(b.first.get());
        if (aUsed != bUsed) {
          return bUsed;
        }
        if (!filter.isEmpty()) {
          const bool aMatch = aName.toLower().startsWith(filter);
          const bool bMatch = bName.toLower().startsWith(filter);
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        const bool aUnconnected = !a.first;
        const bool bUnconnected = !b.first;
        if (aUnconnected != bUnconnected) {
          return aUnconnected;
        }
        return collator(aName, bName);
      },
      Qt::CaseInsensitive, false);

  std::vector<ui::DeviceInteractivePinoutSignalData> items;
  for (const auto& obj : mFilteredSignals) {
    items.push_back(ui::DeviceInteractivePinoutSignalData{
        obj.first ? q2s(*obj.first->getName()) : slint::SharedString(),  // Name
        obj.second,  // Used
    });
  }
  mFilteredSignalsModel->set_vector(items);
  mCurrentSignalIndex = 0;
}

QMap<Uuid, Uuid> DevicePinoutBuilder::getMap() const noexcept {
  QMap<Uuid, Uuid> map;
  for (auto& item : mList) {
    if (auto signalUuid = item.getSignalUuid()) {
      map.insert(item.getPadUuid(), *signalUuid);
    }
  }
  return map;
}

void DevicePinoutBuilder::setMap(const QString& cmdText,
                                 const QMap<Uuid, Uuid>& map) {
  std::unique_ptr<UndoCommandGroup> cmdGrp(new UndoCommandGroup(cmdText));
  for (auto item : mList.values()) {
    auto it = map.find(item->getPadUuid());
    const std::optional<Uuid> sig =
        (it != map.end()) ? std::make_optional(*it) : std::nullopt;
    if (item->getSignalUuid() != sig) {
      std::unique_ptr<CmdDevicePadSignalMapItemEdit> cmd(
          new CmdDevicePadSignalMapItemEdit(item));
      cmd->setSignalUuid(sig);
      cmdGrp->appendChild(cmd.release());
    }
  }
  mUndoStack.execCmd(cmdGrp.release());
}

bool DevicePinoutBuilder::askForResetFirst() noexcept {
  const QMessageBox::StandardButton btn = QMessageBox::question(
      qApp->activeWindow(), tr("Reset Pinout?"),
      tr("There are already some signals connected. Should they be "
         "disconnected before attempting to make new connections?"),
      QMessageBox::Yes | QMessageBox::No);
  return (btn == QMessageBox::Yes);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
