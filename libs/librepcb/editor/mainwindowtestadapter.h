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

#ifndef LIBREPCB_EDITOR_MAINWINDOWTESTADAPTER_H
#define LIBREPCB_EDITOR_MAINWINDOWTESTADAPTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class MainWindow;

/*******************************************************************************
 *  Class MainWindowTestAdapter
 ******************************************************************************/

/**
 * @brief Adapter class for automated GUI tests
 */
class MainWindowTestAdapter final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  MainWindowTestAdapter() = delete;
  MainWindowTestAdapter(const MainWindowTestAdapter& other) = delete;
  explicit MainWindowTestAdapter(GuiApplication& app, MainWindow& win,
                                 QWidget* parent = nullptr) noexcept;
  ~MainWindowTestAdapter() noexcept;

  // Operator Overloadings
  MainWindowTestAdapter& operator=(const MainWindowTestAdapter& rhs) = delete;

public slots:
  QVariant trigger(QVariant action) noexcept;
  QVariant isLibraryScanFinished(QVariant) const noexcept {
    return mLibraryScanFinished;
  }
  QVariant getOpenProjects(QVariant) const noexcept;

signals:
  void actionTriggered(ui::Action a);

private:
  GuiApplication& mApp;
  MainWindow& mWindow;
  bool mLibraryScanFinished = false;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
