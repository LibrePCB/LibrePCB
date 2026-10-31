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
#include <librepcb/rust-core/ffi.h>

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
    Circuit& circuit = mProjectEditor->getProject().getCircuit();
    UndoStackTransaction transaction(mProjectEditor->getUndoStack(),
                                     tr("Renumber Components"));

    // Rename to a temporary name first to avoid conflicts.
    int index = 0;
    for (auto it = mRenames.begin(); it != mRenames.end(); it++) {
      if (!it.key()) throw LogicError(__FILE__, __LINE__);
      QString tmpName;
      do {
        tmpName = QString("_tmp_%1").arg(++index);
      } while (circuit.getComponentInstanceByName(tmpName));
      std::unique_ptr<CmdComponentInstanceEdit> cmd =
          std::make_unique<CmdComponentInstanceEdit>(circuit, *it.key());
      cmd->setName(CircuitIdentifier(tmpName));
      transaction.append(cmd.release());  // can throw
    }

    // Now rename to the desired designator.
    for (auto it = mRenames.begin(); it != mRenames.end(); it++) {
      std::unique_ptr<CmdComponentInstanceEdit> cmd =
          std::make_unique<CmdComponentInstanceEdit>(circuit, *it.key());
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

  // Collect all components (in circuit order).
  QVector<ComponentInstance*> components;
  QHash<ComponentInstance*, std::size_t> componentIndices;
  QStringList names;
  for (ComponentInstance* cmp : project.getCircuit().getComponentInstances()) {
    componentIndices.insert(cmp, componentIndices.count());
    components.append(cmp);
    names.append(*cmp->getName());
  }

  // Collect all symbols (ordered by schematic and their order within) and
  // their electrical connectivity as (symbol index, net segment ID) pairs.
  QVector<rs::RenumberComponentsSymbolData> symbols;
  QVector<std::size_t> connections;
  for (int i = 0; i < project.getSchematics().count(); ++i) {
    Schematic* sch = project.getSchematicByIndex(i);
    Q_ASSERT(sch);
    for (SI_Symbol* sym : sch->getSymbols()) {
      const std::size_t symIndex = static_cast<std::size_t>(symbols.count());
      symbols.append(rs::RenumberComponentsSymbolData{
          componentIndices.value(&sym->getComponentInstance()),
          static_cast<std::size_t>(i),
          sym->getPosition().getX().toNm(),
          sym->getPosition().getY().toNm(),
      });
      for (SI_SymbolPin* pin : sym->getPins()) {
        if (SI_NetSegment* ns = pin->getNetSegmentOfLines()) {
          connections.append(symIndex);
          connections.append(reinterpret_cast<std::size_t>(ns));
        }
      }
    }
  }

  // Run the renumbering algorithm to determine the new designators.
  // This also takes into account all of the components which are not added
  // to any schematic. This is important to avoid potential name conflicts
  // with those components (e.g. if a hidden component is named "C1", it has
  // to be renamed to allow using "C1" for one of the visible components).
  QStringList newNames;
  rs::ffi_renumber_components(
      &names, symbols.constData(), static_cast<std::size_t>(symbols.count()),
      connections.constData(), static_cast<std::size_t>(connections.count()),
      &newNames);
  Q_ASSERT(newNames.count() == names.count());
  for (int i = 0; i < components.count(); ++i) {
    mRenames.insert(components[i], newNames[i]);
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
      ProjectAttributeLookup lookup(*it.key(), {}, {});
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
