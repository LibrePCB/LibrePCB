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

#ifndef LIBREPCB_CORE_CIRCUIT_H
#define LIBREPCB_CORE_CIRCUIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../fileio/filepath.h"
#include "../../library/cmp/componentprefix.h"
#include "../../types/circuitidentifier.h"
#include "../../types/elementname.h"
#include "../../types/fileproofname.h"
#include "../../types/uuid.h"
#include "assemblyvariant.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentInstance;
class NetClass;
class NetSignal;
class Project;
class TransactionalDirectory;

/*******************************************************************************
 *  Class Circuit
 ******************************************************************************/

/**
 * @brief   The Circuit class represents all electrical connections in a project
 * (drawn in the schematics)
 *
 * Each ::librepcb::Project object contains exactly one
 * ::librepcb::Circuit object which contains the whole electrical
 * components and connections. They are created with the schematic editor and
 * used by the board editor. The whole circuit is saved in the file
 * "circuit.lp" in the project's "circuit" directory.
 *
 * Each #Circuit object contains:
 *  - All assembly variants (::librepcb::AssemblyVariant objects)
 *  - All net classes (::librepcb::NetClass objects)
 *  - All net signals (::librepcb::NetSignal objects)
 *  - All component instances (::librepcb::ComponentInstance objects)
 */
class Circuit final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  Circuit() = delete;
  Circuit(const Circuit& other) = delete;
  explicit Circuit(Project& project);
  ~Circuit() noexcept;

  // Getters
  Project& getProject() const noexcept { return mProject; }

  // AssemblyVariant Methods
  AssemblyVariantList& getAssemblyVariants() noexcept {
    return mAssemblyVariants;
  }
  const AssemblyVariantList& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  int addAssemblyVariant(std::shared_ptr<AssemblyVariant> av, int index = -1);
  void removeAssemblyVariant(std::shared_ptr<AssemblyVariant> av);
  void setAssemblyVariantName(std::shared_ptr<AssemblyVariant> av,
                              const FileProofName& newName);

  // NetClass Methods
  const QMap<Uuid, NetClass*>& getNetClasses() const noexcept {
    return mNetClasses;
  }
  NetClass* getNetClassByName(const ElementName& name) const noexcept;
  void addNetClass(NetClass& netclass);
  void removeNetClass(NetClass& netclass);
  void setNetClassName(NetClass& netclass, const ElementName& newName);

  // NetSignal Methods
  QString generateAutoNetSignalName() const noexcept;
  const QMap<Uuid, NetSignal*>& getNetSignals() const noexcept {
    return mNetSignals;
  }
  NetSignal* getNetSignalByName(const QString& name) const noexcept;
  NetSignal* getNetSignalWithMostElements() const noexcept;
  void addNetSignal(NetSignal& netsignal);
  void removeNetSignal(NetSignal& netsignal);
  void setNetSignalName(NetSignal& netsignal, const CircuitIdentifier& newName,
                        bool isAutoName);

  // ComponentInstance Methods
  QString generateAutoComponentInstanceName(
      const ComponentPrefix& cmpPrefix) const noexcept;
  const QMap<Uuid, ComponentInstance*>& getComponentInstances() const noexcept {
    return mComponentInstances;
  }
  ComponentInstance* getComponentInstanceByUuid(
      const Uuid& uuid) const noexcept;
  ComponentInstance* getComponentInstanceByName(
      const QString& name) const noexcept;
  void addComponentInstance(ComponentInstance& cmp);
  void removeComponentInstance(ComponentInstance& cmp);
  void setComponentInstanceName(ComponentInstance& cmp,
                                const CircuitIdentifier& newName);

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  Circuit& operator=(const Circuit& rhs) = delete;
  bool operator==(const Circuit& rhs) const noexcept { return (this == &rhs); }
  bool operator!=(const Circuit& rhs) const noexcept { return (this != &rhs); }

signals:
  void assemblyVariantAdded(std::shared_ptr<AssemblyVariant>& av);
  void assemblyVariantRemoved(std::shared_ptr<AssemblyVariant>& av);
  void netClassAdded(NetClass& netclass);
  void netClassRemoved(NetClass& netclass);
  void netSignalAdded(NetSignal& netsignal);
  void netSignalRemoved(NetSignal& netsignal);
  void componentAdded(ComponentInstance& cmp);
  void componentRemoved(ComponentInstance& cmp);
  void netClassDesignRulesModified();

private:
  Project& mProject;  ///< A reference to the Project object (from the ctor)
  AssemblyVariantList mAssemblyVariants;
  QMap<Uuid, NetClass*> mNetClasses;
  QMap<Uuid, NetSignal*> mNetSignals;
  QMap<Uuid, ComponentInstance*> mComponentInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
