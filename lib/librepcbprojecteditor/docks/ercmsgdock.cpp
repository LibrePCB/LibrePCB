/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <QtWidgets>
#include "ercmsgdock.h"
#include "ui_ercmsgdock.h"
#include <librepcbproject/project.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/erc/ercmsg.h>
#include <librepcbproject/erc/ercmsglist.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ErcMsgDock::ErcMsgDock(Project& project) :
    QDockWidget(0), mProject(project), mErcMsgList(project.getErcMsgList()),
    mUi(new Ui::ErcMsgDock)
{
    mUi->setupUi(this);

    // add top-level items
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::CircuitError),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::CircuitWarning),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::SchematicError),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::SchematicWarning),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::BoardError),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::BoardWarning),
                          new QTreeWidgetItem(mUi->treeWidget));
    mTopLevelItems.insert(static_cast<int>(ErcMsg::ErcMsgType_t::_Count),
                          new QTreeWidgetItem(mUi->treeWidget));

    // check if there is a top level item for each existing ERC message type + one for ignored items
    Q_ASSERT(mTopLevelItems.count() == static_cast<int>(ErcMsg::ErcMsgType_t::_Count) + 1);

    // set icons
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitError)]->
            setIcon(0, QIcon(":/img/status/dialog-error.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitWarning)]->
            setIcon(0, QIcon(":/img/status/dialog-warning.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicError)]->
            setIcon(0, QIcon(":/img/status/dialog-error.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicWarning)]->
            setIcon(0, QIcon(":/img/status/dialog-warning.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardError)]->
            setIcon(0, QIcon(":/img/status/dialog-error.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardWarning)]->
            setIcon(0, QIcon(":/img/status/dialog-warning.png"));
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::_Count)]->
            setIcon(0, QIcon(":/img/actions/apply.png"));

    // expand top-level items
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitError)]->setExpanded(true);
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitWarning)]->setExpanded(true);
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicError)]->setExpanded(true);
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicWarning)]->setExpanded(true);
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardError)]->setExpanded(true);
    mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardWarning)]->setExpanded(true);

    // add all already existing ERC messages
    foreach (ErcMsg* ercMsg, mProject.getErcMsgList().getItems())
    {
        QTreeWidgetItem* parent;
        if (!ercMsg->isIgnored())
            parent = mTopLevelItems.value(static_cast<int>(ercMsg->getMsgType()), 0);
        else
            parent = mTopLevelItems.value(static_cast<int>(ErcMsg::ErcMsgType_t::_Count), 0);
        Q_ASSERT(parent); if (!parent) continue;
        QTreeWidgetItem* child = new QTreeWidgetItem(parent, QStringList(ercMsg->getMsg()));
        child->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(ercMsg))); // ugly...
        child->setToolTip(0, ercMsg->getMsg());
        mErcMsgItems.insert(ercMsg, child);
    }

    // connect to ErcMsgList signals
    connect(&mProject.getErcMsgList(), &ErcMsgList::ercMsgAdded,    this, &ErcMsgDock::ercMsgAdded);
    connect(&mProject.getErcMsgList(), &ErcMsgList::ercMsgRemoved,  this, &ErcMsgDock::ercMsgRemoved);
    connect(&mProject.getErcMsgList(), &ErcMsgList::ercMsgChanged,  this, &ErcMsgDock::ercMsgChanged);

    updateTopLevelItemTexts();
}

ErcMsgDock::~ErcMsgDock()
{
    delete mUi;         mUi = 0;
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void ErcMsgDock::ercMsgAdded(ErcMsg* ercMsg) noexcept
{
    QTreeWidgetItem* parent;
    Q_ASSERT(ercMsg);
    Q_ASSERT(!mErcMsgItems.contains(ercMsg));
    if (!ercMsg->isIgnored())
        parent = mTopLevelItems.value(static_cast<int>(ercMsg->getMsgType()), 0);
    else
        parent = mTopLevelItems.value(static_cast<int>(ErcMsg::ErcMsgType_t::_Count), 0);
    Q_ASSERT(parent); if (!parent) return;
    QTreeWidgetItem* child = new QTreeWidgetItem(parent, QStringList(ercMsg->getMsg()));
    child->setToolTip(0, ercMsg->getMsg());
    parent->sortChildren(0, Qt::AscendingOrder);
    mErcMsgItems.insert(ercMsg, child);
    updateTopLevelItemTexts();
}

void ErcMsgDock::ercMsgRemoved(ErcMsg* ercMsg) noexcept
{
    Q_ASSERT(ercMsg);
    Q_ASSERT(mErcMsgItems.contains(ercMsg));
    delete mErcMsgItems.take(ercMsg);
    updateTopLevelItemTexts();
}

void ErcMsgDock::ercMsgChanged(ErcMsg* ercMsg) noexcept
{
    ercMsgRemoved(ercMsg);
    ercMsgAdded(ercMsg);
}

/*****************************************************************************************
 *  GUI Actions
 ****************************************************************************************/

void ErcMsgDock::on_treeWidget_itemSelectionChanged()
{
    bool allDisplayed = true;
    bool allIgnored = true;

    foreach (QTreeWidgetItem* item, mUi->treeWidget->selectedItems())
    {
        ErcMsg* ercMsg = mErcMsgItems.key(item, nullptr);
        if (!ercMsg)
        {
            allDisplayed = false;
            allIgnored = false;
            break;
        }
        if (ercMsg->isIgnored())
            allDisplayed = false;
        else
            allIgnored = false;
    }

    mUi->btnIgnore->setEnabled(allDisplayed != allIgnored);
    mUi->btnIgnore->setChecked(allIgnored);
}

void ErcMsgDock::on_btnIgnore_clicked(bool checked)
{
    foreach (QTreeWidgetItem* item, mUi->treeWidget->selectedItems())
    {
        ErcMsg* ercMsg = mErcMsgItems.key(item, nullptr);
        if (!ercMsg) continue;
        ercMsg->setIgnored(checked);
        // TODO: set "project modified" flag
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ErcMsgDock::updateTopLevelItemTexts() noexcept
{
    uint countOfNonIgnoredErcMessages = 0;
    QTreeWidgetItem* item;
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitError)];
    item->setText(0, QString(tr("Circuit Errors (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::CircuitWarning)];
    item->setText(0, QString(tr("Circuit Warnings (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicError)];
    item->setText(0, QString(tr("Schematic Errors (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::SchematicWarning)];
    item->setText(0, QString(tr("Schematic Warnings (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardError)];
    item->setText(0, QString(tr("Board Errors (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::BoardWarning)];
    item->setText(0, QString(tr("Board Warnings (%1)")).arg(item->childCount()));
    countOfNonIgnoredErcMessages += item->childCount();
    item = mTopLevelItems[static_cast<int>(ErcMsg::ErcMsgType_t::_Count)];
    item->setText(0, QString(tr("Ignored (%1)")).arg(item->childCount()));

    setWindowTitle(QString(tr("ERC Messages (%1)")).arg(countOfNonIgnoredErcMessages));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
