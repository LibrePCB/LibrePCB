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

#ifndef LIBREPCB_PROJECT_SCHEMATICTAB_H
#define LIBREPCB_PROJECT_SCHEMATICTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscenetab.h"

#include <librepcb/editor/dialogs/graphicsexportdialog.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;
class SchematicEditorFsm;

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
  explicit SchematicTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                        int schematicIndex, QObject* parent = nullptr) noexcept;
  virtual ~SchematicTab() noexcept;

  // General Methods
  ui::TabData getBaseUiData() const noexcept override;
  ui::SchematicTabData getUiData() const noexcept;
  void setUiData(const ui::SchematicTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  bool actionTriggered(ui::ActionId id) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;

  // Operator Overloadings
  SchematicTab& operator=(const SchematicTab& rhs) = delete;

protected:
  const LengthUnit* getCurrentUnit() const noexcept override;

private:
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;

private:
  std::shared_ptr<ProjectEditor> mEditor;
  QScopedPointer<SchematicEditorFsm> mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
