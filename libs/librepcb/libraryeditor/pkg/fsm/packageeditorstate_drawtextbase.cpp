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
#include "packageeditorstate_drawtextbase.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include "../packageeditorwidget.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageEditorState_DrawTextBase::PackageEditorState_DrawTextBase(Context& context, Mode mode) noexcept :
    PackageEditorState(context), mMode(mode), mCurrentText(nullptr),
    mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sTopNames), mLastHeight(1)
{
    resetToDefaultParameters();
}

PackageEditorState_DrawTextBase::~PackageEditorState_DrawTextBase() noexcept
{
    Q_ASSERT(mEditCmd.isNull());
    Q_ASSERT(mCurrentText == nullptr);
    Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool PackageEditorState_DrawTextBase::entry() noexcept
{
    mContext.graphicsScene.setSelectionArea(QPainterPath()); // clear selection
    mContext.graphicsView.setCursor(Qt::CrossCursor);

    // populate command toolbar
    if (mMode == Mode::TEXT) {
        mContext.commandToolBar.addLabel(tr("Layer:"));
        std::unique_ptr<GraphicsLayerComboBox> layerComboBox(new GraphicsLayerComboBox());
        layerComboBox->setLayers(mContext.layerProvider.getBoardGeometryElementLayers());
        layerComboBox->setCurrentLayer(*mLastLayerName);
        connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
                this, &PackageEditorState_DrawTextBase::layerComboBoxValueChanged);
        mContext.commandToolBar.addWidget(std::move(layerComboBox));

        mContext.commandToolBar.addLabel(tr("Text:"), 10);
        std::unique_ptr<QComboBox> textComboBox(new QComboBox());
        textComboBox->setEditable(true);
        textComboBox->addItem("{{NAME}}");
        textComboBox->addItem("{{VALUE}}");
        textComboBox->addItem("{{_BOARD}}");
        textComboBox->addItem("{{_PROJECT}}");
        textComboBox->addItem("{{_AUTHOR}}");
        textComboBox->addItem("{{_VERSION}}");
        textComboBox->addItem("{{_MODIFIED_DATE}}");
        textComboBox->setCurrentIndex(textComboBox->findText(mLastText));
        connect(textComboBox.get(), &QComboBox::currentTextChanged,
                this, &PackageEditorState_DrawTextBase::textComboBoxValueChanged);
        mContext.commandToolBar.addWidget(std::move(textComboBox));
    } else {
        resetToDefaultParameters();
    }

    mContext.commandToolBar.addLabel(tr("Height:"), 10);
    std::unique_ptr<QDoubleSpinBox> lineWidthSpinBox(new QDoubleSpinBox());
    lineWidthSpinBox->setMinimum(0.1);
    lineWidthSpinBox->setMaximum(100);
    lineWidthSpinBox->setSingleStep(0.1);
    lineWidthSpinBox->setDecimals(6);
    lineWidthSpinBox->setValue(mLastHeight->toMm());
    connect(lineWidthSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &PackageEditorState_DrawTextBase::heightSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(lineWidthSpinBox));

    Point pos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
    return startAddText(pos);
}

bool PackageEditorState_DrawTextBase::exit() noexcept
{
    if (mCurrentText && !abortAddText()) {
        return false;
    }

    // cleanup command toolbar
    mContext.commandToolBar.clear();

    mContext.graphicsView.setCursor(Qt::ArrowCursor);
    return true;
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool PackageEditorState_DrawTextBase::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (mCurrentText) {
        Point currentPos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
        mEditCmd->setPosition(currentPos, true);
        return true;
    } else {
        return false;
    }
}

bool PackageEditorState_DrawTextBase::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    Point currentPos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    if (mCurrentText) {
        finishAddText(currentPos);
    }
    return startAddText(currentPos);
}

bool PackageEditorState_DrawTextBase::processGraphicsSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent &e)  noexcept
{
    Q_UNUSED(e);
    return processRotateCcw();
}
bool PackageEditorState_DrawTextBase::processRotateCw() noexcept
{
    if (mCurrentText) {
        mEditCmd->rotate(-Angle::deg90(), mCurrentText->getPosition(), true);
        mLastRotation = mCurrentText->getRotation();
        return true;
    } else {
        return false;
    }
}

bool PackageEditorState_DrawTextBase::processRotateCcw() noexcept
{
    if (mCurrentText) {
        mEditCmd->rotate(Angle::deg90(), mCurrentText->getPosition(), true);
        mLastRotation = mCurrentText->getRotation();
        return true;
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool PackageEditorState_DrawTextBase::startAddText(const Point& pos) noexcept
{
    try {
        mStartPos = pos;
        mContext.undoStack.beginCmdGroup(tr("Add footprint text"));
        mCurrentText = new StrokeText(Uuid::createRandom(), mLastLayerName, mLastText, pos,
            mLastRotation, mLastHeight, UnsignedLength(200000), StrokeTextSpacing(),
            StrokeTextSpacing(), Alignment(HAlign::left(), VAlign::bottom()), false, true);
        mContext.undoStack.appendToCmdGroup(
            new CmdStrokeTextInsert(mContext.currentFootprint->getStrokeTexts(),
                                    std::shared_ptr<StrokeText>(mCurrentText)));
        mEditCmd.reset(new CmdStrokeTextEdit(*mCurrentText));
        mCurrentGraphicsItem = mContext.currentGraphicsItem->getTextGraphicsItem(*mCurrentText);
        Q_ASSERT(mCurrentGraphicsItem);
        mCurrentGraphicsItem->setSelected(true);
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mCurrentGraphicsItem = nullptr;
        mCurrentText = nullptr;
        mEditCmd.reset();
        return false;
    }
}

bool PackageEditorState_DrawTextBase::finishAddText(const Point& pos) noexcept
{
    if (pos == mStartPos) {
        return abortAddText();
    }

    try {
        mEditCmd->setPosition(pos, true);
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentText = nullptr;
        mContext.undoStack.appendToCmdGroup(mEditCmd.take());
        mContext.undoStack.commitCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

bool PackageEditorState_DrawTextBase::abortAddText() noexcept
{
    try {
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentText = nullptr;
        mEditCmd.reset();
        mContext.undoStack.abortCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

void PackageEditorState_DrawTextBase::resetToDefaultParameters() noexcept
{
    switch (mMode) {
        case Mode::NAME:
            mLastLayerName = GraphicsLayer::sTopNames;
            mLastHeight = Length(1000000);
            mLastText = "{{NAME}}";
            break;
        case Mode::VALUE:
            mLastLayerName = GraphicsLayer::sTopValues;
            mLastHeight = Length(1000000);
            mLastText = "{{VALUE}}";
            break;
        default:
            mLastLayerName = GraphicsLayer::sTopPlacement;
            mLastHeight = Length(2000000);
            mLastText = "";
            break;
    }
}

void PackageEditorState_DrawTextBase::layerComboBoxValueChanged(const QString& layerName) noexcept
{
    if (layerName.isEmpty()) {
        return;
    }
    mLastLayerName = layerName;
    if (mEditCmd) {
        mEditCmd->setLayerName(mLastLayerName, true);
    }
}

void PackageEditorState_DrawTextBase::heightSpinBoxValueChanged(double value) noexcept
{
    mLastHeight = Length::fromMm(value);
    if (mEditCmd) {
        mEditCmd->setHeight(mLastHeight, true);
    }
}

void PackageEditorState_DrawTextBase::textComboBoxValueChanged(const QString& value) noexcept
{
    mLastText = value.trimmed();
    if (mEditCmd) {
        mEditCmd->setText(mLastText, true);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
