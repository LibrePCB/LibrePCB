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

#ifndef LIBREPCB_EDITOR_SCHEMATICPAGESDOCK_H
#define LIBREPCB_EDITOR_SCHEMATICPAGESDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsExport;
class GraphicsExportSettings;
class Project;
class SI_Symbol;
class Theme;

namespace editor {

class UndoStack;

namespace Ui {
class SchematicPagesDock;
}

/*******************************************************************************
 *  Class SchematicPagesDock
 ******************************************************************************/

/**
 * @brief The SchematicPagesDock class
 */
class SchematicPagesDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicPagesDock() = delete;
  SchematicPagesDock(const SchematicPagesDock& other) = delete;
  SchematicPagesDock(Project& project, UndoStack& undoStack, const Theme& theme,
                     QWidget* parent = nullptr);
  ~SchematicPagesDock();

  // General Methods
  void setBackgroundColor(const QColor& c) noexcept { mBackgroundColor = c; }
  void setSelectedSchematic(int index) noexcept;

  // Operator Overloadings
  SchematicPagesDock& operator=(const SchematicPagesDock& rhs) = delete;

signals:
  void selectedSchematicChanged(int index);
  void addSchematicTriggered();
  void removeSchematicTriggered(int index);
  void renameSchematicTriggered(int index);

protected:
  void resizeEvent(QResizeEvent* event) noexcept override;

private:  // Methods
  void removeSelectedSchematic() noexcept;
  void renameSelectedSchematic() noexcept;
  void schematicAdded(int newIndex) noexcept;
  void schematicRemoved(int oldIndex) noexcept;
  void schematicModified(SI_Symbol& symbol) noexcept;
  void updateSchematicNames() noexcept;
  void updateNextThumbnail() noexcept;
  void thumbnailReady(int index, const QSize& pageSize, const QRectF margins,
                      std::shared_ptr<QPicture> picture);

private:  // Data
  Project& mProject;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::SchematicPagesDock> mUi;
  QColor mBackgroundColor;

  // Thumbnail generator.
  QSet<Uuid> mScheduledThumbnailSchematics;
  tl::optional<Uuid> mCurrentThumbnailSchematic;
  QScopedPointer<GraphicsExport> mThumbnailGenerator;
  std::shared_ptr<GraphicsExportSettings> mThumbnailSettings;
  QTimer mThumbnailTimer;
  QVector<QVector<QMetaObject::Connection>> mSchematicConnections;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
