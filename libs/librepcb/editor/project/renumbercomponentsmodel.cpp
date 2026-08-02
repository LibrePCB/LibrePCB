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
#include "renumbercomponentsmodel.h"

#include "../undostack.h"
#include "../utils/slinthelpers.h"
#include "cmd/cmdcomponentinstanceedit.h"
#include "projecteditor.h"

#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/items/si_text.h>
#include <librepcb/core/project/schematic/schematic.h>

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

RenumberComponentsModel::RenumberComponentsModel(ProjectEditor& editor,
                                                 QObject* parent) noexcept
  : QObject(parent), mProjectEditor(&editor), mUpToDate(false) {
  connect(&editor.getUndoStack(), &UndoStack::stateModified, this,
          &RenumberComponentsModel::invalidate);
}

RenumberComponentsModel::~RenumberComponentsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RenumberComponentsModel::apply(QWidget* parent) noexcept {
  if (!mProjectEditor) {
    return;
  }

  try {
    UndoStackTransaction transaction(mProjectEditor->getUndoStack(),
                                     tr("Re-Number Components"));

    // Rename to a temporary name first to avoid conflicts.
    int tmp = 0;
    for (auto it = mRenames.begin(); it != mRenames.end(); it++) {
      if (!it.key()) throw LogicError(__FILE__, __LINE__);
      std::unique_ptr<CmdComponentInstanceEdit> cmd =
          std::make_unique<CmdComponentInstanceEdit>(
              mProjectEditor->getProject().getCircuit(), *it.key());
      cmd->setName(CircuitIdentifier(QString("_tmp_%1").arg(++tmp)));
      transaction.append(cmd.release());  // can throw
    }

    // Now rename to the desired designator.
    for (auto it = mRenames.begin(); it != mRenames.end(); it++) {
      std::unique_ptr<CmdComponentInstanceEdit> cmd =
          std::make_unique<CmdComponentInstanceEdit>(
              mProjectEditor->getProject().getCircuit(), *it.key());
      cmd->setName(CircuitIdentifier(it.value()));  // can throw
      transaction.append(cmd.release());  // can throw
    }

    transaction.commit();  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t RenumberComponentsModel::row_count() const {
  update();
  return mItems.size();
}

std::optional<ui::RenumberComponentsItemData> RenumberComponentsModel::row_data(
    std::size_t i) const {
  update();
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RenumberComponentsModel::invalidate() noexcept {
  mUpToDate = false;
  notify_reset();
}

static void traverseSymbolGroup(QVector<SI_Symbol*>& group,
                                SI_Symbol* sym) noexcept {
  group.append(sym);
  for (SI_SymbolPin* pin : sym->getPins()) {
    if (SI_NetSegment* ns = pin->getNetSegmentOfLines()) {
      foreach (SI_SymbolPin* otherPin, ns->getAllConnectedPins()) {
        if (!group.contains(&otherPin->getSymbol())) {
          traverseSymbolGroup(group, &otherPin->getSymbol());
        }
      }
    }
  }
}

void RenumberComponentsModel::update() const noexcept {
  if (mUpToDate) {
    return;
  }

  mRenames.clear();
  mItems.clear();
  mUpToDate = true;
  if (!mProjectEditor) {
    return;
  }

  qDebug() << "Refreshing renumber components model...";
  Project& project = mProjectEditor->getProject();

  // Find groups of interconnected symbols.
  QVector<QVector<SI_Symbol*>> symGroups;
  QSet<SI_Symbol*> traversedSymbols;
  for (Schematic* sch : project.getSchematics()) {
    for (SI_Symbol* sym : sch->getSymbols()) {
      if (!traversedSymbols.contains(sym)) {
        QVector<SI_Symbol*> group;
        traverseSymbolGroup(group, sym);
        symGroups.append(group);
        for (SI_Symbol* s : std::as_const(group)) {
          traversedSymbols.insert(s);
        }
      }
    }
  }

  // Sort groups by schematic page & position of top-left symbol.
  for (QVector<SI_Symbol*>& group : symGroups) {
    std::stable_sort(group.begin(), group.end(),
                     [](const SI_Symbol* a, const SI_Symbol* b) {
                       const Point ap = a->getPosition();
                       const Point bp = b->getPosition();
                       if (ap.getX() != bp.getX()) {
                         return ap.getX() < bp.getX();
                       } else {
                         return ap.getY() > bp.getY();
                       }
                     });
  }
  std::stable_sort(
      symGroups.begin(), symGroups.end(),
      [&](const QVector<SI_Symbol*>& a, const QVector<SI_Symbol*>& b) {
        const int ai = project.getSchematicIndex(a.first()->getSchematic());
        const int bi = project.getSchematicIndex(b.first()->getSchematic());
        if (ai != bi) {
          return ai < bi;
        }
        const Point ap = a.first()->getPosition();
        const Point bp = b.first()->getPosition();
        if (ap.getX() != bp.getX()) {
          return ap.getX() < bp.getX();
        } else {
          return ap.getY() > bp.getY();
        }
      });

  // Helper to determine the next free designator.
  QHash<QString, int> numbers;
  const QRegularExpression re("^([^0-9]*)([0-9]*)(.*)");
  auto getNewName = [&numbers, &re](const CircuitIdentifier& name) {
    const QString prefix = re.match(*name).captured(1);  // NOLINT
    const QString suffix = re.match(*name).captured(3);  // NOLINT
    const int number = numbers.value(prefix, 0) + 1;
    numbers.insert(prefix, number);
    return prefix + QString::number(number) + suffix;
  };

  // Determine new designators.
  // Also take into account all of the remaining components which are not
  // added to any circuit. This is important to avoid potential name conflicts
  // with those components (e.g. if a hidden component is named "C1", it has
  // to be renamed to allow using "C1" for one of the visible components).
  for (QVector<SI_Symbol*>& group : symGroups) {
    for (SI_Symbol* sym : group) {
      ComponentInstance& cmp = sym->getComponentInstance();
      if (!mRenames.contains(&cmp)) {
        mRenames.insert(&cmp, getNewName(cmp.getName()));
      }
    }
  }
  for (ComponentInstance* cmp : project.getCircuit().getComponentInstances()) {
    if (!mRenames.contains(cmp)) {
      mRenames.insert(cmp, getNewName(cmp->getName()));
    }
  }

  // Create model items.
  auto hideComponent = [](const ComponentInstance* cmp) {
    if (!cmp->isPureSchematicOnly()) {
      return false;
    }
    for (const SI_Symbol* sym : cmp->getSymbols()) {
      for (const SI_Text* text : sym->getTexts()) {
        if (text->getTextObj().getText().contains("NAME")) {
          return false;
        }
      }
    }
    return true;
  };
  for (auto it = mRenames.begin(); it != mRenames.end(); it++) {
    if ((it.key()->getName() != it.value()) && (!hideComponent(it.key()))) {
      ProjectAttributeLookup lookup(*it.key(), nullptr, nullptr);
      const QString value =
          AttributeSubstitutor::substitute(it.key()->getValue(), lookup)
              .simplified();
      mItems.push_back(ui::RenumberComponentsItemData{
          q2s(*it.key()->getName()), q2s(it.value()), q2s(value)});
    }
  }
  Toolbox::sortNumeric(
      mItems,
      [](const QCollator& collator, const ui::RenumberComponentsItemData& a,
         const ui::RenumberComponentsItemData& b) {
        return collator(s2q(a.name), s2q(b.name));
      });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
