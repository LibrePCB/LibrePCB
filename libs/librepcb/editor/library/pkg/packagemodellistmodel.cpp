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
#include "../../undostack.h"
#include "../cmd/cmdfootprintedit.h"
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
  : QAbstractTableModel(parent),
    mPackage(nullptr),
    mFootprint(),
    mUndoStack(nullptr),
    mNewEnabled(true),
    mNewName(),
    mOnEditedSlot(*this, &PackageModelListModel::modelListEdited),
    mOnFootprintEditedSlot(*this, &PackageModelListModel::footprintEdited) {
}

PackageModelListModel::~PackageModelListModel() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackageModelListModel::setPackage(Package* package) noexcept {
  emit beginResetModel();

  if (mPackage) {
    mPackage->getModels().onEdited.detach(mOnEditedSlot);
  }

  mPackage = package;

  if (mPackage) {
    mPackage->getModels().onEdited.attach(mOnEditedSlot);
  }

  emit endResetModel();
}

void PackageModelListModel::setFootprint(
    std::shared_ptr<Footprint> footprint) noexcept {
  if (footprint != mFootprint) {
    if (mFootprint) {
      mFootprint->onEdited.detach(mOnFootprintEditedSlot);
    }
    mFootprint = footprint;
    if (mFootprint) {
      mFootprint->onEdited.attach(mOnFootprintEditedSlot);
    }
    dataChanged(index(0, COLUMN_ENABLED),
                index(rowCount() - 1, COLUMN_ENABLED));
  }
}

void PackageModelListModel::setUndoStack(UndoStack* stack) noexcept {
  mUndoStack = stack;
}

/*******************************************************************************
 *  Slots
 ******************************************************************************/

void PackageModelListModel::add(
    const QPersistentModelIndex& itemIndex) noexcept {
  Q_UNUSED(itemIndex);
  if (!mPackage) {
    return;
  }

  try {
    FilePath fp;
    QByteArray content;
    if (!chooseStepFile(content, &fp)) {  // can throw
      return;
    }

    if (mNewName.isEmpty()) {
      mNewName = cleanElementName(fp.getCompleteBasename());
    }

    // Add new package model with the loaded STEP file.
    std::shared_ptr<PackageModel> obj = std::make_shared<PackageModel>(
        Uuid::createRandom(), validateNameOrThrow(mNewName));
    execCmd(new CmdPackageModelAdd(*mPackage, obj, content, mNewEnabled));
    mNewName = QString();

    // Make sure the view selects the new model as the user expects that.
    emit newModelAdded(mPackage->getModels().count() - 1);
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Error"), e.getMsg());
  }
}

void PackageModelListModel::remove(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    std::shared_ptr<PackageModel> obj = mPackage->getModels().get(uuid);
    execCmd(new CmdPackageModelRemove(*mPackage, obj));
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Error"), e.getMsg());
  }
}

void PackageModelListModel::edit(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    std::shared_ptr<PackageModel> obj = mPackage->getModels().get(uuid);

    QByteArray content;
    if (!chooseStepFile(content)) {
      return;
    }

    std::unique_ptr<CmdPackageModelEdit> cmd(
        new CmdPackageModelEdit(*mPackage, *obj));
    cmd->setStepContent(content);
    execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Error"), e.getMsg());
  }
}

void PackageModelListModel::moveUp(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    const Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    const int index = mPackage->getModels().indexOf(uuid);
    if ((index >= 1) && (index < mPackage->getModels().count())) {
      execCmd(
          new CmdPackageModelsSwap(mPackage->getModels(), index, index - 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

void PackageModelListModel::moveDown(
    const QPersistentModelIndex& itemIndex) noexcept {
  if (!mPackage) {
    return;
  }

  try {
    const Uuid uuid = Uuid::fromString(itemIndex.data(Qt::EditRole).toString());
    const int index = mPackage->getModels().indexOf(uuid);
    if ((index >= 0) && (index < mPackage->getModels().count() - 1)) {
      execCmd(
          new CmdPackageModelsSwap(mPackage->getModels(), index, index + 1));
    }
  } catch (const Exception& e) {
    QMessageBox::critical(0, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Inherited from QAbstractItemModel
 ******************************************************************************/

int PackageModelListModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid() && mPackage) {
    return mPackage->getModels().count() + 1;
  }
  return 0;
}

int PackageModelListModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return _COLUMN_COUNT;
  }
  return 0;
}

QVariant PackageModelListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || !mPackage) {
    return QVariant();
  }

  std::shared_ptr<PackageModel> item = mPackage->getModels().value(index.row());
  switch (index.column()) {
    case COLUMN_ENABLED: {
      switch (role) {
        case Qt::CheckStateRole: {
          const bool enabled = item
              ? (mFootprint &&
                 mFootprint->getModels().contains(item->getUuid()))
              : mNewEnabled;
          return enabled ? Qt::Checked : Qt::Unchecked;
        }
        case Qt::ToolTipRole:
          return tr("Enable/disable this model for the selected footprint.");
        default:
          return QVariant();
      }
    }
    case COLUMN_NAME: {
      QString name = item ? *item->getName() : mNewName;
      bool showHint = (!item) && mNewName.isEmpty();
      QString hint = tr("3D model name");
      switch (role) {
        case Qt::DisplayRole:
          return showHint ? hint : name;
        case Qt::ToolTipRole:
          return showHint ? hint : QVariant();
        case Qt::EditRole:
          return name;
        case Qt::ForegroundRole:
          if (showHint) {
            QColor color = qApp->palette().text().color();
            color.setAlpha(128);
            return QBrush(color);
          } else {
            return QVariant();
          }
        default:
          return QVariant();
      }
    }
    case COLUMN_ACTIONS: {
      switch (role) {
        case Qt::EditRole:
          return item ? item->getUuid().toStr() : QVariant();
        default:
          return QVariant();
      }
    }
    default:
      return QVariant();
  }

  return QVariant();
}

QVariant PackageModelListModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case COLUMN_ENABLED:
          return "☑";
        case COLUMN_NAME:
          return tr("3D Models");
        default:
          return QVariant();
      }
    } else if ((role == Qt::TextAlignmentRole) && (section == COLUMN_NAME)) {
      return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::FontRole) {
      QFont f = QAbstractItemModel::headerData(section, orientation, role)
                    .value<QFont>();
      f.setBold(section == COLUMN_NAME);
      return f;
    }
  } else if (orientation == Qt::Vertical) {
    if (mPackage && (role == Qt::DisplayRole)) {
      std::shared_ptr<PackageModel> item = mPackage->getModels().value(section);
      return item ? QString::number(section + 1) : tr("New:");
    } else if (mPackage && (role == Qt::ToolTipRole)) {
      std::shared_ptr<PackageModel> item = mPackage->getModels().value(section);
      return item ? item->getUuid().toStr() : tr("Add a new 3D model");
    } else if (role == Qt::TextAlignmentRole) {
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

Qt::ItemFlags PackageModelListModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid()) {
    if (index.column() == COLUMN_ENABLED) {
      f |= Qt::ItemIsUserCheckable;
    }
    if (index.column() != COLUMN_ACTIONS) {
      f |= Qt::ItemIsEditable;
    }
  }
  return f;
}

bool PackageModelListModel::setData(const QModelIndex& index,
                                    const QVariant& value, int role) {
  if (!mPackage) {
    return false;
  }

  try {
    std::shared_ptr<PackageModel> item =
        mPackage->getModels().value(index.row());
    std::unique_ptr<CmdPackageModelEdit> cmd;
    if (item) {
      cmd.reset(new CmdPackageModelEdit(*mPackage, *item));
    }
    std::unique_ptr<CmdFootprintEdit> cmdFpt;
    if (mFootprint) {
      cmdFpt.reset(new CmdFootprintEdit(*mFootprint));
    }
    if ((index.column() == COLUMN_NAME) && role == Qt::EditRole) {
      QString name = value.toString().trimmed();
      QString cleanedName = cleanElementName(name);
      if (cmd) {
        if (cleanedName != item->getName()) {
          cmd->setName(validateNameOrThrow(cleanedName));
        }
      } else {
        mNewName = name;
      }
    } else if ((index.column() == COLUMN_ENABLED) &&
               role == Qt::CheckStateRole) {
      const bool checked = (value.toInt() == Qt::Checked);
      if (item && cmdFpt) {
        QSet<Uuid> models = mFootprint->getModels();
        if (checked) {
          models.insert(item->getUuid());
        } else {
          models.remove(item->getUuid());
        }
        cmdFpt->setModels(models);
      } else if (!item) {
        mNewEnabled = checked;
      }
    } else {
      return false;  // do not execute command!
    }
    if (cmd) {
      execCmd(cmd.release());
    }
    if (cmdFpt) {
      execCmd(cmdFpt.release());
    }
    if (!item) {
      emit dataChanged(index, index);
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Error"), e.getMsg());
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageModelListModel::modelListEdited(
    const PackageModelList& list, int index,
    const std::shared_ptr<const PackageModel>& obj,
    PackageModelList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(obj);
  switch (event) {
    case PackageModelList::Event::ElementAdded:
      beginInsertRows(QModelIndex(), index, index);
      endInsertRows();
      break;
    case PackageModelList::Event::ElementRemoved:
      beginRemoveRows(QModelIndex(), index, index);
      endRemoveRows();
      break;
    case PackageModelList::Event::ElementEdited:
      dataChanged(this->index(index, 0), this->index(index, _COLUMN_COUNT - 1));
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "PackageModelListModel::modelListEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void PackageModelListModel::footprintEdited(const Footprint& obj,
                                            Footprint::Event event) noexcept {
  Q_UNUSED(obj);
  if (event == Footprint::Event::ModelsChanged) {
    dataChanged(index(0, COLUMN_ENABLED),
                index(rowCount() - 1, COLUMN_ENABLED));
  }
}

void PackageModelListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    QScopedPointer<UndoCommand> cmdGuard(cmd);
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
  QSettings clientSettings;
  QString key = "library_editor/package_editor/step_file";
  QString initialFp = clientSettings.value(key, QDir::homePath()).toString();
  FilePath fp(FileDialog::getOpenFileName(
      qobject_cast<QWidget*>(parent()), tr("Choose STEP Model"), initialFp,
      "STEP Models (*.step *.stp *.STEP *.STP *.Step *.Stp)"));
  if (selectedFile) {
    *selectedFile = fp;
  }
  if (!fp.isValid()) {
    return false;
  }
  clientSettings.setValue(key, fp.toStr());

  tl::optional<QString> minifyError;
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
