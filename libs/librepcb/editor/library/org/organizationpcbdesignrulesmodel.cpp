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
#include "organizationpcbdesignrulesmodel.h"

#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../cmd/cmdorganizationedit.h"

#include <librepcb/core/library/org/organization.h>

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

OrganizationPcbDesignRulesModel::OrganizationPcbDesignRulesModel(
    QObject* parent) noexcept
  : QObject(parent) {
}

OrganizationPcbDesignRulesModel::~OrganizationPcbDesignRulesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OrganizationPcbDesignRulesModel::setReferences(
    Organization* organization, UndoStack* stack,
    std::function<void(OrganizationPcbDesignRules&)> editCallback) noexcept {
  if ((organization == mOrganization) && (stack == mUndoStack)) {
    return;
  }

  if (mOrganization) {
    disconnect(mOrganization, &Organization::pcbDesignRulesModified, this,
               &OrganizationPcbDesignRulesModel::refresh);
  }

  mOrganization = organization;
  mUndoStack = stack;
  mEditCallback = editCallback;

  if (mOrganization) {
    connect(mOrganization.get(), &Organization::pcbDesignRulesModified, this,
            &OrganizationPcbDesignRulesModel::refresh);
  }

  notify_reset();
}

void OrganizationPcbDesignRulesModel::addItem() noexcept {
  if ((!mOrganization) || (!mUndoStack)) return;

  try {
    const QString name = askForName(QString());
    if (name.isEmpty()) return;

    auto list = mOrganization->getPcbDesignRules();
    list.append(OrganizationPcbDesignRules(
        Uuid::createRandom(), ElementName(cleanElementName(name)), QString(),
        QUrl(), BoardDesignRuleCheckSettings()));
    setList(list);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t OrganizationPcbDesignRulesModel::row_count() const {
  return mOrganization ? mOrganization->getPcbDesignRules().count() : 9;
}

std::optional<ui::OrganizationPcbDesignRulesData>
    OrganizationPcbDesignRulesModel::row_data(std::size_t i) const {
  if ((!mOrganization) ||
      (i >=
       static_cast<std::size_t>(mOrganization->getPcbDesignRules().count()))) {
    return std::nullopt;
  }

  return ui::OrganizationPcbDesignRulesData{
      q2s(*mOrganization->getPcbDesignRules()
               .at(i)
               .getNames()
               .getDefaultValue()),  // Name
      ui::OrganizationPcbDesignRulesAction::None,  // Action
  };
}

void OrganizationPcbDesignRulesModel::set_row_data(
    std::size_t i, const ui::OrganizationPcbDesignRulesData& data) noexcept {
  if ((!mOrganization) ||
      (i >=
       static_cast<std::size_t>(mOrganization->getPcbDesignRules().count()))) {
    return;
  }

  if (data.action != ui::OrganizationPcbDesignRulesAction::None) {
    const auto& obj = mOrganization->getPcbDesignRules().at(i);
    // if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0): Remove lambda.
    QMetaObject::invokeMethod(
        this,
        [this, i, uuid = obj.getUuid(), a = data.action]() {
          trigger(i, uuid, a);
        },
        Qt::QueuedConnection);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OrganizationPcbDesignRulesModel::refresh() noexcept {
  notify_reset();
}

void OrganizationPcbDesignRulesModel::trigger(
    int index, const Uuid& uuid,
    ui::OrganizationPcbDesignRulesAction a) noexcept {
  if ((!mOrganization) || (!mUndoStack) ||
      (index >= mOrganization->getPcbDesignRules().count()) ||
      (mOrganization->getPcbDesignRules().at(index).getUuid() != uuid)) {
    return;
  }

  try {
    auto list = mOrganization->getPcbDesignRules();
    if (a == ui::OrganizationPcbDesignRulesAction::Edit) {
      if (mEditCallback) {
        mEditCallback(list[index]);  // can throw
      }
    } else if (a == ui::OrganizationPcbDesignRulesAction::Rename) {
      auto names = list[index].getNames();
      const QString name = askForName(*names.getDefaultValue());
      if (name.isEmpty()) return;
      names.setDefaultValue(ElementName(cleanElementName(name)));
      list[index].setNames(names);
    } else if (a == ui::OrganizationPcbDesignRulesAction::Duplicate) {
      auto copy = list.at(index);
      copy.setUuid(Uuid::createRandom());
      auto names = copy.getNames();
      const QString name =
          askForName(tr("Copy of %1").arg(*names.getDefaultValue()));
      if (name.isEmpty()) return;
      names.setDefaultValue(ElementName(cleanElementName(name)));  // can throw
      copy.setNames(names);
      list.append(copy);
    } else if (a == ui::OrganizationPcbDesignRulesAction::Delete) {
      list.removeAt(index);
    }
    setList(list);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), "Error", e.getMsg());
  }
}

void OrganizationPcbDesignRulesModel::setList(
    const QVector<OrganizationPcbDesignRules>& list) {
  std::unique_ptr<CmdOrganizationEdit> cmd(
      new CmdOrganizationEdit(*mOrganization));
  cmd->setPcbDesignRules(list);
  mUndoStack->execCmd(cmd.release());
}

QString OrganizationPcbDesignRulesModel::askForName(
    const QString& defaultValue) const {
  return QInputDialog::getText(qApp->activeWindow(),
                               tr("PCB Design Rules Name"),
                               tr("Name of the PCB design rules:"),
                               QLineEdit::EchoMode::Normal, defaultValue);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
