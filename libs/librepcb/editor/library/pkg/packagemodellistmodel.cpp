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
#include "packagemodellistmodel.h"

#include "../../dialogs/filedialog.h"
#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdpackagemodeladd.h"
#include "../cmd/cmdpackagemodeledit.h"
#include "../cmd/cmdpackagemodelremove.h"

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/utils/scopeguard.h>

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

PackageModelListModel::PackageModelListModel(QObject* parent) noexcept
  : QObject(parent),
    mPackage(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &PackageModelListModel::listEdited) {
}

PackageModelListModel::~PackageModelListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PackageModelListModel::setReferences(Package* pkg,
                                          UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (pkg == mPackage) return;

  if (mPackage) {
    mPackage->getModels().onEdited.detach(mOnEditedSlot);
  }

  mPackage = pkg;
  mItems.clear();

  if (mPackage) {
    mPackage->getModels().onEdited.attach(mOnEditedSlot);

    for (auto sig : mPackage->getModels()) {
      mItems.append(createItem(sig));
    }
  }

  notify_reset();
}

std::optional<int> PackageModelListModel::add() noexcept {
  if (!mPackage) return std::nullopt;

  try {
    FilePath fp;
    QByteArray content;
    if (!chooseStepFile(content, &fp)) {  // can throw
      return std::nullopt;
    }

    // Generate a unique name from the file name.
    QString name;
    int number = 1;
    do {
      name = cleanElementName(fp.getCompleteBasename());
      if (name.isEmpty()) {
        name = "Model";
      }
      if (number > 1) {
        name += QString(" (%1)").arg(number);
      }
      ++number;
    } while (mPackage->getModels().contains(name));

    // Add new package model with the loaded STEP file.
    std::shared_ptr<PackageModel> obj = std::make_shared<PackageModel>(
        Uuid::createRandom(), validateNameOrThrow(name));
    execCmd(new CmdPackageModelAdd(*mPackage, obj, content, true));
    return mPackage->getModels().count() - 1;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    return std::nullopt;
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t PackageModelListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::PackageModelData> PackageModelListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void PackageModelListModel::set_row_data(
    std::size_t i, const ui::PackageModelData& data) noexcept {
  if ((!mPackage) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mPackage->getModels().value(i)) {
    const QString nameStr = s2q(data.name);

    if (data.action != ui::PackageModelAction::None) {
      // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
      QMetaObject::invokeMethod(
          this, [this, i, obj, a = data.action]() { trigger(i, obj, a); },
          Qt::QueuedConnection);
    } else if (obj->getName() != nameStr) {
      try {
        std::unique_ptr<CmdPackageModelEdit> cmd(
            new CmdPackageModelEdit(*mPackage, *obj));
        cmd->setName(validateNameOrThrow(cleanElementName(nameStr)));
        execCmd(cmd.release());
      } catch (const Exception& e) {
        qCritical() << e.getMsg();
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

ui::PackageModelData PackageModelListModel::createItem(
    const PackageModel& obj) noexcept {
  return ui::PackageModelData{
      q2s(obj.getUuid().toStr().left(8)),  // ID
      q2s(*obj.getName()),  // Name
      ui::PackageModelAction::None,  // Action
  };
}

void PackageModelListModel::trigger(int index,
                                    std::shared_ptr<PackageModel> obj,
                                    ui::PackageModelAction a) noexcept {
  if ((!mPackage) || (!obj) || (mPackage->getModels().value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::PackageModelAction::MoveUp) {
      const int index = mPackage->getModels().indexOf(obj.get());
      execCmd(
          new CmdPackageModelsSwap(mPackage->getModels(), index, index - 1));
    } else if (a == ui::PackageModelAction::Browse) {
      QByteArray content;
      if (!chooseStepFile(content)) {  // can throw
        return;
      }
      std::unique_ptr<CmdPackageModelEdit> cmd(
          new CmdPackageModelEdit(*mPackage, *obj));
      cmd->setStepContent(content);
      execCmd(cmd.release());
    } else if (a == ui::PackageModelAction::Delete) {
      execCmd(new CmdPackageModelRemove(*mPackage, obj));
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void PackageModelListModel::listEdited(
    const PackageModelList& list, int index,
    const std::shared_ptr<const PackageModel>& item,
    PackageModelList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case PackageModelList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case PackageModelList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case PackageModelList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PackageModelListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PackageModelListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

ElementName PackageModelListModel::validateNameOrThrow(
    const QString& name) const {
  if (mPackage && mPackage->getModels().contains(name)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("There is already a 3D model with the name \"%1\".").arg(name));
  }
  return ElementName(name);  // can throw
}

bool PackageModelListModel::chooseStepFile(QByteArray& content,
                                           FilePath* selectedFile) {
  QSettings cs;
  QString key = "library_editor/package_editor/step_file";
  QString initialFp = cs.value(key, QDir::homePath()).toString();
  FilePath fp(FileDialog::getOpenFileName(
      qobject_cast<QWidget*>(parent()), tr("Choose STEP Model"), initialFp,
      "STEP Models (*.step *.stp *.STEP *.STP *.Step *.Stp)"));
  if (selectedFile) {
    *selectedFile = fp;
  }
  if (!fp.isValid()) {
    return false;
  }
  cs.setValue(key, fp.toStr());

  std::optional<QString> minifyError;
  {
    // Loading and minifying the STEP file can block the UI some time, so let's
    // indicate the ongoing operation with a wait cursor.
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    auto csg = scopeGuard([]() { QGuiApplication::restoreOverrideCursor(); });

    // Load and try to minify the provided STEP file.
    content = FileUtils::readFile(fp);
    try {
      const QByteArray minified = OccModel::minifyStep(content);
      OccModel::loadStep(minified);  // throws if invalid
      content = minified;
    } catch (const Exception& e) {
      // Maybe the original STEP file is already broken, let's validate it now.
      OccModel::loadStep(content);  // throws if invalid
      minifyError = e.getMsg();
    }
  }

  if (minifyError) {
    // Original looks good, just show a warning that the minification failed.
    qCritical() << "Failed to minify STEP file:" << *minifyError;
    QString msg = "<p>" %
        tr("Failed to minify the provided STEP file, will keep the original "
           "as-is.") %
        "</p>";
    msg += "<p>" % tr("Reason:") % " " % (*minifyError) % "</p>";
    msg += "<p>" %
        tr("Please <a href='%1'>report this issue</a> to the LibrePCB "
           "developers with the STEP file attached.")
            .arg("https://github.com/LibrePCB/LibrePCB/issues/new/choose") %
        "</p>";
    QMessageBox::warning(nullptr, tr("Warning"), msg, QMessageBox::Ok);
  }

  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
