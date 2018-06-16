/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
#include "circuit.h"
#include "../project.h"
#include "netclass.h"
#include "netsignal.h"
#include "componentinstance.h"
#include <librepcb/library/cmp/component.h>
#include "../settings/projectsettings.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Project& project, bool restore, bool readOnly, bool create) :
    QObject(&project), mProject(project),
    mFilepath(project.getPath().getPathTo("circuit/circuit.lp")), mFile(nullptr)
{
    qDebug() << "load circuit...";
    Q_ASSERT(!(create && (restore || readOnly)));

    try
    {
        // try to create/open the file "circuit.lp"
        if (create)
        {
            mFile = SmartSExprFile::create(mFilepath);
            NetClass* netclass = new NetClass(*this, "default");
            addNetClass(*netclass); // add a netclass with name "default"
        }
        else
        {
            mFile = new SmartSExprFile(mFilepath, restore, readOnly);
            SExpression root = mFile->parseFileAndBuildDomTree();

            // OK - file is open --> now load the whole circuit stuff

            // Load all netclasses
            foreach (const SExpression& node, root.getChildren("netclass")) {
                NetClass* netclass = new NetClass(*this, node);
                addNetClass(*netclass);
            }

            // Load all netsignals
            foreach (const SExpression& node, root.getChildren("net")) {
                NetSignal* netsignal = new NetSignal(*this, node);
                addNetSignal(*netsignal);
            }

            // Load all component instances
            foreach (const SExpression& node, root.getChildren("component")) {
                ComponentInstance* component = new ComponentInstance(*this, node);
                addComponentInstance(*component);
            }
        }
    }
    catch (...)
    {
        // free allocated memory (see comments in the destructor) and rethrow the exception
        foreach (ComponentInstance* compInstance, mComponentInstances)
            try { removeComponentInstance(*compInstance); delete compInstance; } catch (...) {}
        foreach (NetSignal* netsignal, mNetSignals)
            try { removeNetSignal(*netsignal); delete netsignal; } catch (...) {}
        foreach (NetClass* netclass, mNetClasses)
            try { removeNetClass(*netclass); delete netclass; } catch (...) {}
        delete mFile;            mFile = nullptr;
        throw;
    }

    qDebug() << "circuit successfully loaded!";
}

Circuit::~Circuit() noexcept
{
    // delete all component instances (and catch all throwed exceptions)
    foreach (ComponentInstance* compInstance, mComponentInstances)
        try { removeComponentInstance(*compInstance); delete compInstance; } catch (...) {}

    // delete all netsignals (and catch all throwed exceptions)
    foreach (NetSignal* netsignal, mNetSignals)
        try { removeNetSignal(*netsignal); delete netsignal; } catch (...) {}

    // delete all netclasses (and catch all throwed exceptions)
    foreach (NetClass* netclass, mNetClasses)
        try { removeNetClass(*netclass); delete netclass; } catch (...) {}

    delete mFile;            mFile = nullptr;
}

/*****************************************************************************************
 *  NetClass Methods
 ****************************************************************************************/

NetClass* Circuit::getNetClassByUuid(const Uuid& uuid) const noexcept
{
    return mNetClasses.value(uuid, nullptr);
}

NetClass* Circuit::getNetClassByName(const QString& name) const noexcept
{
    foreach (NetClass* netclass, mNetClasses) {
        if (netclass->getName() == name) {
            return netclass;
        }
    }
    return nullptr;
}

void Circuit::addNetClass(NetClass& netclass)
{
    if (&netclass.getCircuit() != this) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netclass with the same uuid in the list
    if (getNetClassByUuid(netclass.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net class with the UUID \"%1\"!"))
            .arg(netclass.getUuid().toStr()));
    }
    // check if there is no netclass with the same name in the list
    if (getNetClassByName(netclass.getName())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net class with the name \"%1\"!"))
            .arg(netclass.getName()));
    }
    // add netclass to circuit
    netclass.addToCircuit(); // can throw
    mNetClasses.insert(netclass.getUuid(), &netclass);
    emit netClassAdded(netclass);
}

void Circuit::removeNetClass(NetClass& netclass)
{
    // check if the netclass was added to the circuit
    if (mNetClasses.value(netclass.getUuid()) != &netclass) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove netclass from project
    netclass.removeFromCircuit(); // can throw
    mNetClasses.remove(netclass.getUuid());
    emit netClassRemoved(netclass);
}

void Circuit::setNetClassName(NetClass& netclass, const QString& newName)
{
    // check if the netclass was added to the circuit
    if (mNetClasses.value(netclass.getUuid()) != &netclass) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netclass with the same name in the list
    if (getNetClassByName(newName)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net class with the name \"%1\"!")).arg(newName));
    }
    // apply the new name
    netclass.setName(newName); // can throw
}

/*****************************************************************************************
 *  NetSignal Methods
 ****************************************************************************************/

QString Circuit::generateAutoNetSignalName() const noexcept
{
    QString name;
    int i = 1;
    do {
        name = QString("N%1").arg(i++);
    } while (getNetSignalByName(name));
    return name;
}

NetSignal* Circuit::getNetSignalByUuid(const Uuid& uuid) const noexcept
{
    return mNetSignals.value(uuid, nullptr);
}

NetSignal* Circuit::getNetSignalByName(const QString& name) const noexcept
{
    foreach (NetSignal* netsignal, mNetSignals) {
        if (netsignal->getName() == name) {
            return netsignal;
        }
    }
    return nullptr;
}

NetSignal* Circuit:: getNetSignalWithMostElements() const noexcept
{
    NetSignal* netsignal = nullptr;
    foreach (NetSignal* ns, mNetSignals) {
        if ((!netsignal) || (ns->getRegisteredElementsCount() > netsignal->getRegisteredElementsCount())) {
            netsignal = ns;
        }
    }
    return netsignal;
}

void Circuit::addNetSignal(NetSignal& netsignal)
{
    if (&netsignal.getCircuit() != this) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netsignal with the same uuid in the list
    if (getNetSignalByUuid(netsignal.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net signal with the UUID \"%1\"!"))
            .arg(netsignal.getUuid().toStr()));
    }
    // check if there is no netsignal with the same name in the list
    if (getNetSignalByName(netsignal.getName())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net signal with the name \"%1\"!"))
            .arg(netsignal.getName()));
    }
    // add netsignal to circuit
    netsignal.addToCircuit(); // can throw
    mNetSignals.insert(netsignal.getUuid(), &netsignal);
    emit netSignalAdded(netsignal);
}

void Circuit::removeNetSignal(NetSignal& netsignal)
{
    // check if the netsignal was added to the circuit
    if (mNetSignals.value(netsignal.getUuid()) != &netsignal) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove netsignal from circuit
    netsignal.removeFromCircuit(); // can throw
    mNetSignals.remove(netsignal.getUuid());
    emit netSignalRemoved(netsignal);
}

void Circuit::setNetSignalName(NetSignal& netsignal, const QString& newName,
                               bool isAutoName)
{
    // check if the netsignal was added to the circuit
    if (mNetSignals.value(netsignal.getUuid()) != &netsignal) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no netsignal with the same name in the list
    if (getNetSignalByName(newName)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a net signal with the name \"%1\"!")).arg(newName));
    }
    // apply the new name
    netsignal.setName(newName, isAutoName); // can throw
}

void Circuit::setHighlightedNetSignal(NetSignal* signal) noexcept
{
    foreach (NetSignal* netsignal, mNetSignals) {
        netsignal->setHighlighted(signal == netsignal);
    }
}

/*****************************************************************************************
 *  ComponentInstance Methods
 ****************************************************************************************/

QString Circuit::generateAutoComponentInstanceName(const QString& cmpPrefix) const noexcept
{
    QString name;
    int i = 1;
    do {
        name = QString("%1%2").arg(cmpPrefix.isEmpty() ? "?" : cmpPrefix).arg(i++);
    } while (getComponentInstanceByName(name));
    return name;
}

ComponentInstance* Circuit::getComponentInstanceByUuid(const Uuid& uuid) const noexcept
{
    return mComponentInstances.value(uuid, nullptr);
}

ComponentInstance* Circuit::getComponentInstanceByName(const QString& name) const noexcept
{
    foreach (ComponentInstance* compInstance, mComponentInstances)
    {
        if (compInstance->getName() == name)
            return compInstance;
    }
    return nullptr;
}

void Circuit::addComponentInstance(ComponentInstance& cmp)
{
    if (&cmp.getCircuit() != this) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no component with the same uuid in the list
    if (getComponentInstanceByUuid(cmp.getUuid())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a component with the UUID \"%1\"!"))
            .arg(cmp.getUuid().toStr()));
    }
    // check if there is no component with the same name in the list
    if (getComponentInstanceByName(cmp.getName())) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a component with the name \"%1\"!"))
            .arg(cmp.getName()));
    }
    // add to circuit
    cmp.addToCircuit(); // can throw
    mComponentInstances.insert(cmp.getUuid(), &cmp);
    emit componentAdded(cmp);
}

void Circuit::removeComponentInstance(ComponentInstance& cmp)
{
    // check if the component instance was added to the circuit
    if (mComponentInstances.value(cmp.getUuid()) != &cmp) {
        throw LogicError(__FILE__, __LINE__);
    }
    // remove from circuit
    cmp.removeFromCircuit(); // can throw
    mComponentInstances.remove(cmp.getUuid());
    emit componentRemoved(cmp);
}

void Circuit::setComponentInstanceName(ComponentInstance& cmp, const QString& newName)
{
    // check if the component instance was added to the circuit
    if (mComponentInstances.value(cmp.getUuid()) != &cmp) {
        throw LogicError(__FILE__, __LINE__);
    }
    // check if there is no component with the same name in the list
    if ((newName != cmp.getName()) && getComponentInstanceByName(newName)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("There is already a component with the name \"%1\"!")).arg(newName));
    }
    // apply the new name
    cmp.setName(newName); // can throw
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Circuit::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "circuit/circuit.lp"
    try
    {
        SExpression doc(serializeToDomElement("librepcb_circuit"));
        mFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Circuit::serialize(SExpression& root) const
{
    root.appendLineBreak();
    serializePointerContainer(root, mNetClasses, "netclass");
    root.appendLineBreak();
    serializePointerContainer(root, mNetSignals, "net");
    root.appendLineBreak();
    serializePointerContainer(root, mComponentInstances,"component");
    root.appendLineBreak();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
