/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_EXCLUSIVEACTIONGROUP_H
#define LIBREPCB_EXCLUSIVEACTIONGROUP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ExclusiveActionGroup
 ******************************************************************************/

/**
 * @brief The ExclusiveActionGroup class groups multiple QAction's together
 *
 * This class is basically the same as QActionGroup
 * (http://doc.qt.io/qt-5/qactiongroup.html). But there is one important
 * difference: When the user clicks on a QAction, that action won't be checked
 * instantly. Instead, this class only emits the signal
 * #changeRequestTriggered(). Whether the triggered action actually gets checked
 * or the request is rejected can be decided from outside this class (typically
 * by the state machine of an editor window). To change the selected action,
 * #setCurrentAction() needs to be called.
 *
 * @author ubruhin
 * @date 2016-11-29
 */
class ExclusiveActionGroup final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ExclusiveActionGroup() noexcept;
  ExclusiveActionGroup(const ExclusiveActionGroup& other) = delete;
  ~ExclusiveActionGroup() noexcept;

  // General Methods
  void            reset() noexcept;
  void            setEnabled(bool enabled) noexcept;
  void            addAction(const QVariant& key, QAction* action) noexcept;
  void            setActionEnabled(const QVariant& key, bool enabled) noexcept;
  void            setCurrentAction(const QVariant& key) noexcept;
  const QVariant& getCurrentAction() const noexcept { return mCurrentAction; }

  // Operator Overloadings
  ExclusiveActionGroup& operator=(const ExclusiveActionGroup& rhs) = delete;

signals:
  void changeRequestTriggered(const QVariant& key);

private:  // Methods
  void actionTriggered() noexcept;

private:  // Data
  QVariant                 mCurrentAction;
  QMap<QVariant, QAction*> mActions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_EXCLUSIVEACTIONGROUP_H
