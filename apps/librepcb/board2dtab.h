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

#ifndef LIBREPCB_BOARD2DTAB_H
#define LIBREPCB_BOARD2DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscenetab.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class Board2dTab
 ******************************************************************************/

/**
 * @brief The Board2dTab class
 */
class Board2dTab final : public GraphicsSceneTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  Board2dTab() = delete;
  Board2dTab(const Board2dTab& other) = delete;
  explicit Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                      int boardIndex, QObject* parent = nullptr) noexcept;
  virtual ~Board2dTab() noexcept;

  // General Methods
  const ui::Board2dTabData& getUiData() const noexcept { return mUiData; }
  void setUiData(const ui::Board2dTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;

  // Operator Overloadings
  Board2dTab& operator=(const Board2dTab& rhs) = delete;

private:
  ui::Board2dTabData mUiData;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
