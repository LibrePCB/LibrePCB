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
#include "newelementwizardpage_componentsignals.h"

#include "ui_newelementwizardpage_componentsignals.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_ComponentSignals::NewElementWizardPage_ComponentSignals(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_ComponentSignals) {
  mUi->setupUi(this);
}

NewElementWizardPage_ComponentSignals::
    ~NewElementWizardPage_ComponentSignals() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_ComponentSignals::validatePage() noexcept {
  return true;
}

bool NewElementWizardPage_ComponentSignals::isComplete() const noexcept {
  return true;
}

int NewElementWizardPage_ComponentSignals::nextId() const noexcept {
  return NewElementWizardContext::ID_ComponentPinSignalMap;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QString NewElementWizardPage_ComponentSignals::appendNumberToSignalName(
    QString name, int number) noexcept {
  name.truncate(CircuitIdentifierConstraint::MAX_LENGTH - 4);
  if ((!name.isEmpty()) && (name.back().isDigit())) {
    name.append("_");
  }
  name.append(QString::number(number));
  return cleanCircuitIdentifier(name);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QHash<Uuid, CircuitIdentifier>
    NewElementWizardPage_ComponentSignals::getPinNames(
        const Uuid& symbol) const noexcept {
  QHash<Uuid, CircuitIdentifier> names;
  try {
    FilePath fp = mContext.getWorkspace().getLibraryDb().getLatest<Symbol>(
        symbol);  // can throw
    std::unique_ptr<Symbol> symbol = Symbol::open(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            TransactionalFileSystem::openRO(fp))));  // can throw
    for (const SymbolPin& pin : symbol->getPins()) {
      names.insert(pin.getUuid(), pin.getName());
    }
  } catch (const Exception& e) {
    // TODO: what could we do here?
  }
  return names;
}

void NewElementWizardPage_ComponentSignals::initializePage() noexcept {
  QWizardPage::initializePage();

  // Automatically create signals if no signals exist.
  const ComponentSymbolVariant* variant =
      mContext.mComponentSymbolVariants.value(0).get();
  if (variant && mContext.mComponentSignals.count() < 1) {
    // First collect all pin names to allow making signal names unique
    // (https://github.com/LibrePCB/LibrePCB/issues/1425).
    QHash<std::pair<Uuid, Uuid>, QString> names;
    for (const ComponentSymbolVariantItem& item : variant->getSymbolItems()) {
      QHash<Uuid, CircuitIdentifier> tmp = getPinNames(item.getSymbolUuid());
      for (auto it = tmp.begin(); it != tmp.end(); it++) {
        names.insert(std::make_pair(item.getUuid(), it.key()), *it.value());
      }
    }

    // Now add the signals.
    QSet<QString> usedNames = Toolbox::toSet(names.values());
    for (const ComponentSymbolVariantItem& item : variant->getSymbolItems()) {
      for (const ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
        QString name =
            names.value(std::make_pair(item.getUuid(), map.getPinUuid()));
        if (names.values().count(name) > 1) {
          // Append number to make the signal name unique.
          int number = 1;
          while (usedNames.contains(appendNumberToSignalName(name, number))) {
            ++number;
          }
          name = appendNumberToSignalName(name, number);
          usedNames.insert(name);
        }
        if (CircuitIdentifierConstraint()(name)) {
          mContext.mComponentSignals.append(std::make_shared<ComponentSignal>(
              Uuid::createRandom(), CircuitIdentifier(name),
              SignalRole::passive(), QString(), false, false, false));
        }
      }
    }
  }

  mUi->signalListEditorWidget->setReferences(nullptr,
                                             &mContext.mComponentSignals);
}

void NewElementWizardPage_ComponentSignals::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
