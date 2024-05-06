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
#include "editortoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool EditorToolbox::isWindowBackgroundDark() noexcept {
  auto detect = []() {
    QImage image(10, 10, QImage::Format_ARGB32);
    image.fill(qApp->palette().color(QPalette::Window));  // Fallback
    QWidget widget;
    widget.resize(image.size());
    widget.render(&image);
    const QColor bgColor = image.pixelColor(image.rect().center());
    const bool dark = (bgColor.alphaF() > 0.2) && (bgColor.blackF() > 0.5);
    qDebug().nospace().noquote()
        << "Detected " << (dark ? "dark" : "light")
        << " theme based on window background color " << bgColor.name() << ".";
    return dark;
  };
  static bool value = detect();
  return value;
}

void EditorToolbox::removeFormLayoutRow(QLabel& label) noexcept {
  if (auto layout = label.parentWidget()->layout()) {
    if (removeFormLayoutRow(*layout, label)) {
      return;
    }
  }
  qWarning().nospace() << "Failed to remove form layout row "
                       << label.objectName() << ".";
}

void EditorToolbox::deleteLayoutItemRecursively(QLayoutItem* item) noexcept {
  Q_ASSERT(item);
  if (QWidget* widget = item->widget()) {
    delete widget;
  } else if (QLayout* layout = item->layout()) {
    for (int i = layout->count() - 1; i >= 0; --i) {
      deleteLayoutItemRecursively(layout->takeAt(i));
    }
  } else if (QSpacerItem* spacer = item->spacerItem()) {
    delete spacer;
  }
  delete item;
}

bool EditorToolbox::startToolBarTabFocusCycle(
    QToolBar& toolBar, QWidget& returnFocusToWidget) noexcept {
  QWidget* previousWidget = nullptr;
  foreach (QAction* action, toolBar.actions()) {
    QWidget* widget = toolBar.widgetForAction(action);
    if (widget && (widget->focusPolicy() & Qt::TabFocus)) {
      if (!previousWidget) {
        widget->setFocus(Qt::TabFocusReason);
      } else {
        toolBar.setTabOrder(previousWidget, widget);
      }
      previousWidget = widget;
    }
  }
  if (previousWidget) {
    toolBar.setTabOrder(previousWidget, &returnFocusToWidget);
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool EditorToolbox::removeFormLayoutRow(QLayout& layout,
                                        QLabel& label) noexcept {
  if (auto formLayout = dynamic_cast<QFormLayout*>(&layout)) {
    for (int i = 0; i < formLayout->rowCount(); ++i) {
      QLayoutItem* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
      QLayoutItem* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
      if ((labelItem) && (labelItem->widget() == &label) && (fieldItem)) {
        hideLayoutItem(*labelItem);
        hideLayoutItem(*fieldItem);
        formLayout->takeRow(i);  // Avoid ugly space caused by the empty rows.
        return true;
      }
    }
  }
  for (int i = 0; i < layout.count(); ++i) {
    if (QLayoutItem* item = layout.itemAt(i)) {
      if (QLayout* child = item->layout()) {
        if (removeFormLayoutRow(*child, label)) {
          return true;
        }
      }
    }
  }
  return false;
}

void EditorToolbox::hideLayoutItem(QLayoutItem& item) noexcept {
  if (QWidget* widget = item.widget()) {
    widget->hide();
  } else if (QLayout* layout = item.layout()) {
    for (int i = 0; i < layout->count(); ++i) {
      if (QLayoutItem* child = layout->itemAt(i)) {
        hideLayoutItem(*child);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
