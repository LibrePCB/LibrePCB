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

#ifndef LIBREPCB_SCHEMATICTAB_H
#define LIBREPCB_SCHEMATICTAB_H

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
namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class SchematicTab
 ******************************************************************************/

/**
 * @brief The SchematicTab class
 */
class SchematicTab final : public GraphicsSceneTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicTab() = delete;
  SchematicTab(const SchematicTab& other) = delete;
  explicit SchematicTab(GuiApplication& app,int id, std::shared_ptr<ProjectEditor> prj,
                        int schematicIndex, QObject* parent = nullptr) noexcept;
  virtual ~SchematicTab() noexcept;

  // General Methods
  void activate() noexcept override;
  void deactivate() noexcept override;

  // Operator Overloadings
  SchematicTab& operator=(const SchematicTab& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
