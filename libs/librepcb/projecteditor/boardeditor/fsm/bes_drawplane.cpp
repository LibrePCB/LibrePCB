/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bes_drawplane.h"
#include "../boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcb/common/undostack.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/project/project.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/boards/cmd/cmdboardplaneadd.h>
#include <librepcb/project/boards/cmd/cmdboardplaneedit.h>
#include "../boardeditor.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BES_DrawPlane::BES_DrawPlane(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    BES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mSubState(SubState::Idle), mCurrentNetSignal(nullptr),
    mCurrentLayerName(GraphicsLayer::sTopCopper),
    mCmdEditCurrentPlane(nullptr),
    // command toolbar actions / widgets:
    mNetSignalLabel(nullptr), mNetSignalComboBox(nullptr), mLayerLabel(nullptr),
    mLayerComboBox(nullptr)
{
}

BES_DrawPlane::~BES_DrawPlane()
{
    Q_ASSERT(mSubState == SubState::Idle);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_DrawPlane::process(BEE_Base* event) noexcept
{
    switch (mSubState) {
        case SubState::Idle:
            return processSubStateIdle(event);
        case SubState::Positioning:
            return processSubStatePositioning(event);
        default:
            Q_ASSERT(false);
            return PassToParentState;
    }
}

bool BES_DrawPlane::entry(BEE_Base* event) noexcept
{
    Q_UNUSED(event);
    Q_ASSERT(mSubState == SubState::Idle);

    // clear board selection because selection does not make sense in this state
    if (mEditor.getActiveBoard()) mEditor.getActiveBoard()->clearSelection();

    // get most used net signalmNetSignalComboBox
    if (!mCurrentNetSignal) {
        mCurrentNetSignal = mCircuit.getNetSignalWithMostElements();
    }
    if (!mCurrentNetSignal) return false;

    // add the "Signal:" label to the toolbar
    mNetSignalLabel = new QLabel(tr("Signal:"));
    mNetSignalLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mNetSignalLabel);

    // add the netsignals combobox to the toolbar
    mNetSignalComboBox = new QComboBox();
    mNetSignalComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mNetSignalComboBox->setInsertPolicy(QComboBox::NoInsert);
    mNetSignalComboBox->setEditable(false);
    foreach (NetSignal* netsignal, mEditor.getProject().getCircuit().getNetSignals())
        mNetSignalComboBox->addItem(*netsignal->getName(), netsignal->getUuid().toStr());
    mNetSignalComboBox->model()->sort(0);
    mNetSignalComboBox->setCurrentText(mCurrentNetSignal ? *mCurrentNetSignal->getName() : "");
    mEditorUi.commandToolbar->addWidget(mNetSignalComboBox);
    connect(mNetSignalComboBox, &QComboBox::currentTextChanged,
            [this](const QString& value)
            {setNetSignal(mEditor.getProject().getCircuit().getNetSignalByName(value));});

    // add the "Layer:" label to the toolbar
    mLayerLabel = new QLabel(tr("Layer:"));
    mLayerLabel->setIndent(10);
    mEditorUi.commandToolbar->addWidget(mLayerLabel);

    // add the layers combobox to the toolbar
    mLayerComboBox = new GraphicsLayerComboBox();
    if (mEditor.getActiveBoard()) {
        QList<GraphicsLayer*> layers;
        foreach (GraphicsLayer* layer, mEditor.getActiveBoard()->getLayerStack().getAllLayers()) {
            if (layer->isCopperLayer() && layer->isEnabled()) {
                layers.append(layer);
            }
        }
        mLayerComboBox->setLayers(layers);
    }
    mLayerComboBox->setCurrentLayer(*mCurrentLayerName);
    mEditorUi.commandToolbar->addWidget(mLayerComboBox);
    connect(mLayerComboBox, &GraphicsLayerComboBox::currentLayerChanged,
            this, &BES_DrawPlane::layerComboBoxLayerChanged);

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::CrossCursor);

    return true;
}

bool BES_DrawPlane::exit(BEE_Base* event) noexcept
{
    Q_UNUSED(event);

    // abort the currently active command
    if (mSubState != SubState::Idle) abort(true);

    // Remove actions / widgets from the "command" toolbar
    delete mLayerComboBox;          mLayerComboBox = nullptr;
    delete mLayerLabel;             mLayerLabel = nullptr;
    delete mNetSignalComboBox;      mNetSignalComboBox = nullptr;
    delete mNetSignalLabel;         mNetSignalLabel = nullptr;
    qDeleteAll(mActionSeparators);  mActionSeparators.clear();

    // change the cursor
    mEditorGraphicsView.setCursor(Qt::ArrowCursor);

    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

BES_Base::ProcRetVal BES_DrawPlane::processSubStateIdle(BEE_Base* event) noexcept
{
    switch (event->getType()) {
        case BEE_Base::GraphicsViewEvent:
            return processIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_DrawPlane::processIdleSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type()) {
        case QEvent::GraphicsSceneMousePress: {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos()).mappedToGrid(board->getGridProperties().getInterval());
            switch (sceneEvent->button()) {
                case Qt::LeftButton:
                    start(*board, pos);
                    return ForceStayInState;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    return PassToParentState;
}

BES_Base::ProcRetVal BES_DrawPlane::processSubStatePositioning(BEE_Base* event) noexcept
{
    switch (event->getType()) {
        case BEE_Base::AbortCommand:
            abort(true);
            return ForceStayInState;
        case BEE_Base::GraphicsViewEvent:
            return processPositioningSceneEvent(event);
        default:
            return PassToParentState;
    }
}

BES_Base::ProcRetVal BES_DrawPlane::processPositioningSceneEvent(BEE_Base* event) noexcept
{
    QEvent* qevent = BEE_RedirectedQEvent::getQEventFromBEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Board* board = mEditor.getActiveBoard();
    Q_ASSERT(board); if (!board) return PassToParentState;

    switch (qevent->type()) {
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress: {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos()).mappedToGrid(board->getGridProperties().getInterval());
            switch (sceneEvent->button()) {
                case Qt::LeftButton:
                    addSegment(*board, pos);
                    return ForceStayInState;
                case Qt::RightButton:
                    return ForceStayInState;
                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseMove: {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos()).mappedToGrid(board->getGridProperties().getInterval());
            updateVertexPosition(pos);
            return ForceStayInState;
        }

        default:
            break;
    }

    return PassToParentState;
}

bool BES_DrawPlane::start(Board& board, const Point& pos) noexcept
{
    try {
        // start a new undo command
        Q_ASSERT(mSubState == SubState::Idle);
        mUndoStack.beginCmdGroup(tr("Draw board plane"));
        mSubState = SubState::Positioning;

        // add plane with two vertices
        Path path({Vertex(pos), Vertex(pos)});
        mCurrentPlane = new BI_Plane(board, Uuid::createRandom(), mCurrentLayerName,
            *mCurrentNetSignal, path);
        mUndoStack.appendToCmdGroup(new CmdBoardPlaneAdd(*mCurrentPlane));

        // start undo command
        mCmdEditCurrentPlane = new CmdBoardPlaneEdit(*mCurrentPlane, false);
        mLastVertexPos = pos;
        makeSelectedLayerVisible();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        if (mSubState != SubState::Idle) abort(false);
        return false;
    }
}

bool BES_DrawPlane::addSegment(Board& board, const Point& pos) noexcept
{
    Q_UNUSED(board);
    Q_ASSERT(mSubState == SubState::Positioning);

    // abort if no segment drawn
    if (pos == mLastVertexPos) {
        abort(true);
        return false;
    }

    try {
        // if the plane has more than 2 vertices, start a new undo command
        if (mCurrentPlane->getOutline().getVertices().count() > 2) {
            mUndoStack.appendToCmdGroup(mCmdEditCurrentPlane); mCmdEditCurrentPlane = nullptr;
            mUndoStack.commitCmdGroup();
            mSubState = SubState::Idle;

            // start a new undo command
            mUndoStack.beginCmdGroup(tr("Draw board plane"));
            mSubState = SubState::Positioning;
            mCmdEditCurrentPlane = new CmdBoardPlaneEdit(*mCurrentPlane, false);
        }

        // add new vertex
        Path newPath = mCurrentPlane->getOutline();
        newPath.addVertex(pos, Angle::deg0());
        mCmdEditCurrentPlane->setOutline(newPath, true);
        mLastVertexPos = pos;
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        if (mSubState != SubState::Idle) abort(false);
        return false;
    }
}

bool BES_DrawPlane::abort(bool showErrMsgBox) noexcept
{
    try {
        delete mCmdEditCurrentPlane; mCmdEditCurrentPlane = nullptr;
        mCurrentPlane = nullptr;
        mUndoStack.abortCmdGroup(); // can throw
        mSubState = SubState::Idle;
        return true;
    } catch (const Exception& e) {
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        return false;
    }
}

void BES_DrawPlane::updateVertexPosition(const Point& cursorPos) noexcept
{
    if (mCmdEditCurrentPlane) {
        Path newPath = mCurrentPlane->getOutline();
        newPath.getVertices().last().setPos(cursorPos);
        mCmdEditCurrentPlane->setOutline(newPath, true);
    }
}

void BES_DrawPlane::layerComboBoxLayerChanged(const QString& layerName) noexcept
{
    mCurrentLayerName = layerName;
    if (mCmdEditCurrentPlane) {
        mCmdEditCurrentPlane->setLayerName(mCurrentLayerName, true);
        makeSelectedLayerVisible();
    }
}

void BES_DrawPlane::makeSelectedLayerVisible() noexcept
{
    if (mCurrentPlane) {
        Board& board = mCurrentPlane->getBoard();
        GraphicsLayer* layer = board.getLayerStack().getLayer(*mCurrentLayerName);
        if (layer && layer->isEnabled()) layer->setVisible(true);
    }
}

void BES_DrawPlane::setNetSignal(NetSignal* netsignal) noexcept
{
    try {
        if (!netsignal) throw LogicError(__FILE__, __LINE__);
        mCurrentNetSignal = netsignal;
        if (mCmdEditCurrentPlane) {
            mCmdEditCurrentPlane->setNetSignal(*mCurrentNetSignal);
        }
    } catch (const Exception& e) {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
