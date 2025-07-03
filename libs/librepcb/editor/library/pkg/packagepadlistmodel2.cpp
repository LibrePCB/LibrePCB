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
#include "packagepadlistmodel2.h"

#include "../../undocommand.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdpackagepadedit.h"

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

PackagePadListModel2::PackagePadListModel2(QObject* parent) noexcept
  : QObject(parent),
    mList(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &PackagePadListModel2::listEdited) {
}

PackagePadListModel2::~PackagePadListModel2() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PackagePadListModel2::setList(PackagePadList* list) noexcept {
  if (list == mList) return;

  if (mList) {
    mList->onEdited.detach(mOnEditedSlot);
  }

  mList = list;
  mItems.clear();

  if (mList) {
    mList->onEdited.attach(mOnEditedSlot);

    for (auto sig : *mList) {
      mItems.append(createItem(sig));
    }
  }

  notify_reset();
}

void PackagePadListModel2::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

bool PackagePadListModel2::add(const QStringList& names) noexcept {
  if (!mList) return false;

  try {
    std::unique_ptr<UndoCommandGroup> cmd(
        new UndoCommandGroup(tr("Add Package Pad(s)")));
    foreach (const QString& nameStr, names) {
      const CircuitIdentifier name(
          cleanCircuitIdentifier(nameStr));  // can throw
      if (mList->contains(*name)) {
        throwDuplicateNameError(*name);
      }
      std::shared_ptr<PackagePad> sig =
          std::make_shared<PackagePad>(Uuid::createRandom(), name);
      cmd->appendChild(new CmdPackagePadInsert(*mList, sig));
    }
    execCmd(cmd.release());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return false;
  }
}

void PackagePadListModel2::apply() {
  if (!mList) return;

  /*for (auto i = 0; i < mItems.count(); ++i) {
    auto& item = mItems[i];
    if (auto sig = mList->value(i)) {
      std::unique_ptr<CmdPackagePadEdit> cmd(
          new CmdPackagePadEdit(*sig));
      const auto name =
          parseCircuitIdentifier(cleanCircuitIdentifier(s2q(item.name)));
      if (name && ((*name) != sig->getName())) {
        if (mList->contains(**name)) {
          item.name = q2s(*sig->getName());
          item.name_error = slint::SharedString();
          notify_row_changed(i);
          throwDuplicateNameError(**name);
        }
        cmd->setName(*name);
      } else {
        item.name = q2s(*sig->getName());
        item.name_error = slint::SharedString();
        notify_row_changed(i);
      }
      cmd->setIsRequired(item.required);
      cmd->setForcedNetName(cleanForcedNetName(s2q(item.forced_net_name)));
      execCmd(cmd.release());
    }
  }*/
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t PackagePadListModel2::row_count() const {
  return mList ? mList->count() : 0;
}

std::optional<ui::PackagePadData> PackagePadListModel2::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void PackagePadListModel2::set_row_data(
    std::size_t i, const ui::PackagePadData& data) noexcept {
  if (mList && (i < static_cast<std::size_t>(mItems.count()))) {
    if (auto sig = mList->value(i)) {
      if (data.delete_) {
        // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
        QMetaObject::invokeMethod(
            this,
            [this, sig]() {
              try {
                execCmd(new CmdPackagePadRemove(*mList, sig.get()));
              } catch (const Exception& e) {
                qCritical() << e.getMsg();
              }
            },
            Qt::QueuedConnection);
      } else {
        mItems[i] = data;
        validateCircuitIdentifier(s2q(data.name), mItems[i].name_error);
        notify_row_changed(i);
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::PackagePadData PackagePadListModel2::createItem(
    const PackagePad& sig) noexcept {
  return ui::PackagePadData{
      q2s(sig.getUuid().toStr().left(8)),  // ID
      q2s(*sig.getName()),  // Name
      slint::SharedString(),  // Name error
      false,  // Delete
  };
}

void PackagePadListModel2::listEdited(
    const PackagePadList& list, int index,
    const std::shared_ptr<const PackagePad>& item,
    PackagePadList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(item);
  switch (event) {
    case PackagePadList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case PackagePadList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case PackagePadList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PackagePadListModel2::signalListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PackagePadListModel2::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

void PackagePadListModel2::throwDuplicateNameError(const QString& name) {
  throw RuntimeError(
      __FILE__, __LINE__,
      tr("There is already a signal with the name \"%1\".").arg(name));
}

QString PackagePadListModel2::cleanForcedNetName(const QString& name) noexcept {
  // Same as cleanCircuitIdentifier(), but allowing '{' and '}' because it's
  // allowed to have attribute placeholders in a forced net name. Also remove
  // spaces because they must not be replaced by underscores inside {{ and }}.
  return Toolbox::cleanUserInputString(
      name, QRegularExpression("[^-a-zA-Z0-9_+/!?@#$\\{\\}]"), true, false,
      false, "");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
