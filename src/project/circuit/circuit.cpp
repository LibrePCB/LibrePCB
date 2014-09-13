/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QtWidgets>
#include <QDomDocument>
#include "../../common/exceptions.h"
#include "../../common/xmlfile.h"
#include "circuit.h"
#include "../project.h"
#include "netclass.h"
#include "netsignal.h"
#include "genericcomponentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Workspace& workspace, Project& project, bool restore, bool isNew) throw (Exception) :
    QObject(0), mWorkspace(workspace), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/circuit.xml")), mXmlFile(0)
{
    qDebug() << "load circuit...";

    try
    {
        // try to open the XML file "circuit.xml"
        mXmlFile = new XmlFile(mXmlFilepath, restore, "circuit");

        // OK - XML file is open --> now load the whole circuit stuff

        QDomElement tmpNode; // for temporary use...
        QDomElement root = mXmlFile->getRoot();

        // Load all netclasses
        if (isNew)
        {
            root.appendChild(mXmlFile->getDocument().createElement("netclasses"));
            addNetClass(createNetClass("default"));
        }
        else
        {
            tmpNode = root.firstChildElement("netclasses").firstChildElement("netclass");
            while (!tmpNode.isNull())
            {
                NetClass* netclass = new NetClass(tmpNode);
                addNetClass(netclass, false);
                tmpNode = tmpNode.nextSiblingElement("netclass");
            }
        }
        qDebug() << mNetClasses.count() << "netclasses successfully loaded!";

        // Load all netsignals
        if (isNew)
        {
            root.appendChild(mXmlFile->getDocument().createElement("netsignals"));
        }
        else
        {
            tmpNode = root.firstChildElement("netsignals").firstChildElement("netsignal");
            while (!tmpNode.isNull())
            {
                NetSignal* netsignal = new NetSignal(*this, tmpNode);
                addNetSignal(netsignal, false);
                tmpNode = tmpNode.nextSiblingElement("netsignal");
            }
        }
        qDebug() << mNetSignals.count() << "netsignals successfully loaded!";

        // Load all generic component instances
        tmpNode = root.firstChildElement("generic_component_instances")
                  .firstChildElement("instance");
        while (!tmpNode.isNull())
        {
            GenericComponentInstance* genComp = new GenericComponentInstance(*this, tmpNode);
            addGenCompInstance(genComp, false);
            tmpNode = tmpNode.nextSiblingElement("instance");
        }
        qDebug() << mGenCompInstances.count() << "generic component instances successfully loaded!";
    }
    catch (...)
    {
        // free allocated memory (see comments in the destructor) and rethrow the exception
        foreach (GenericComponentInstance* genCompInstance, mGenCompInstances)
            try { removeGenCompInstance(genCompInstance, false, true); } catch (...) {}
        foreach (NetSignal* netsignal, mNetSignals)
            try { removeNetSignal(netsignal, false, true); } catch (...) {}
        foreach (NetClass* netclass, mNetClasses)
            try { removeNetClass(netclass, false, true); } catch (...) {}
        delete mXmlFile;            mXmlFile = 0;
        throw;
    }

    qDebug() << "circuit successfully loaded!";
}

Circuit::~Circuit() noexcept
{
    // delete all generic component instances (and catch all throwed exceptions)
    foreach (GenericComponentInstance* genCompInstance, mGenCompInstances)
        try { removeGenCompInstance(genCompInstance, false, true); } catch (...) {}

    // delete all netsignals (and catch all throwed exceptions)
    foreach (NetSignal* netsignal, mNetSignals)
        try { removeNetSignal(netsignal, false, true); } catch (...) {}

    // delete all netclasses (and catch all throwed exceptions)
    foreach (NetClass* netclass, mNetClasses)
        try { removeNetClass(netclass, false, true); } catch (...) {}

    delete mXmlFile;            mXmlFile = 0;
}

/*****************************************************************************************
 *  NetClass Methods
 ****************************************************************************************/

NetClass* Circuit::getNetClassByUuid(const QUuid& uuid) const noexcept
{
    return mNetClasses.value(uuid, 0);
}

NetClass* Circuit::getNetClassByName(const QString& name) const noexcept
{
    foreach (NetClass* netclass, mNetClasses)
    {
        if (netclass->getName() == name)
            return netclass;
    }
    return 0;
}

NetClass* Circuit::createNetClass(const QString& name) throw (Exception)
{
    return NetClass::create(mXmlFile->getDocument(), name);
}

void Circuit::addNetClass(NetClass* netclass, bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(netclass);

    // check if there is no netclass with the same uuid in the list
    if (getNetClassByUuid(netclass->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netclass->getUuid().toString(),
            QString(tr("There is already a netclass with the UUID \"%1\"!"))
            .arg(netclass->getUuid().toString()));
    }

    // check if there is no netclass with the same name in the list
    if (getNetClassByName(netclass->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, netclass->getUuid().toString(),
            QString(tr("There is already a netclass with the name \"%1\"!"))
            .arg(netclass->getName()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netclasses");
    netclass->addToCircuit(toDomTree, parent); // can throw an exception

    mNetClasses.insert(netclass->getUuid(), netclass);
}

void Circuit::removeNetClass(NetClass* netclass, bool fromDomTree, bool deleteNetClass) throw (Exception)
{
    Q_CHECK_PTR(netclass);
    Q_ASSERT(mNetClasses.contains(netclass->getUuid()));

    // the netclass cannot be removed if there are already netsignals with that netclass!
    if (netclass->getNetSignalCount() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(netclass->getUuid().toString()).arg(netclass->getNetSignalCount()),
            QString(tr("There are already signals in the netclass \"%1\"!"))
            .arg(netclass->getName()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netclasses");
    netclass->removeFromCircuit(fromDomTree, parent); // can throw an exception

    mNetClasses.remove(netclass->getUuid());

    if (deleteNetClass)
        delete netclass;
}

/*****************************************************************************************
 *  NetSignal Methods
 ****************************************************************************************/

NetSignal* Circuit::getNetSignalByUuid(const QUuid& uuid) const noexcept
{
    return mNetSignals.value(uuid, 0);
}

NetSignal* Circuit::getNetSignalByName(const QString& name) const noexcept
{
    foreach (NetSignal* netsignal, mNetSignals)
    {
        if (netsignal->getName() == name)
            return netsignal;
    }
    return 0;
}

NetSignal* Circuit::createNetSignal(const QUuid& netclass) throw (Exception)
{
    unsigned int i = 1;
    QString name;
    do
    {
        name = QString("N#%1").arg(i++); // find a new unique signal name
    } while (getNetSignalByName(name));

    return NetSignal::create(*this, mXmlFile->getDocument(), netclass, name, true);
}

void Circuit::addNetSignal(NetSignal* netsignal, bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(netsignal);

    // check if there is no netsignal with the same uuid in the list
    if (getNetSignalByUuid(netsignal->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal->getUuid().toString(),
            QString(tr("There is already a netsignal with the UUID \"%1\"!"))
            .arg(netsignal->getUuid().toString()));
    }

    // check if there is no netsignal with the same name in the list
    if (getNetSignalByName(netsignal->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal->getUuid().toString(),
            QString(tr("There is already a netsignal with the name \"%1\"!"))
            .arg(netsignal->getName()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netsignals");
    netsignal->addToCircuit(toDomTree, parent); // can throw an exception

    mNetSignals.insert(netsignal->getUuid(), netsignal);
}

void Circuit::removeNetSignal(NetSignal* netsignal, bool fromDomTree, bool deleteNetSignal) throw (Exception)
{
    Q_CHECK_PTR(netsignal);
    Q_ASSERT(mNetSignals.contains(netsignal->getUuid()));

    // the netsignal cannot be removed if there are already elements with that netsignal!
    if ((netsignal->getGenCompSignalCount() > 0) || (netsignal->getNetPointCount() > 0))
    {
        throw LogicError(__FILE__, __LINE__, QString("%1:%2/%3")
            .arg(netsignal->getUuid().toString()).arg(netsignal->getGenCompSignalCount())
            .arg(netsignal->getNetPointCount()),
            QString(tr("There are already elements in the netsignal \"%1\"!"))
            .arg(netsignal->getName()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netsignals");
    netsignal->removeFromCircuit(fromDomTree, parent); // can throw an exception

    mNetSignals.remove(netsignal->getUuid());

    if (deleteNetSignal)
        delete netsignal;
}

/*****************************************************************************************
 *  GenericComponentInstance Methods
 ****************************************************************************************/

GenericComponentInstance* Circuit::getGenCompInstanceByUuid(const QUuid& uuid) const noexcept
{
    return mGenCompInstances.value(uuid, 0);
}

GenericComponentInstance* Circuit::getGenCompInstanceByName(const QString& name) const noexcept
{
    foreach (GenericComponentInstance* genCompInstance, mGenCompInstances)
    {
        if (genCompInstance->getName() == name)
            return genCompInstance;
    }
    return 0;
}

GenericComponentInstance* Circuit::createGenCompInstance(const QUuid& genComp,
                                                         const QString& name)
                                                         throw (Exception)
{
    return GenericComponentInstance::create(*this, mXmlFile->getDocument(), genComp, name);
}

void Circuit::addGenCompInstance(GenericComponentInstance* genCompInstance,
                                 bool toDomTree) throw (Exception)
{
    Q_CHECK_PTR(genCompInstance);

    // check if there is no generic component with the same uuid in the list
    if (getGenCompInstanceByUuid(genCompInstance->getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, genCompInstance->getUuid().toString(),
            QString(tr("There is already a component with the UUID \"%1\"!"))
            .arg(genCompInstance->getUuid().toString()));
    }

    // check if there is no generic component with the same name in the list
    if (getGenCompInstanceByName(genCompInstance->getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, genCompInstance->getUuid().toString(),
            QString(tr("There is already a component with the name \"%1\"!"))
            .arg(genCompInstance->getName()));
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("generic_component_instances");
    genCompInstance->addToCircuit(toDomTree, parent); // can throw an exception

    mGenCompInstances.insert(genCompInstance->getUuid(), genCompInstance);
}

void Circuit::removeGenCompInstance(GenericComponentInstance* genCompInstance,
                                    bool fromDomTree, bool deleteGenCompInstance)
                                    throw (Exception)
{
    Q_CHECK_PTR(genCompInstance);
    Q_ASSERT(mGenCompInstances.contains(genCompInstance->getUuid()));

    // TODO: check if there are still objects like symbols or footprints which use this
    // generic component

    QDomElement parent = mXmlFile->getRoot().firstChildElement("generic_component_instances");
    genCompInstance->removeFromCircuit(fromDomTree, parent); // can throw an exception

    mGenCompInstances.remove(genCompInstance->getUuid());

    if (deleteGenCompInstance)
        delete genCompInstance;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool Circuit::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    // Save "core/circuit.xml"
    try
    {
        mXmlFile->save(toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

Circuit* Circuit::create(Workspace& workspace, Project& project) throw (Exception)
{
    XmlFile* file = 0;
    Circuit* circuit = 0;

    try
    {
        file = XmlFile::create(project.getPath().getPathTo("core/circuit.xml"), "circuit", 0);
        circuit = new Circuit(workspace, project, true, true);
        delete file;
    }
    catch (Exception& e)
    {
        delete circuit;
        delete file;
        throw;
    }

    return circuit;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
