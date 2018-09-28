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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorwidgetbase.h"

#include <librepcb/common/undostack.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/common/utils/toolbarproxy.h>
#include <librepcb/common/utils/undostackactiongroup.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditorWidgetBase::EditorWidgetBase(const Context& context, const FilePath& fp,
                                   QWidget* parent)
  : QWidget(parent),
    mContext(context),
    mFilePath(fp),
    mUndoStackActionGroup(nullptr),
    mToolsActionGroup(nullptr),
    mIsDirty(false),
    mIsInterfaceBroken(false) {
  mUndoStack.reset(new UndoStack());
  connect(mUndoStack.data(), &UndoStack::cleanChanged, this,
          &EditorWidgetBase::undoStackCleanChanged);
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &EditorWidgetBase::undoStackStateModified);

  mCommandToolBarProxy.reset(new ToolBarProxy());
}

EditorWidgetBase::~EditorWidgetBase() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool EditorWidgetBase::isDirty() const noexcept {
  return (!mUndoStack->isClean() || mIsDirty);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EditorWidgetBase::setUndoStackActionGroup(
    UndoStackActionGroup* group) noexcept {
  if (group == mUndoStackActionGroup) return;
  if (mUndoStackActionGroup) mUndoStackActionGroup->setUndoStack(nullptr);
  mUndoStackActionGroup = group;
  if (mUndoStackActionGroup)
    mUndoStackActionGroup->setUndoStack(mUndoStack.data());
}

void EditorWidgetBase::setToolsActionGroup(
    ExclusiveActionGroup* group) noexcept {
  if (group == mToolsActionGroup) {
    return;
  }

  if (mToolsActionGroup) {
    disconnect(mToolsActionGroup, &ExclusiveActionGroup::changeRequestTriggered,
               this, &EditorWidgetBase::toolActionGroupChangeTriggered);
    mToolsActionGroup->reset();
  }

  mToolsActionGroup = group;

  if (mToolsActionGroup) {
    mToolsActionGroup->reset();
    connect(mToolsActionGroup, &ExclusiveActionGroup::changeRequestTriggered,
            this, &EditorWidgetBase::toolActionGroupChangeTriggered);
  }
}

void EditorWidgetBase::setCommandToolBar(QToolBar* toolbar) noexcept {
  mCommandToolBarProxy->setToolBar(toolbar);
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool EditorWidgetBase::save() noexcept {
  mIsDirty           = false;
  mIsInterfaceBroken = false;
  mUndoStack->setClean();
  emit dirtyChanged(false);
  emit interfaceBrokenChanged(false);
  emit elementEdited(mFilePath);
  return true;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void EditorWidgetBase::setupInterfaceBrokenWarningWidget(
    QWidget& widget) noexcept {
  widget.setVisible(false);
  widget.setStyleSheet(
      "background-color: rgb(255, 255, 127); "
      "color: rgb(170, 0, 0);");
  QLabel* label = new QLabel(&widget);
  QFont   font  = label->font();
  font.setBold(true);
  label->setFont(font);
  label->setWordWrap(true);
  label->setText(
      tr("WARNING: You have changed some important properties of this "
         "library element. This breaks all other elements which depend on "
         "this one! Maybe you want to create a new library element instead "
         "of modifying this one?"));
  QHBoxLayout* layout = new QHBoxLayout(&widget);
  layout->addWidget(label);
  connect(this, &EditorWidgetBase::interfaceBrokenChanged, &widget,
          &QWidget::setVisible);
}

void EditorWidgetBase::setDirty() noexcept {
  if (!mIsDirty) {
    mIsDirty = true;
    emit dirtyChanged(true);
  }
}

void EditorWidgetBase::undoStackStateModified() noexcept {
  if (!mContext.elementIsNewlyCreated) {
    bool broken = isInterfaceBroken();
    if (broken != mIsInterfaceBroken) {
      mIsInterfaceBroken = broken;
      emit interfaceBrokenChanged(mIsInterfaceBroken);
    }
  }
}

const QStringList& EditorWidgetBase::getLibLocaleOrder() const noexcept {
  return mContext.workspace.getSettings().getLibLocaleOrder().getLocaleOrder();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EditorWidgetBase::toolActionGroupChangeTriggered(
    const QVariant& newTool) noexcept {
  toolChangeRequested(static_cast<Tool>(newTool.toInt()));
}

void EditorWidgetBase::undoStackCleanChanged(bool clean) noexcept {
  Q_UNUSED(clean);
  emit dirtyChanged(isDirty());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
