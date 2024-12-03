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
#include "packageeditorstate_renumberpads.h"

#include "../../../graphics/graphicsscene.h"
#include "../../../undocommandgroup.h"
#include "../../../undostack.h"
#include "../../../widgets/graphicsview.h"
#include "../../cmd/cmdfootprintpadedit.h"
#include "../footprintgraphicsitem.h"
#include "../footprintpadgraphicsitem.h"
#include "../packageeditorwidget.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_ReNumberPads::PackageEditorState_ReNumberPads(
    Context& context) noexcept
  : PackageEditorState(context) {
}

PackageEditorState_ReNumberPads::~PackageEditorState_ReNumberPads() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_ReNumberPads::entry() noexcept {
  // Populate command toolbar.
  std::unique_ptr<QToolButton> btnFinish(new QToolButton());
  btnFinish->setIcon(QIcon(":/img/actions/apply.png"));
  btnFinish->setText(tr("Finish"));
  btnFinish->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  connect(btnFinish.get(), &QToolButton::clicked, this,
          &PackageEditorState_ReNumberPads::finish);
  mContext.commandToolBar.addWidget(std::move(btnFinish));

  // Start undo command.
  if (!start()) {
    mContext.commandToolBar.clear();
    return false;
  }

  const QString note = " " %
      tr("(press %1 for single-selection, %2 to change numbering mode, %3 to "
         "finish)")
          .arg(QCoreApplication::translate("QShortcut", "Ctrl"))
          .arg(QCoreApplication::translate("QShortcut", "Shift"))
          .arg(QCoreApplication::translate("QShortcut", "Return"));
  emit statusBarMessageChanged(tr("Click on the next pad") % note);
  mContext.graphicsScene.setSelectionArea(QPainterPath());
  mContext.graphicsView.setCursor(Qt::PointingHandCursor);
  return true;
}

bool PackageEditorState_ReNumberPads::exit() noexcept {
  // Abort command.
  try {
    mPreviousPad.reset();
    mCurrentPad.reset();
    mTmpCmd.reset();
    if (mUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mUndoCmdActive = false;
    }
  } catch (const Exception& e) {
    qCritical() << "Could not abort command:" << e.getMsg();
  }

  mPackagePads.clear();

  // Cleanup command toolbar.
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  mContext.graphicsScene.setSelectionArea(QPainterPath());
  emit statusBarMessageChanged(QString());
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_ReNumberPads::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
  };
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_ReNumberPads::processKeyPressed(
    const QKeyEvent& e) noexcept {
  if (e.key() == Qt::Key_Return) {
    finish();
    return true;
  } else if (mCurrentModifiers != e.modifiers()) {
    mCurrentModifiers = e.modifiers();
    updateCurrentPad(true);
    return true;
  }

  return false;
}

bool PackageEditorState_ReNumberPads::processKeyReleased(
    const QKeyEvent& e) noexcept {
  return processKeyPressed(e);
}

bool PackageEditorState_ReNumberPads::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  mCurrentPos = Point::fromPx(e.scenePos());
  const bool force = (mCurrentModifiers != e.modifiers());
  mCurrentModifiers = e.modifiers();
  updateCurrentPad(force);
  return true;
}

bool PackageEditorState_ReNumberPads::
    processGraphicsSceneLeftMouseButtonPressed(
        QGraphicsSceneMouseEvent& e) noexcept {
  mCurrentPos = Point::fromPx(e.scenePos());
  const bool force = (mCurrentModifiers != e.modifiers());
  mCurrentModifiers = e.modifiers();

  commitCurrentPad();
  if (mContext.currentFootprint &&
      (mAssignedFootprintPadCount ==
       mContext.currentFootprint->getPads().count())) {
    finish();
  } else {
    updateCurrentPad(force);
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_ReNumberPads::start() noexcept {
  try {
    if (!mContext.currentFootprint) {
      return false;
    }

    // Memorize package pads, sorted by name.
    mPackagePads = mContext.package.getPads().values();
    Toolbox::sortNumeric(
        mPackagePads,
        [](const QCollator& cmp, const std::shared_ptr<PackagePad>& a,
           const std::shared_ptr<PackagePad>& b) {
          return cmp(*a->getName(), *b->getName());
        },
        Qt::CaseInsensitive, false);

    // Reset state.
    mUndoCmdActive = false;
    mAssignedFootprintPadCount = 0;
    mPreviousPad.reset();
    mCurrentPad.reset();
    mTmpCmd.reset();
    mCurrentPos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(),
                                                               true, false);
    mCurrentModifiers = Qt::KeyboardModifiers();

    // Start undo command group.
    mContext.undoStack.beginCmdGroup(tr("Re-number pads"));
    mUndoCmdActive = true;

    // Clear all pad numbers.
    for (auto& pad : mContext.currentFootprint->getPads()) {
      std::unique_ptr<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(pad));
      cmd->setPackagePadUuid(std::nullopt, true);
      mContext.undoStack.appendToCmdGroup(cmd.release());
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_ReNumberPads::updateCurrentPad(bool force) noexcept {
  try {
    if ((!mUndoCmdActive) || (!mContext.currentFootprint) ||
        (!mContext.currentGraphicsItem)) {
      return;
    }

    // Find pad under cursor.
    QList<std::shared_ptr<QGraphicsItem>> items =
        mContext.currentGraphicsItem->findItemsAtPos(
            mContext.graphicsView.calcPosWithTolerance(mCurrentPos),
            mContext.graphicsView.calcPosWithTolerance(mCurrentPos, 2),
            FootprintGraphicsItem::FindFlag::Pads |
                FootprintGraphicsItem::FindFlag::AcceptNearMatch);
    std::shared_ptr<FootprintPadGraphicsItem> pad =
        std::dynamic_pointer_cast<FootprintPadGraphicsItem>(items.value(0));

    // If no change, return.
    if ((pad == mCurrentPad) && (!force)) {
      return;
    }

    // Discard temporary changes.
    mTmpCmd.reset();
    mCurrentPad = pad;
    mContext.graphicsScene.setSelectionArea(QPainterPath());

    // If no pad selected, return.
    if ((!pad) || (pad->getObj().getPackagePadUuid())) {
      return;
    }

    // Determine area between last pad and current pad.
    const Point curPos = pad->getObj().getPosition();
    const Point prevPos =
        mPreviousPad ? mPreviousPad->getObj().getPosition() : curPos;
    const Length minX = std::min(prevPos.getX(), curPos.getX());
    const Length maxX = std::max(prevPos.getX(), curPos.getX());
    const Length minY = std::min(prevPos.getY(), curPos.getY());
    const Length maxY = std::max(prevPos.getY(), curPos.getY());

    // Find all unconnected pads in the area, sorted by their position.
    QVector<std::shared_ptr<FootprintPad>> pads =
        mContext.currentFootprint->getPads().values();
    pads.erase(
        std::remove_if(
            pads.begin(), pads.end(),
            [&](const std::shared_ptr<FootprintPad>& p) {
              const Point pos = p->getPosition();
              if (p->getPackagePadUuid()) {
                return true;
              } else if (!mPreviousPad) {
                return p.get() != &pad->getObj();
              } else if (mCurrentModifiers.testFlag(Qt::ControlModifier) &&
                         (p.get() != &pad->getObj())) {
                return true;
              } else if ((pos.getX() >= minX) && (pos.getX() <= maxX) &&
                         (pos.getY() >= minY) && (pos.getY() <= maxY)) {
                return false;
              } else {
                return true;
              }
            }),
        pads.end());
    std::sort(pads.begin(), pads.end(),
              [&](const std::shared_ptr<FootprintPad>& a,
                  const std::shared_ptr<FootprintPad>& b) {
                const Point pA = a->getPosition();
                const Point pB = b->getPosition();
                const bool invX = (prevPos.getX() > curPos.getX());
                const bool invY = (prevPos.getY() < curPos.getY());
                if (mCurrentModifiers.testFlag(Qt::ShiftModifier)) {
                  if (pA.getY() != pB.getY()) {
                    return (pA.getY() > pB.getY()) != invY;
                  } else {
                    return (pA.getX() < pB.getX()) != invX;
                  }
                } else {
                  if (pA.getX() != pB.getX()) {
                    return (pA.getX() < pB.getX()) != invX;
                  } else {
                    return (pA.getY() > pB.getY()) != invY;
                  }
                }
              });

    // Determine next unused pad number.
    int pkgPadIndex = 0;
    if ((mPreviousPad) && (mPreviousPad->getObj().getPackagePadUuid())) {
      pkgPadIndex = findIndexOfPad(*mPreviousPad->getObj().getPackagePadUuid());
      const bool keepLastIndex = mCurrentModifiers.testFlag(Qt::ShiftModifier);
      if ((pads.count() > 1) || (!keepLastIndex)) {
        ++pkgPadIndex;
      }
    }

    // Assign new pad numbers.
    mTmpCmd.reset(new UndoCommandGroup("group"));
    for (auto padPtr : pads) {
      if (std::shared_ptr<PackagePad> pkgPad =
              mPackagePads.value(pkgPadIndex)) {
        std::unique_ptr<CmdFootprintPadEdit> cmd(
            new CmdFootprintPadEdit(*padPtr));
        cmd->setPackagePadUuid(pkgPad->getUuid(), true);
        mTmpCmd->appendChild(cmd.release());
      }
      if (auto i = mContext.currentGraphicsItem->getGraphicsItem(padPtr)) {
        i->setSelected(true);
      }
      ++pkgPadIndex;
    }

  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

bool PackageEditorState_ReNumberPads::commitCurrentPad() noexcept {
  try {
    if (mCurrentPad && mTmpCmd) {
      const int count = mTmpCmd->getChildCount();
      mContext.graphicsScene.setSelectionArea(QPainterPath());
      mContext.undoStack.appendToCmdGroup(mTmpCmd.release());
      mPreviousPad = mCurrentPad;
      mCurrentPad = nullptr;
      mAssignedFootprintPadCount += count;
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_ReNumberPads::finish() noexcept {
  try {
    if (mUndoCmdActive) {
      mContext.undoStack.commitCmdGroup();
      mUndoCmdActive = false;
      emit abortRequested();
    }
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
  }
}

int PackageEditorState_ReNumberPads::findIndexOfPad(
    const Uuid& uuid) const noexcept {
  for (int i = 0; i < mPackagePads.count(); ++i) {
    if (mPackagePads.at(i)->getUuid() == uuid) {
      return i;
    }
  }
  return -1;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
