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
#include "shortcutsreferencegenerator.h"

#include "../editorcommandset.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

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

ShortcutsReferenceGenerator::ShortcutsReferenceGenerator(
    EditorCommandSet& commands) noexcept
  : mCommands(commands) {
}

ShortcutsReferenceGenerator::~ShortcutsReferenceGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool ShortcutsReferenceGenerator::generatePdf(const FilePath& fp) {
  FileUtils::makePath(fp.getParentDir());

  QPdfWriter writer(fp.toStr());
  writer.setCreator(QString("LibrePCB %1").arg(Application::getVersion()));
  writer.setTitle("LibrePCB Keyboard Shortcuts Reference");
  writer.setPageSize(QPageSize(QPageSize::A4));
  writer.setPageOrientation(QPageLayout::Landscape);
  const qreal marginsX = (297 - sPageWidth) / 2;
  const qreal marginsY = (210 - sPageHeight) / 2;
  writer.setPageMargins(QMarginsF(marginsX, marginsY, marginsX, marginsY),
                        QPageLayout::Millimeter);

  QPainter painter;
  if (!painter.begin(&writer)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to start PDF export - invalid output file?");
  }

  QRect imageRect(mmToPx(writer, 0), mmToPx(writer, 0), mmToPx(writer, 15),
                  mmToPx(writer, 15));
  painter.drawImage(imageRect, QImage(":/img/app/librepcb.png"));
  drawText(writer, painter, 17, 4.5, 12, 0, "LibrePCB");
  drawText(writer, painter, 17.5, 13, 3.5, 0, "Keyboard Shortcuts Reference");

  qreal x = sPageWidth - 2 * sColumnWidth - sColumnSpacing;
  qreal shortcutsWidth = sColumnWidth;
  qreal y = 5.5;
  drawSectionTitle(writer, painter, x, sPageWidth, 1.2, "Built-In");
  drawRow(writer, painter, x, y, sPageWidth - x, shortcutsWidth,
          "Switch Back to Last Used Tool", "Right Click", true);
  y += sRowHeight;
  drawRow(writer, painter, x, y, sPageWidth - x, shortcutsWidth, "Pan View",
          "Middle Click (Wheel)", false);
  y += sRowHeight;
  drawRow(writer, painter, x, y, sPageWidth - x, shortcutsWidth, "Zoom View",
          "Scroll Wheel", true);

  drawSectionTitle(writer, painter, 0, sPageWidth, 19,
                   "Configured in Workspace Settings");

  // Use manual order of categories to get a compact page layout.
  const QVector<EditorCommandCategory*> categories = {
      &mCommands.categoryEditor,  // long
      &mCommands.categoryWindowManagement,  //
      &mCommands.categoryImportExport,  //
      &mCommands.categoryModify,  // long
      &mCommands.categoryTextInput,  //
      &mCommands.categoryView,  //
      &mCommands.categoryTools,  // long
      &mCommands.categoryComponents,  //
      &mCommands.categoryDocks,  //
      &mCommands.categoryCommands,  // long
      &mCommands.categoryHelp,  //
      &mCommands.categoryContextMenu,  // Not visible
  };
  if (categories.count() != mCommands.getCategories().count()) {
    throw LogicError(
        __FILE__, __LINE__,
        "Editor command category not added to shortcuts reference export.");
  }
  x = 0;
  y = 25;
  bool layoutOverflow = false;
  for (int i = 0; i < categories.count(); ++i) {
    if (!categories.at(i)->isConfigurable()) {
      continue;
    }
    const qreal categoryHeight = sCategoryTextSize + 2 +
        mCommands.getCommands(categories.at(i)).count() * sRowHeight;
    if ((y + categoryHeight) > sPageHeight) {
      x += sColumnWidth + sColumnSpacing;
      y = 25;
      if (((y + categoryHeight) > sPageHeight) ||
          ((x + sColumnWidth) > sPageWidth)) {
        layoutOverflow = true;
      }
    }
    drawCommandCategory(writer, painter, x, y, *categories.at(i));
    y += categoryHeight + sCategorySpacing;
  }

  drawText(writer, painter, sPageWidth, sPageHeight - 1.25, 2.5, 0,
           QString("Generated by LibrePCB %1 at %2")
               .arg(Application::getVersion(),
                    QDateTime::currentDateTime().toString("yyyy-MM-dd")),
           Flag::Italic | Flag::AlignRight);

  if (!painter.end()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to finish PDF export - invalid output file?");
  }

  return !layoutOverflow;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ShortcutsReferenceGenerator::drawSectionTitle(
    QPdfWriter& writer, QPainter& painter, qreal x1, qreal x2, qreal y,
    const QString& text) const noexcept {
  int textLength = drawText(writer, painter, (x1 + x2) / 2, y - 0.25, 3, 0,
                            text, Flag::AlignCenter);

  painter.setPen(QPen(Qt::black, 1));
  const int lineLength = (mmToPx(writer, x2 - x1 - 4) - textLength) / 2;
  painter.drawLine(mmToPx(writer, x1), mmToPx(writer, y),
                   mmToPx(writer, x1) + lineLength, mmToPx(writer, y));
  painter.drawLine(mmToPx(writer, x2) - lineLength, mmToPx(writer, y),
                   mmToPx(writer, x2), mmToPx(writer, y));
}

void ShortcutsReferenceGenerator::drawCommandCategory(
    QPdfWriter& writer, QPainter& painter, qreal x, qreal y,
    EditorCommandCategory& cat) const noexcept {
  drawText(writer, painter, x, y, sCategoryTextSize, sColumnWidth,
           cat.getTextNoTr(), Flag::Bold);

  y += 0.5 + sCategoryTextSize / 2;
  painter.setPen(QPen(Qt::black, 1));
  painter.drawLine(mmToPx(writer, x), mmToPx(writer, y),
                   mmToPx(writer, x + sColumnWidth), mmToPx(writer, y));
  y += 0.5 + sRowHeight / 2;

  auto commands = mCommands.getCommands(&cat);
  for (int i = 0; i < commands.count(); ++i) {
    QStringList shortcutsStr;
    foreach (const QKeySequence& sequence, commands.at(i)->getKeySequences()) {
      shortcutsStr.append(sequence.toString(QKeySequence::NativeText));
    }
    drawRow(writer, painter, x, y, sColumnWidth, sShortcutsWidth,
            commands.at(i)->getDisplayTextNoTr(), shortcutsStr.join(" | "),
            !(i % 2));
    y += sRowHeight;
  }
}

void ShortcutsReferenceGenerator::drawRow(QPdfWriter& writer, QPainter& painter,
                                          qreal x, qreal y, qreal totalWidth,
                                          qreal shortcutsWidth,
                                          const QString& text,
                                          const QString& shortcuts,
                                          bool gray) const noexcept {
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(gray ? QColor("#d0d0d0") : Qt::transparent));
  painter.drawRect(mmToPx(writer, x), mmToPx(writer, y - sRowHeight / 2),
                   mmToPx(writer, totalWidth), mmToPx(writer, sRowHeight));
  drawText(writer, painter, x + 0.5, y, sRowTextSize,
           totalWidth - shortcutsWidth - 1, text);
  drawText(writer, painter, x + totalWidth - shortcutsWidth, y, sRowTextSize,
           shortcutsWidth, shortcuts);
}

int ShortcutsReferenceGenerator::drawText(QPdfWriter& writer, QPainter& painter,
                                          qreal x, qreal y, qreal size,
                                          qreal maxLength, const QString& text,
                                          Flags flags) const noexcept {
  QFont font = Application::getDefaultSansSerifFont();
  font.setPixelSize(mmToPx(writer, size));
  font.setBold(flags.testFlag(Flag::Bold));
  font.setItalic(flags.testFlag(Flag::Italic));
  painter.setFont(font);
  painter.setPen(QPen(Qt::black, 0));
  QRect rect(mmToPx(writer, x), mmToPx(writer, y - size),
             mmToPx(writer, maxLength), mmToPx(writer, 2 * size));
  int intFlags = Qt::TextSingleLine;
  intFlags |= Qt::AlignVCenter;
  if (flags.testFlag(Flag::AlignCenter)) {
    intFlags |= Qt::AlignHCenter;
  } else if (flags.testFlag(Flag::AlignRight)) {
    intFlags |= Qt::AlignRight;
  } else {
    intFlags |= Qt::AlignLeft;
  }
  if (maxLength == 0) {
    intFlags |= Qt::TextDontClip;
  }
  QRect boundingRect;
  painter.drawText(rect, intFlags, text, &boundingRect);
  return boundingRect.width();
}

int ShortcutsReferenceGenerator::mmToPx(QPdfWriter& writer,
                                        qreal mm) const noexcept {
  return qRound(mm * writer.resolution() / 25.4);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
