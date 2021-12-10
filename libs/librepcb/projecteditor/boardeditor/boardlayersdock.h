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

#ifndef LIBREPCB_PROJECTEDITOR_BOARDLAYERSDOCK_H
#define LIBREPCB_PROJECTEDITOR_BOARDLAYERSDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;

namespace project {

class Board;
class ComponentInstance;
class Project;

namespace editor {

class BoardEditor;

namespace Ui {
class BoardLayersDock;
}

/*******************************************************************************
 *  Class BoardLayersDock
 ******************************************************************************/

/**
 * @brief The BoardLayersDock class
 */
class BoardLayersDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardLayersDock() = delete;
  BoardLayersDock(const BoardLayersDock& other) = delete;
  explicit BoardLayersDock(BoardEditor& editor) noexcept;
  ~BoardLayersDock() noexcept;

  // Setters
  void setActiveBoard(Board* board);

  // Operator Overloadings
  BoardLayersDock& operator=(const BoardLayersDock& rhs) = delete;

private slots:

  void on_listWidget_itemChanged(QListWidgetItem* item);
  void on_btnTop_clicked();
  void on_btnBottom_clicked();
  void on_btnTopBottom_clicked();
  void on_btnAll_clicked();
  void on_btnNone_clicked();

private:
  // Private Methods
  void updateListWidget() noexcept;
  void setVisibleLayers(const QList<QString>& layers) noexcept;
  QList<QString> getCommonLayers() const noexcept;
  QList<QString> getTopLayers() const noexcept;
  QList<QString> getBottomLayers() const noexcept;
  QList<QString> getAllLayers() const noexcept;

  // General
  QScopedPointer<Ui::BoardLayersDock> mUi;
  BoardEditor& mBoardEditor;
  Board* mActiveBoard;
  QMetaObject::Connection mActiveBoardConnection;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
