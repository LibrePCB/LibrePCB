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
#include "cmdassemblyvariantremove.h"

#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdAssemblyVariantRemove::CmdAssemblyVariantRemove(
    Circuit& circuit, std::shared_ptr<AssemblyVariant> av) noexcept
  : UndoCommand(tr("Remove assembly variant")),
    mCircuit(circuit),
    mAssemblyVariant(av),
    mComponentAssemblyOptions(),
    mIndex(-1) {
}

CmdAssemblyVariantRemove::~CmdAssemblyVariantRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAssemblyVariantRemove::performExecute() {
  // Determine assembly options to modify.
  foreach (ComponentInstance* cmp, mCircuit.getComponentInstances()) {
    for (int i = 0; i < cmp->getAssemblyOptions().count(); ++i) {
      if (cmp->getAssemblyOptions().at(i)->getAssemblyVariants().contains(
              mAssemblyVariant->getUuid())) {
        mComponentAssemblyOptions.append(std::make_pair(cmp->getUuid(), i));
      }
    }
  }

  // Memorize index.
  mIndex = mCircuit.getAssemblyVariants().indexOf(mAssemblyVariant.get());
  Q_ASSERT(mIndex >= 0);

  performRedo();  // can throw
  return true;
}

void CmdAssemblyVariantRemove::performUndo() {
  mCircuit.addAssemblyVariant(mAssemblyVariant, mIndex);  // can throw

  foreach (const auto& pair, mComponentAssemblyOptions) {
    if (ComponentInstance* cmp =
            mCircuit.getComponentInstanceByUuid(pair.first)) {
      auto options = cmp->getAssemblyOptions();
      if (auto option = options.value(pair.second)) {
        auto variants = option->getAssemblyVariants();
        Q_ASSERT(!variants.contains(mAssemblyVariant->getUuid()));
        variants.insert(mAssemblyVariant->getUuid());
        option->setAssemblyVariants(variants);
        cmp->setAssemblyOptions(options);
      }
    }
  }
}

void CmdAssemblyVariantRemove::performRedo() {
  mCircuit.removeAssemblyVariant(mAssemblyVariant);  // can throw

  foreach (const auto& pair, mComponentAssemblyOptions) {
    if (ComponentInstance* cmp =
            mCircuit.getComponentInstanceByUuid(pair.first)) {
      auto options = cmp->getAssemblyOptions();
      if (auto option = options.value(pair.second)) {
        auto variants = option->getAssemblyVariants();
        Q_ASSERT(variants.contains(mAssemblyVariant->getUuid()));
        variants.remove(mAssemblyVariant->getUuid());
        option->setAssemblyVariants(variants);
        cmp->setAssemblyOptions(options);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
