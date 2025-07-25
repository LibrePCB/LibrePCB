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
#include "footprintlistmodel.h"

#include "../../undocommand.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../cmd/cmdfootprintedit.h"

#include <librepcb/core/library/pkg/package.h>

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

FootprintListModel::FootprintListModel(QObject* parent) noexcept
  : QObject(parent),
    mPackage(nullptr),
    mUndoStack(nullptr),
    mOnEditedSlot(*this, &FootprintListModel::listEdited),
    mOnModelsEditedSlot(*this, &FootprintListModel::modelListEdited) {
}

FootprintListModel::~FootprintListModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintListModel::setReferences(Package* pkg,
                                       UndoStack* stack) noexcept {
  mUndoStack = stack;

  if (pkg == mPackage) return;

  if (mPackage) {
    mPackage->getFootprints().onEdited.detach(mOnEditedSlot);
    mPackage->getModels().onEdited.detach(mOnModelsEditedSlot);
  }

  mPackage = pkg;
  mItems.clear();

  if (mPackage) {
    mPackage->getFootprints().onEdited.attach(mOnEditedSlot);
    mPackage->getModels().onEdited.attach(mOnModelsEditedSlot);

    for (auto obj : mPackage->getFootprints()) {
      mItems.append(createItem(obj));
    }
  }

  notify_reset();
}

void FootprintListModel::add(const QString& name) noexcept {
  if (!mPackage) return;

  try {
    execCmd(new CmdFootprintInsert(
        mPackage->getFootprints(),
        std::make_shared<Footprint>(Uuid::createRandom(),
                                    validateNameOrThrow(cleanElementName(name)),
                                    QString())));
    emit footprintAdded(mPackage->getFootprints().count() - 1);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void FootprintListModel::apply() {
  if ((!mPackage) || (mPackage->getFootprints().count() != mItems.count())) {
    return;
  }

  for (int i = 0; i < mPackage->getFootprints().count(); ++i) {
    const auto& item = mItems.at(i);
    if ((!item.models) ||
        (static_cast<int>(item.models->row_count()) !=
         mPackage->getModels().count())) {
      continue;
    }

    QSet<Uuid> models;
    for (std::size_t k = 0; k < item.models->row_count(); ++k) {
      const auto checked = item.models->row_data(k);
      if (checked && *checked) {
        models.insert(mPackage->getModels().at(k)->getUuid());
      }
    }
    std::shared_ptr<Footprint> fpt = mPackage->getFootprints().value(i);
    if (fpt && (models != fpt->getModels())) {
      std::unique_ptr<CmdFootprintEdit> cmd(new CmdFootprintEdit(*fpt));
      cmd->setModels(models);
      execCmd(cmd.release());  // can throw
    }
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t FootprintListModel::row_count() const {
  return mItems.count();
}

std::optional<ui::FootprintData> FootprintListModel::row_data(
    std::size_t i) const {
  return (i < static_cast<std::size_t>(mItems.count()))
      ? std::make_optional(mItems.at(i))
      : std::nullopt;
}

void FootprintListModel::set_row_data(std::size_t i,
                                      const ui::FootprintData& data) noexcept {
  if ((!mPackage) || (i >= static_cast<std::size_t>(mItems.count()))) {
    return;
  }

  if (auto obj = mPackage->getFootprints().value(i)) {
    if (data.action != ui::FootprintAction::None) {
      // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
      QMetaObject::invokeMethod(
          this, [this, i, obj, a = data.action]() { trigger(i, obj, a); },
          Qt::QueuedConnection);
    } else {
      try {
        std::unique_ptr<CmdFootprintEdit> cmd(new CmdFootprintEdit(*obj));
        const QString nameStr = s2q(data.name);
        if (nameStr != obj->getNames().getDefaultValue()) {
          cmd->setName(validateNameOrThrow(cleanElementName(nameStr)));
        }
        cmd->setModelPosition(std::make_tuple(s2length(data.model_x),
                                              s2length(data.model_y),
                                              s2length(data.model_z)));
        cmd->setModelRotation(std::make_tuple(s2angle(data.model_rx),
                                              s2angle(data.model_ry),
                                              s2angle(data.model_rz)));
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

ui::FootprintData FootprintListModel::createItem(
    const Footprint& obj) noexcept {
  ui::FootprintData data{
      q2s(obj.getUuid().toStr().left(8)),  // ID
      q2s(*obj.getNames().getDefaultValue()),  // Name
      l2s(std::get<0>(obj.getModelPosition())),  // Model X
      l2s(std::get<1>(obj.getModelPosition())),  // Model Y
      l2s(std::get<2>(obj.getModelPosition())),  // Model Z
      l2s(std::get<0>(obj.getModelRotation())),  // Model RX
      l2s(std::get<1>(obj.getModelRotation())),  // Model RY
      l2s(std::get<2>(obj.getModelRotation())),  // Model RZ
      std::make_shared<slint::VectorModel<bool>>(),  // Checked 3D models
      ui::FootprintAction::None,  // Action
  };
  updateModels(obj, data);
  return data;
}

void FootprintListModel::updateModels(const Footprint& obj,
                                      ui::FootprintData& item) noexcept {
  if (auto models =
          std::dynamic_pointer_cast<slint::VectorModel<bool>>(item.models)) {
    std::vector<bool> values;
    for (const auto& model : mPackage->getModels()) {
      values.push_back(obj.getModels().contains(model.getUuid()));
    }
    models->set_vector(values);
  }
}

void FootprintListModel::trigger(int index, std::shared_ptr<Footprint> obj,
                                 ui::FootprintAction a) noexcept {
  if ((!mPackage) || (!obj) ||
      (mPackage->getFootprints().value(index) != obj)) {
    return;
  }

  try {
    if (a == ui::FootprintAction::MoveUp) {
      const int index = mPackage->getFootprints().indexOf(obj.get());
      execCmd(
          new CmdFootprintsSwap(mPackage->getFootprints(), index, index - 1));
    } else if (a == ui::FootprintAction::Duplicate) {
      ElementName newName("Copy of " % obj->getNames().getDefaultValue());
      std::shared_ptr<Footprint> copy(
          new Footprint(Uuid::createRandom(), newName, ""));  // can throw
      copy->getDescriptions() = obj->getDescriptions();
      copy->setModelPosition(obj->getModelPosition());
      copy->setModelRotation(obj->getModelRotation());
      copy->setModels(obj->getModels());
      copy->getPads() = obj->getPads();
      copy->getPolygons() = obj->getPolygons();
      copy->getCircles() = obj->getCircles();
      copy->getStrokeTexts() = obj->getStrokeTexts();
      copy->getZones() = obj->getZones();
      copy->getHoles() = obj->getHoles();
      execCmd(new CmdFootprintInsert(mPackage->getFootprints(), copy));
      emit footprintAdded(mPackage->getFootprints().count() - 1);
    } else if (a == ui::FootprintAction::Delete) {
      execCmd(new CmdFootprintRemove(mPackage->getFootprints(), obj.get()));
    }
  } catch (const Exception& e) {
    qCritical() << e.getMsg();
  }
}

void FootprintListModel::listEdited(
    const FootprintList& list, int index,
    const std::shared_ptr<const Footprint>& item,
    FootprintList::Event event) noexcept {
  Q_UNUSED(list);

  switch (event) {
    case FootprintList::Event::ElementAdded:
      mItems.insert(index, createItem(*item));
      notify_row_added(index, 1);
      break;
    case FootprintList::Event::ElementRemoved:
      mItems.remove(index);
      notify_row_removed(index, 1);
      break;
    case FootprintList::Event::ElementEdited:
      mItems[index] = createItem(*item);
      notify_row_changed(index);
      break;
    default:
      qWarning() << "Unhandled switch-case in "
                    "FootprintListModel::listEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void FootprintListModel::modelListEdited(
    const PackageModelList& list, int index,
    const std::shared_ptr<const PackageModel>& item,
    PackageModelList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(item);

  if (mPackage &&
      ((event == PackageModelList::Event::ElementAdded) ||
       (event == PackageModelList::Event::ElementRemoved))) {
    for (int i = 0; i < mItems.count(); ++i) {
      if (auto fpt = mPackage->getFootprints().value(i)) {
        updateModels(*fpt, mItems[i]);
      }
    }
    notify_reset();
  }
}

void FootprintListModel::execCmd(UndoCommand* cmd) {
  if (mUndoStack) {
    mUndoStack->execCmd(cmd);
  } else {
    std::unique_ptr<UndoCommand> cmdGuard(cmd);
    cmdGuard->execute();
  }
}

ElementName FootprintListModel::validateNameOrThrow(const QString& name) const {
  if (mPackage) {
    for (const Footprint& footprint : mPackage->getFootprints()) {
      if (footprint.getNames().getDefaultValue() == name) {
        throw RuntimeError(
            __FILE__, __LINE__,
            tr("There is already a footprint with the name \"%1\".").arg(name));
      }
    }
  }
  return ElementName(name);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
