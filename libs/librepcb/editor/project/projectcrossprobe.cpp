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
#include "projectcrossprobe.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectCrossProbe::ProjectCrossProbe(QObject* parent) noexcept
  : QObject(parent), mSource(nullptr) {
}

ProjectCrossProbe::~ProjectCrossProbe() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectCrossProbe::reset() noexcept {
  // Do not emit modified() since this is called during ProjectEditor cleanup.
  mSource = nullptr;
  mNets.clear();
  mComponents.clear();
  mComponentSignals.clear();
  mBuses.clear();
}

void ProjectCrossProbe::set(
    const WindowTab* source, const QSet<const NetSignal*>& nets,
    const QSet<const ComponentInstance*>& components,
    const QSet<const ComponentSignalInstance*>& cmpSignals,
    const QSet<const Bus*>& buses) noexcept {
  if ((source != mSource) || (nets != mNets) || (components != mComponents) ||
      (cmpSignals != mComponentSignals) || (buses != mBuses)) {
    mSource = source;
    mNets = nets;
    mComponents = components;
    mComponentSignals = cmpSignals;
    mBuses = buses;
    emit modified();
  }
}

bool ProjectCrossProbe::isActive() const noexcept {
  return (!mNets.isEmpty()) || (!mComponents.isEmpty()) ||
      (!mComponentSignals.isEmpty()) || (!mBuses.isEmpty());
}

bool ProjectCrossProbe::isCrossProbed(const WindowTab* target) const noexcept {
  return (target != mSource) && isActive();
}

bool ProjectCrossProbe::isSelfProbed(const WindowTab* target) const noexcept {
  return (target == mSource) && isActive();
}

bool ProjectCrossProbe::isProbed(const NetSignal* obj) const noexcept {
  return obj && mNets.contains(obj);
}

bool ProjectCrossProbe::isProbed(const ComponentInstance* obj) const noexcept {
  return obj && mComponents.contains(obj);
}

bool ProjectCrossProbe::isProbed(
    const ComponentSignalInstance* obj) const noexcept {
  return obj && mComponentSignals.contains(obj);
}

bool ProjectCrossProbe::isProbed(const Bus* obj) const noexcept {
  return obj && mBuses.contains(obj);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
