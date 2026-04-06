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

#ifndef LIBREPCB_EDITOR_PROJECTCROSSPROBE_H
#define LIBREPCB_EDITOR_PROJECTCROSSPROBE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bus;
class ComponentInstance;
class ComponentSignalInstance;
class NetSignal;

namespace editor {

class WindowTab;

/*******************************************************************************
 *  Class ProjectCrossProbe
 ******************************************************************************/

/**
 * @brief Holding the state of cross-probing objects between project tabs
 */
class ProjectCrossProbe final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectCrossProbe(const ProjectCrossProbe& other) = delete;
  explicit ProjectCrossProbe(QObject* parent = nullptr) noexcept;
  ~ProjectCrossProbe() noexcept;

  // General Methods
  void reset() noexcept;
  void set(const WindowTab* source, const QSet<const NetSignal*>& nets,
           const QSet<const ComponentInstance*>& components,
           const QSet<const ComponentSignalInstance*>& cmpSignals,
           const QSet<const Bus*>& buses) noexcept;
  bool isActive() const noexcept;
  bool isCrossProbed(const WindowTab* target) const noexcept;
  bool isSelfProbed(const WindowTab* target) const noexcept;
  bool isProbed(const NetSignal* obj) const noexcept;
  bool isProbed(const ComponentInstance* obj) const noexcept;
  bool isProbed(const ComponentSignalInstance* obj) const noexcept;
  bool isProbed(const Bus* obj) const noexcept;

  // Operator Overloadings
  ProjectCrossProbe& operator=(const ProjectCrossProbe& rhs) = delete;

signals:
  void modified();

private:
  const WindowTab* mSource;
  QSet<const NetSignal*> mNets;
  QSet<const ComponentInstance*> mComponents;
  QSet<const ComponentSignalInstance*> mComponentSignals;
  QSet<const Bus*> mBuses;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
