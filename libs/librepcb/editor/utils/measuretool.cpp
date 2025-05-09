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
#include "measuretool.h"

#include "../editorcommandset.h"
#include "../graphics/graphicsscene.h"
#include "../widgets/graphicsview.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/schematic/items/si_netlabel.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_polygon.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/utils/transform.h>

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

MeasureTool::MeasureTool(QObject* parent) noexcept
  : QObject(parent),
    mScene(),
    mUnit(LengthUnit::millimeters()),
    mSnapCandidates(),
    mLastScenePos(),
    mCursorPos(),
    mCursorSnapped(false),
    mStartPos(),
    mEndPos() {
}

MeasureTool::~MeasureTool() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void MeasureTool::setSymbol(const Symbol* symbol) noexcept {
  mSnapCandidates.clear();
  if (symbol) {
    mSnapCandidates |= snapCandidatesFromSymbol(*symbol, Transform());
  }
}

void MeasureTool::setFootprint(const Footprint* footprint) noexcept {
  mSnapCandidates.clear();
  if (footprint) {
    mSnapCandidates |= snapCandidatesFromFootprint(*footprint, Transform());
  }
}

void MeasureTool::setSchematic(const Schematic* schematic) noexcept {
  mSnapCandidates.clear();
  if (schematic) {
    foreach (const SI_Symbol* symbol, schematic->getSymbols()) {
      mSnapCandidates.insert(symbol->getPosition());
      mSnapCandidates |=
          snapCandidatesFromSymbol(symbol->getLibSymbol(), Transform(*symbol));
    }
    foreach (const SI_NetSegment* segment, schematic->getNetSegments()) {
      foreach (const SI_NetPoint* netpoint, segment->getNetPoints()) {
        mSnapCandidates.insert(netpoint->getPosition());
      }
      foreach (const SI_NetLabel* netlabel, segment->getNetLabels()) {
        mSnapCandidates.insert(netlabel->getPosition());
      }
    }
    foreach (const SI_Polygon* polygon, schematic->getPolygons()) {
      mSnapCandidates |=
          snapCandidatesFromPath(polygon->getPolygon().getPath());
    }
    foreach (const SI_Text* text, schematic->getTexts()) {
      mSnapCandidates.insert(text->getPosition());
    }
  }
}

void MeasureTool::setBoard(const Board* board) noexcept {
  mSnapCandidates.clear();
  if (board) {
    foreach (const BI_Device* device, board->getDeviceInstances()) {
      mSnapCandidates.insert(device->getPosition());
      mSnapCandidates |= snapCandidatesFromFootprint(device->getLibFootprint(),
                                                     Transform(*device));
    }
    foreach (const BI_NetSegment* segment, board->getNetSegments()) {
      foreach (const BI_NetPoint* netpoint, segment->getNetPoints()) {
        mSnapCandidates.insert(netpoint->getPosition());
      }
      foreach (const BI_Via* via, segment->getVias()) {
        mSnapCandidates.insert(via->getPosition());
        Path path = via->getVia().getOutline();
        path.addVertex(Point(via->getSize() / 2, 0));
        path.addVertex(Point(-via->getSize() / 2, 0));
        path.addVertex(Point(0, via->getSize() / 2));
        path.addVertex(Point(0, -via->getSize() / 2));
        mSnapCandidates |=
            snapCandidatesFromPath(path.translated(via->getPosition()));
        mSnapCandidates |= snapCandidatesFromCircle(via->getPosition(),
                                                    *via->getDrillDiameter());
      }
    }
    foreach (const BI_Plane* plane, board->getPlanes()) {
      mSnapCandidates |= snapCandidatesFromPath(plane->getOutline());
      foreach (const Path& fragment, plane->getFragments()) {
        mSnapCandidates |= snapCandidatesFromPath(fragment);
      }
    }
    foreach (const BI_Polygon* polygon, board->getPolygons()) {
      mSnapCandidates |= snapCandidatesFromPath(polygon->getData().getPath());
    }
    foreach (const BI_StrokeText* text, board->getStrokeTexts()) {
      mSnapCandidates.insert(text->getData().getPosition());
    }
    foreach (const BI_Hole* hole, board->getHoles()) {
      foreach (const Vertex& vertex, hole->getData().getPath()->getVertices()) {
        mSnapCandidates |= snapCandidatesFromCircle(
            vertex.getPos(), *hole->getData().getDiameter());
      }
    }
  }
}

void MeasureTool::enter(GraphicsScene& scene, const LengthUnit& unit,
                        const Point& pos) noexcept {
  mScene = &scene;
  mUnit = unit;
  mLastScenePos = pos;

  mScene->setSelectionArea(QPainterPath());  // clear selection
  mScene->setGrayOut(true);

  updateCursorPosition(Qt::KeyboardModifier::NoModifier);
  updateRulerPositions();
  updateStatusBarMessage();
}

void MeasureTool::leave() noexcept {
  // Note: Do not clear the current start/end points to make the ruler
  // re-appear on the same coordinates when re-entering this tool some time
  // later. This might be useful in some cases to avoid needing to measure the
  // same distance again.

  if (mScene) {
    mScene->setSceneCursor(Point(), false, false);
    mScene->setRulerPositions(std::nullopt);
    mScene->setGrayOut(false);
  }

  emit infoBoxTextChanged(QString());
  emit statusBarMessageChanged(QString());
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool MeasureTool::processKeyPressed(int key,
                                    Qt::KeyboardModifiers modifiers) noexcept {
  if (key == Qt::Key_Shift) {
    updateCursorPosition(modifiers);
    return true;
  }

  return false;
}

bool MeasureTool::processKeyReleased(int key,
                                     Qt::KeyboardModifiers modifiers) noexcept {
  if (key == Qt::Key_Shift) {
    updateCursorPosition(modifiers);
    return true;
  }

  return false;
}

bool MeasureTool::processGraphicsSceneMouseMoved(
    const Point& pos, Qt::KeyboardModifiers modifiers) noexcept {
  mLastScenePos = pos;
  updateCursorPosition(modifiers);
  return true;
}

bool MeasureTool::processGraphicsSceneLeftMouseButtonPressed() noexcept {
  if ((!mStartPos) || mEndPos) {
    // Set first point.
    mStartPos = mCursorPos;
    mEndPos = std::nullopt;
  } else {
    // Set second point.
    mEndPos = mCursorPos;
  }

  updateRulerPositions();
  updateStatusBarMessage();

  return true;
}

bool MeasureTool::processCopy() noexcept {
  if (mStartPos && mEndPos) {
    const Point diff = (*mEndPos) - (*mStartPos);
    const qreal value = mUnit.convertToUnit(*diff.getLength());
    const QString str = Toolbox::floatToString(value, 12, QLocale());
    qApp->clipboard()->setText(str);
    emit statusBarMessageChanged(tr("Copied to clipboard: %1").arg(str), 3000);
    return true;
  }

  return false;
}

bool MeasureTool::processRemove() noexcept {
  if (mStartPos && mEndPos) {
    mStartPos = std::nullopt;
    mEndPos = std::nullopt;
    updateRulerPositions();
    updateStatusBarMessage();
    return true;
  }

  return false;
}

bool MeasureTool::processAbortCommand() noexcept {
  if (mStartPos && (!mEndPos)) {
    mStartPos = std::nullopt;
    updateRulerPositions();
    updateStatusBarMessage();
    return true;
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QSet<Point> MeasureTool::snapCandidatesFromSymbol(
    const Symbol& symbol, const Transform& transform) noexcept {
  QSet<Point> candidates;
  for (const SymbolPin& p : symbol.getPins()) {
    candidates.insert(transform.map(p.getPosition()));
    candidates.insert(transform.map(
        p.getPosition() + Point(*p.getLength(), 0).rotated(p.getRotation())));
  }
  for (const Polygon& p : symbol.getPolygons()) {
    candidates |= snapCandidatesFromPath(transform.map(p.getPath()));
  }
  for (const Circle& c : symbol.getCircles()) {
    candidates |= snapCandidatesFromCircle(transform.map(c.getCenter()),
                                           *c.getDiameter());
  }
  for (const Text& s : symbol.getTexts()) {
    candidates.insert(transform.map(s.getPosition()));
  }
  return candidates;
}

QSet<Point> MeasureTool::snapCandidatesFromFootprint(
    const Footprint& footprint, const Transform& transform) noexcept {
  QSet<Point> candidates;
  for (const FootprintPad& p : footprint.getPads()) {
    candidates.insert(transform.map(p.getPosition()));
    try {
      foreach (const Path& outline, p.getGeometry().toOutlines()) {
        candidates |= snapCandidatesFromPath(transform.map(
            outline.rotated(p.getRotation()).translated(p.getPosition())));
      }
    } catch (const Exception& e) {
      qWarning() << "Failed to determine snap candidates:" << e.getMsg();
    }
    const Transform padTransform(p.getPosition(), p.getRotation());
    for (const PadHole& h : p.getHoles()) {
      foreach (const Vertex& vertex,
               padTransform.map(h.getPath())->getVertices()) {
        candidates |= snapCandidatesFromCircle(transform.map(vertex.getPos()),
                                               *h.getDiameter());
      }
    }
  }
  for (const Polygon& p : footprint.getPolygons()) {
    candidates |= snapCandidatesFromPath(transform.map(p.getPath()));
  }
  for (const Circle& c : footprint.getCircles()) {
    candidates |= snapCandidatesFromCircle(transform.map(c.getCenter()),
                                           *c.getDiameter());
  }
  for (const StrokeText& s : footprint.getStrokeTexts()) {
    candidates.insert(transform.map(s.getPosition()));
  }
  for (const Hole& h : footprint.getHoles()) {
    foreach (const Vertex& vertex, h.getPath()->getVertices()) {
      candidates |= snapCandidatesFromCircle(transform.map(vertex.getPos()),
                                             *h.getDiameter());
    }
  }
  return candidates;
}

QSet<Point> MeasureTool::snapCandidatesFromPath(const Path& path) noexcept {
  QSet<Point> candidates;
  for (int i = 0; i < path.getVertices().count(); ++i) {
    const Vertex& v = path.getVertices().at(i);
    candidates.insert(v.getPos());
    if ((v.getAngle().abs() == Angle::deg180()) &&
        (i < (path.getVertices().count() - 1))) {
      const Point& p2 = path.getVertices().at(i + 1).getPos();
      const Point center = (v.getPos() + p2) / 2;
      const Point middle = v.getPos().rotated(v.getAngle() / 2, center);
      candidates.insert(middle);
    }
  }
  return candidates;
}

QSet<Point> MeasureTool::snapCandidatesFromCircle(
    const Point& center, const Length& diameter) noexcept {
  QSet<Point> candidates;
  candidates.insert(center);
  candidates.insert(center + Point(0, diameter / 2));
  candidates.insert(center + Point(0, -diameter / 2));
  candidates.insert(center + Point(diameter / 2, 0));
  candidates.insert(center + Point(-diameter / 2, 0));
  return candidates;
}

void MeasureTool::updateCursorPosition(
    Qt::KeyboardModifiers modifiers) noexcept {
  if (!mScene) {
    return;
  }

  mCursorPos = mLastScenePos;
  mCursorSnapped = false;
  if (!modifiers.testFlag(Qt::ShiftModifier)) {
    Point nearestCandidate;
    Length nearestDistance(-1);
    foreach (const Point& candidate, mSnapCandidates) {
      const UnsignedLength distance = (mCursorPos - candidate).getLength();
      if ((nearestDistance < 0) || (distance < nearestDistance)) {
        nearestCandidate = candidate;
        nearestDistance = *distance;
      }
    }

    const Point posOnGrid = mCursorPos.mappedToGrid(mScene->getGridInterval());
    const Length gridDistance = *(mCursorPos - posOnGrid).getLength();
    if ((nearestDistance >= 0) && (nearestDistance <= gridDistance)) {
      mCursorPos = nearestCandidate;
      mCursorSnapped = true;
    } else {
      mCursorPos = posOnGrid;
    }
  }
  updateRulerPositions();
}

void MeasureTool::updateRulerPositions() noexcept {
  if (!mScene) {
    return;
  }

  mScene->setSceneCursor(mCursorPos, (!mStartPos) || mEndPos, mCursorSnapped);

  const Point startPos = mStartPos ? *mStartPos : mCursorPos;
  const Point endPos = mEndPos ? *mEndPos : mCursorPos;
  if (mStartPos) {
    mScene->setRulerPositions(std::make_pair(startPos, endPos));
  } else {
    mScene->setRulerPositions(std::nullopt);
  }

  const Point diff = endPos - startPos;
  const UnsignedLength length = diff.getLength();
  const Angle angle = Angle::fromRad(
      std::atan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()));
  int decimals = mUnit.getReasonableNumberOfDecimals() + 1;

  QString text;
  text += QString("X0: %1 %2<br>")
              .arg(mUnit.convertToUnit(startPos.getX()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("Y0: %1 %2<br>")
              .arg(mUnit.convertToUnit(startPos.getY()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("X1: %1 %2<br>")
              .arg(mUnit.convertToUnit(endPos.getX()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("Y1: %1 %2<br>")
              .arg(mUnit.convertToUnit(endPos.getY()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("<br>");
  text += QString("ΔX: %1 %2<br>")
              .arg(mUnit.convertToUnit(diff.getX()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("ΔY: %1 %2<br>")
              .arg(mUnit.convertToUnit(diff.getY()), 10, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("<br>");
  text += QString("<b>Δ: %1 %2</b><br>")
              .arg(mUnit.convertToUnit(*length), 11, 'f', decimals)
              .arg(mUnit.toShortStringTr());
  text += QString("<b>∠: %1°</b>").arg(angle.toDeg(), 14 - decimals, 'f', 3);
  text.replace(" ", "&nbsp;");
  emit infoBoxTextChanged(text);
}

void MeasureTool::updateStatusBarMessage() noexcept {
  const QList<QKeySequence> copyKeys =
      EditorCommandSet::instance().clipboardCopy.getKeySequences();
  const QList<QKeySequence> deleteKeys =
      EditorCommandSet::instance().remove.getKeySequences();
  const QString disableSnapNote = " " %
      tr("(press %1 to disable snap)")
          .arg(QCoreApplication::translate("QShortcut", "Shift"));

  if (mEndPos && (!copyKeys.isEmpty()) && (!deleteKeys.isEmpty())) {
    emit statusBarMessageChanged(
        tr("Press %1 to copy the value to clipboard or %2 to clear the "
           "measurement")
            .arg(copyKeys.first().toString(QKeySequence::NativeText))
            .arg(deleteKeys.first().toString(QKeySequence::NativeText)));
  } else if (mStartPos && (!mEndPos)) {
    emit statusBarMessageChanged(tr("Click to specify the end point") %
                                 disableSnapNote);
  } else {
    emit statusBarMessageChanged(tr("Click to specify the start point") %
                                 disableSnapNote);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
