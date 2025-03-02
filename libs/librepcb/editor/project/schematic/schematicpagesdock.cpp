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
#include "schematicpagesdock.h"

#include "../../editorcommandset.h"
#include "../../undostack.h"
#include "ui_schematicpagesdock.h"

#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/workspace/theme.h>

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

SchematicPagesDock::SchematicPagesDock(Project& project, UndoStack& undoStack,
                                       const Theme& theme, QWidget* parent)
  : QDockWidget(parent),
    mProject(project),
    mUndoStack(undoStack),
    mUi(new Ui::SchematicPagesDock),
    mBackgroundColor(
        theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor()),
    mScheduledThumbnailSchematics(),
    mCurrentThumbnailSchematic(std::nullopt),
    mThumbnailGenerator(),
    mThumbnailSettings(),
    mThumbnailTimer() {
  mUi->setupUi(this);

  // disable wrapping to avoid "disappearing" schematic pages, see
  // https://github.com/LibrePCB/LibrePCB/issues/681
  mUi->listWidget->setWrapping(false);

  // add all schematics to list widget
  for (int i = 0; i < mProject.getSchematics().count(); i++) schematicAdded(i);
  mUi->listWidget->setCurrentRow(-1);

  // connect signals/slots
  connect(mUi->btnNewSchematic, &QToolButton::clicked, this,
          &SchematicPagesDock::addSchematicTriggered);
  connect(mUi->btnRemoveSchematic, &QToolButton::clicked, this,
          &SchematicPagesDock::removeSelectedSchematic);
  connect(mUi->listWidget, &QListWidget::currentRowChanged, this,
          &SchematicPagesDock::selectedSchematicChanged);
  connect(&mProject, &Project::schematicAdded, this,
          &SchematicPagesDock::schematicAdded);
  connect(&mProject, &Project::schematicRemoved, this,
          &SchematicPagesDock::schematicRemoved);
  connect(&mProject, &Project::attributesChanged, this,
          &SchematicPagesDock::updateSchematicNames);

  // Add keyboard shortcuts.
  EditorCommandSet& cmd = EditorCommandSet::instance();
  mUi->listWidget->addAction(cmd.rename.createAction(
      this, this, &SchematicPagesDock::renameSelectedSchematic,
      EditorCommand::ActionFlag::WidgetShortcut));
  mUi->listWidget->addAction(cmd.remove.createAction(
      this, this, &SchematicPagesDock::removeSelectedSchematic,
      EditorCommand::ActionFlag::WidgetShortcut));

  // Setup thumbnail generator.
  mThumbnailGenerator.reset(new GraphicsExport());
  mThumbnailSettings = std::make_shared<GraphicsExportSettings>();
  mThumbnailSettings->loadColorsFromTheme(theme);
  mThumbnailSettings->setBackgroundColor(Qt::transparent);
  mThumbnailSettings->setPixmapDpi(40);
  mThumbnailSettings->setMinLineWidth(UnsignedLength(700000));
  connect(mThumbnailGenerator.data(), &GraphicsExport::previewReady, this,
          &SchematicPagesDock::thumbnailReady);
  connect(&mThumbnailTimer, &QTimer::timeout, this,
          &SchematicPagesDock::updateNextThumbnail);
  mThumbnailTimer.start(300);
}

SchematicPagesDock::~SchematicPagesDock() {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void SchematicPagesDock::setSelectedSchematic(int index) noexcept {
  mUi->listWidget->setCurrentRow(index);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void SchematicPagesDock::resizeEvent(QResizeEvent* event) noexcept {
  int iconSize = event->size().width() - 10;  // this is not good...
  mUi->listWidget->setIconSize(QSize(iconSize, iconSize));
  QDockWidget::resizeEvent(event);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SchematicPagesDock::removeSelectedSchematic() noexcept {
  emit removeSchematicTriggered(mUi->listWidget->currentRow());
}

void SchematicPagesDock::renameSelectedSchematic() noexcept {
  emit renameSchematicTriggered(mUi->listWidget->currentRow());
}

void SchematicPagesDock::schematicAdded(int newIndex) noexcept {
  Schematic* schematic = mProject.getSchematicByIndex(newIndex);
  Q_ASSERT(schematic);
  if (!schematic) return;

  QListWidgetItem* item = new QListWidgetItem();
  item->setText(QString("%1: %2").arg(newIndex + 1).arg(*schematic->getName()));
  item->setData(Qt::UserRole, schematic->getUuid().toStr());
  mUi->listWidget->insertItem(newIndex, item);

  mSchematicConnections.insert(
      newIndex,
      {
          connect(schematic, &Schematic::symbolAdded, this,
                  &SchematicPagesDock::schematicModified),
          connect(schematic, &Schematic::symbolRemoved, this,
                  &SchematicPagesDock::schematicModified),
      });

  mScheduledThumbnailSchematics.insert(schematic->getUuid());
}

void SchematicPagesDock::schematicRemoved(int oldIndex) noexcept {
  foreach (auto connection, mSchematicConnections.takeAt(oldIndex)) {
    disconnect(connection);
  }

  delete mUi->listWidget->item(oldIndex);
}

void SchematicPagesDock::schematicModified(SI_Symbol& symbol) noexcept {
  mScheduledThumbnailSchematics.insert(symbol.getSchematic().getUuid());
}

void SchematicPagesDock::updateSchematicNames() noexcept {
  for (int i = 0; i < mUi->listWidget->count(); ++i) {
    QListWidgetItem* item = mUi->listWidget->item(i);
    const Schematic* schematic = mProject.getSchematicByIndex(i);
    if (item && schematic) {
      item->setText(QString("%1: %2").arg(i + 1).arg(*schematic->getName()));
    }
  }
}

void SchematicPagesDock::updateNextThumbnail() noexcept {
  if (mCurrentThumbnailSchematic) {
    return;  // Still busy.
  }

  if (mScheduledThumbnailSchematics.isEmpty()) {
    return;  // Nothing to do.
  }

  if (mUndoStack.isCommandGroupActive()) {
    return;  // Too annoying while the user is doing something.
  }

  const Uuid schematicUuid = *mScheduledThumbnailSchematics.begin();
  mScheduledThumbnailSchematics -= schematicUuid;

  if (const Schematic* schematic = mProject.getSchematicByUuid(schematicUuid)) {
    qDebug().noquote() << "Generating thumbnail of schematic:"
                       << schematicUuid.toStr();
    mCurrentThumbnailSchematic = schematicUuid;
    std::shared_ptr<GraphicsPagePainter> painter =
        std::make_shared<SchematicPainter>(*schematic, true);
    GraphicsExport::Pages pages = {
        std::make_pair(painter, mThumbnailSettings),
    };
    mThumbnailGenerator->startPreview(pages);
  }
}

void SchematicPagesDock::thumbnailReady(int index, const QSize& pageSize,
                                        const QRectF margins,
                                        std::shared_ptr<QPicture> picture) {
  Q_UNUSED(index);
  Q_UNUSED(margins);
  Q_ASSERT(picture);
  if (mCurrentThumbnailSchematic) {
    const Uuid uuid = *mCurrentThumbnailSchematic;
    for (int i = 0; i < mUi->listWidget->count(); ++i) {
      QListWidgetItem* item = mUi->listWidget->item(i);
      Q_ASSERT(item);
      if (item->data(Qt::UserRole).toString() == uuid.toStr()) {
        QPixmap pixmap(pageSize.expandedTo(QSize(250, 100)));
        pixmap.fill(mBackgroundColor);
        QPainter painter(&pixmap);
        picture->play(&painter);
        item->setIcon(pixmap);
        qDebug().noquote() << "Schematic thumbnail updated:" << uuid.toStr();
        // Workaround for broken list widget layout update.
        mUi->listWidget->setSpacing(1);
        mUi->listWidget->setSpacing(0);
        break;
      }
    }
    mCurrentThumbnailSchematic = std::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
