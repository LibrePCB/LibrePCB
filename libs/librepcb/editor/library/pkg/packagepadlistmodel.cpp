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
#include "packagepadlistmodel.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdpackagepadedit.h"

#include <librepcb/core/utils/toolbox.h>

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

PackagePadListModel::PackagePadListModel(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &PackagePadListModel::listEdited) {
}

PackagePadListModel::~PackagePadListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PackagePadListModel::setReferences(PackagePadList* list,
                                        UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (list == mList) return;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (auto obj : *mList) {
      mItems.append(createItem(obj, mItems.count()));
    }
    updateSortOrder(false);
  }

  notify_reset();
}

bool PackagePadListModel::add(QString names) noexcept {
  if (!mList) return false;

  try {
    // If no name is set we search for the next free numerical pad name.
    if (names.isEmpty()) {
      names = getNextPadNameProposal();
    }

    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Package Pad(s)")));
    foreach (const QString& nameStr, Toolbox::expandRangesInString(names)) {
      std::shared_ptr<PackagePad> obj = std::make_shared<PackagePad>(
          Uuid::createRandom(),
          validateNameOrThrow(cleanCircuitIdentifier(nameStr)));
      cmd->appendChild(new CmdPackagePadInsert(*mList, obj));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void PackagePadListModel::apply() {
  if ((!mList) || (mList->count() != mItems.count())) {
    return;
  }

  for (int i = 0; i < mList->count(); ++i) {
    auto& item = mItems[i];
    if (auto obj = mList->value(i)) {
      const QString nameStr = s2q(item.name);
      if ((nameStr != obj->getName()) && (item.name_error.empty())) {
        std::unique_ptr<CmdPackagePadEdit> cmd(new CmdPackagePadEdit(*obj));
        cmd->setName(validateNameOrThrow(cleanCircuitIdentifier(nameStr)));
        execCmd(cmd.release());
      } else {
        item.name = q2s(*obj->getName());
        item.name_error = slint::SharedString();
        notify_row_changed(i);
      }
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t PackagePadListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::PackagePadData> PackagePadListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void PackagePadListModel::set_row_data(
    std::size_t i, const ui::PackagePadData& data) noexcept {
  if ((!mList) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mList->value(i)) {
    if (data.delete_) {
      // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
      QMetaObject::invokeMethod(
          this,
          [this, i, obj]() {
            try {
              if (mList && (mList->value(i) == obj)) {
                execCmd(new CmdPackagePadRemove(*mList, obj.get()));
              }
            } catch (const Exception& e) {
              qCritical() << e.getMsg();
            }
          },
          Qt::QueuedConnection);
    } else {
      mItems[i] = data;
      const QString name = s2q(data.name);
      const bool duplicate = (name != obj->getName()) && mList->find(name);
      validateCircuitIdentifier(name, mItems[i].name_error, duplicate);
      notify_row_changed(i);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::PackagePadData PackagePadListModel::createItem(const PackagePad& obj,
                                                   int sortIndex) noexcept {
  return ui::PackagePadData{
      q2s(obj.getUuid().toStr().left(8)),  // ID
      q2s(*obj.getName()),  // Name
      slint::SharedString(),  // Name error
      false,  // Delete
      sortIndex,  // Sort index
  };
}

void PackagePadListModel::updateSortOrder(bool notify) noexcept {
  // Note: The sorting needs to be done only when the underlying list data
  // was modfied, not when the UI data is changed, since this would lead to
  // reordering while the user is typing, causing focus issues etc.

  if (!mList) return;

  auto sorted = mList->values();
  Toolbox::sortNumeric(
      sorted,
      [](const QCollator& collator, const std::shared_ptr<PackagePad>& lhs,
         const std::shared_ptr<PackagePad>& rhs) {
        return collator(*lhs->getName(), *rhs->getName());
      });
  for (int i = 0; i < mList->count(); ++i) {
    const int sortIndex = sorted.indexOf(mList->value(i));
    if (sortIndex != mItems[i].sort_index) {
      mItems[i].sort_index = sortIndex;
      if (notify) {
        notify_row_changed(i);
      }
    }
  }
}

void PackagePadListModel::listEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& item,
    PackagePadList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case PackagePadList::Event::ElementAdded:
      mItems.insert(index, createItem(*item, index));
      notify_row_added(index, 1);
      updateSortOrder(true);
      break;
    case PackagePadList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case PackagePadList::Event::ElementEdited:
      mItems[index] = createItem(*item, index);
      notify_row_changed(index);
      updateSortOrder(true);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PackagePadListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PackagePadListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

CircuitIdentifier PackagePadListModel::validateNameOrThrow(
    const QString& name) const {
  if (mList && mList->contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already a pad with the name \"%1\".").arg(name));
  }
  return CircuitIdentifier(name);  // can throw
}

QString PackagePadListModel::getNextPadNameProposal() const noexcept {
  int i = 1;
  while (mList && mList->contains(QString::number(i))) {
    ++i;
  }
  return QString::number(i);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
