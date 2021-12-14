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

#include "ui_schematicpagesdock.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>

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

SchematicPagesDock::SchematicPagesDock(Project& project, QWidget* parent)
  : QDockWidget(parent), mProject(project), mUi(new Ui::SchematicPagesDock) {
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

  // install event filter on the list widget to implement keyboard shortcuts
  mUi->listWidget->installEventFilter(this);
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

bool SchematicPagesDock::eventFilter(QObject* obj, QEvent* event) noexcept {
  if (event->type() == QEvent::ShortcutOverride) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    switch (keyEvent->key()) {
      case Qt::Key_Delete: {
        removeSelectedSchematic();
        event->accept();
        return true;
      }
      case Qt::Key_F2: {
        renameSelectedSchematic();
        event->accept();
        return true;
      }
      default:
        break;
    }
  }
  return QDockWidget::eventFilter(obj, event);
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
  item->setIcon(schematic->getIcon());
  mUi->listWidget->insertItem(newIndex, item);
}

void SchematicPagesDock::schematicRemoved(int oldIndex) noexcept {
  delete mUi->listWidget->item(oldIndex);
}

void SchematicPagesDock::updateSchematicNames() noexcept {
  for (int i = 0; i < mUi->listWidget->count(); ++i) {
    QListWidgetItem* item = mUi->listWidget->item(i);
    const Schematic* schematic = mProject.getSchematicByIndex(i);
    if (item && schematic) {
      item->setText(*schematic->getName());
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
