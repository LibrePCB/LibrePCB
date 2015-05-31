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
#include <eda4ucommon/exceptions.h>
#include <eda4ucommon/fileio/smartxmlfile.h>
#include <eda4ucommon/fileio/xmldomdocument.h>
#include <eda4ucommon/fileio/xmldomelement.h>
#include "circuit.h"
#include "../project.h"
#include "netclass.h"
#include "netsignal.h"
#include "gencompinstance.h"
#include "editnetclassesdialog.h"
#include <eda4ulibrary/gencmp/genericcomponent.h>
#include "../settings/projectsettings.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Project& project, bool restore, bool readOnly, bool create) throw (Exception) :
    QObject(0), mProject(project),
    mXmlFilepath(project.getPath().getPathTo("core/circuit.xml")), mXmlFile(nullptr)
{
    qDebug() << "load circuit...";
    Q_ASSERT(!(create && (restore || readOnly)));

    try
    {
        // try to create/open the XML file "circuit.xml"
        if (create)
        {
            mXmlFile = SmartXmlFile::create(mXmlFilepath);
            NetClass* netclass = new NetClass(*this, "default");
            addNetClass(*netclass); // add a netclass with name "default"
        }
        else
        {
            mXmlFile = new SmartXmlFile(mXmlFilepath, restore, readOnly);
            QSharedPointer<XmlDomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
            XmlDomElement& root = doc->getRoot();

            // OK - XML file is open --> now load the whole circuit stuff

            // Load all netclasses
            for (XmlDomElement* node = root.getFirstChild("netclasses/netclass", true, false);
                 node; node = node->getNextSibling("netclass"))
            {
                NetClass* netclass = new NetClass(*this, *node);
                addNetClass(*netclass);
            }

            // Load all netsignals
            for (XmlDomElement* node = root.getFirstChild("netsignals/netsignal", true, false);
                 node; node = node->getNextSibling("netsignal"))
            {
                NetSignal* netsignal = new NetSignal(*this, *node);
                addNetSignal(*netsignal);
            }

            // Load all generic component instances
            for (XmlDomElement* node = root.getFirstChild("generic_component_instances/generic_component_instance", true, false);
                 node; node = node->getNextSibling("generic_component_instance"))
            {
                GenCompInstance* genComp = new GenCompInstance(*this, *node);
                addGenCompInstance(*genComp);
            }
        }

        if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);
    }
    catch (...)
    {
        // free allocated memory (see comments in the destructor) and rethrow the exception
        foreach (GenCompInstance* genCompInstance, mGenCompInstances)
            try { removeGenCompInstance(*genCompInstance); delete genCompInstance; } catch (...) {}
        foreach (NetSignal* netsignal, mNetSignals)
            try { removeNetSignal(*netsignal); delete netsignal; } catch (...) {}
        foreach (NetClass* netclass, mNetClasses)
            try { removeNetClass(*netclass); delete netclass; } catch (...) {}
        delete mXmlFile;            mXmlFile = nullptr;
        throw;
    }

    qDebug() << "circuit successfully loaded!";
}

Circuit::~Circuit() noexcept
{
    // delete all generic component instances (and catch all throwed exceptions)
    foreach (GenCompInstance* genCompInstance, mGenCompInstances)
        try { removeGenCompInstance(*genCompInstance); delete genCompInstance; } catch (...) {}

    // delete all netsignals (and catch all throwed exceptions)
    foreach (NetSignal* netsignal, mNetSignals)
        try { removeNetSignal(*netsignal); delete netsignal; } catch (...) {}

    // delete all netclasses (and catch all throwed exceptions)
    foreach (NetClass* netclass, mNetClasses)
        try { removeNetClass(*netclass); delete netclass; } catch (...) {}

    delete mXmlFile;            mXmlFile = nullptr;
}

/*****************************************************************************************
 *  NetClass Methods
 ****************************************************************************************/

NetClass* Circuit::getNetClassByUuid(const QUuid& uuid) const noexcept
{
    return mNetClasses.value(uuid, nullptr);
}

NetClass* Circuit::getNetClassByName(const QString& name) const noexcept
{
    foreach (NetClass* netclass, mNetClasses)
    {
        if (netclass->getName() == name)
            return netclass;
    }
    return nullptr;
}

void Circuit::addNetClass(NetClass& netclass) throw (Exception)
{
    // check if there is no netclass with the same uuid in the list
    if (getNetClassByUuid(netclass.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netclass.getUuid().toString(),
            QString(tr("There is already a netclass with the UUID \"%1\"!"))
            .arg(netclass.getUuid().toString()));
    }

    // check if there is no netclass with the same name in the list
    if (getNetClassByName(netclass.getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, netclass.getUuid().toString(),
            QString(tr("There is already a netclass with the name \"%1\"!"))
            .arg(netclass.getName()));
    }

    // add netclass to circuit
    netclass.addToCircuit();
    mNetClasses.insert(netclass.getUuid(), &netclass);
    emit netClassAdded(netclass);
}

void Circuit::removeNetClass(NetClass& netclass) throw (Exception)
{
    Q_ASSERT(mNetClasses.contains(netclass.getUuid()) == true);

    // the netclass cannot be removed if there are already netsignals with that netclass!
    if (netclass.getNetSignalCount() > 0)
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
            .arg(netclass.getUuid().toString()).arg(netclass.getNetSignalCount()),
            QString(tr("There are already signals in the netclass \"%1\"!"))
            .arg(netclass.getName()));
    }

    // remove netclass from project
    netclass.removeFromCircuit();
    mNetClasses.remove(netclass.getUuid());
    emit netClassRemoved(netclass);
}

void Circuit::setNetClassName(NetClass& netclass, const QString& newName) throw (Exception)
{
    Q_ASSERT(mNetClasses.contains(netclass.getUuid()) == true);
    if (newName == netclass.getName()) return;

    // check the validity of the new name
    if (newName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, netclass.getUuid().toString(),
            QString(tr("The new netclass name must not be empty!")));
    }

    // check if there is no netclass with the same name in the list
    if (getNetClassByName(newName))
    {
        throw RuntimeError(__FILE__, __LINE__, netclass.getUuid().toString(),
            QString(tr("There is already a netclass with the name \"%1\"!")).arg(newName));
    }

    // apply the new name
    netclass.setName(newName);
}

void Circuit::execEditNetClassesDialog(QWidget* parent) noexcept
{
    try
    {
        EditNetClassesDialog dialog(*this, parent);
        dialog.exec();
    }
    catch (Exception& e)
    {
        QMessageBox::critical(parent, tr("Error"), e.getUserMsg());
    }
}

/*****************************************************************************************
 *  NetSignal Methods
 ****************************************************************************************/

NetSignal* Circuit::getNetSignalByUuid(const QUuid& uuid) const noexcept
{
    return mNetSignals.value(uuid, nullptr);
}

NetSignal* Circuit::getNetSignalByName(const QString& name) const noexcept
{
    foreach (NetSignal* netsignal, mNetSignals)
    {
        if (netsignal->getName() == name)
            return netsignal;
    }
    return nullptr;
}

NetSignal* Circuit::createNetSignal(NetClass& netclass, QString name) throw (Exception)
{
    bool autoName = false;
    if (name.isEmpty())
    {
        unsigned int i = 1;
        do
        {
            name = QString("N#%1").arg(i++); // find a new unique signal name
        } while (getNetSignalByName(name));
        autoName = true;
    }
    else
    {
        if (getNetSignalByName(name))
        {
            throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The net signal "
                "name \"%1\" does already exist in the circuit.")).arg(name));
        }
    }
    return new NetSignal(*this, netclass, name, autoName);
}

void Circuit::addNetSignal(NetSignal& netsignal) throw (Exception)
{
    // check if there is no netsignal with the same uuid in the list
    if (getNetSignalByUuid(netsignal.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal.getUuid().toString(),
            QString(tr("There is already a netsignal with the UUID \"%1\"!"))
            .arg(netsignal.getUuid().toString()));
    }

    // check if there is no netsignal with the same name in the list
    if (getNetSignalByName(netsignal.getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal.getUuid().toString(),
            QString(tr("There is already a netsignal with the name \"%1\"!"))
            .arg(netsignal.getName()));
    }

    // add netsignal to circuit
    netsignal.addToCircuit();
    mNetSignals.insert(netsignal.getUuid(), &netsignal);
    emit netSignalAdded(netsignal);
}

void Circuit::removeNetSignal(NetSignal& netsignal) throw (Exception)
{
    Q_ASSERT(mNetSignals.contains(netsignal.getUuid()) == true);

    // the netsignal cannot be removed if there are already elements with that netsignal!
    if (   (netsignal.getGenCompSignals().count() > 0)
        || (netsignal.getNetPoints().count() > 0)
        || (netsignal.getNetLabels().count() > 0))
    {
        throw LogicError(__FILE__, __LINE__,
            QString("%1:%2/%3")
            .arg(netsignal.getUuid().toString())
            .arg(netsignal.getGenCompSignals().count())
            .arg(netsignal.getNetPoints().count()),
            QString(tr("There are already elements in the netsignal \"%1\"!"))
            .arg(netsignal.getName()));
    }

    // remove netsignal from circuit
    netsignal.removeFromCircuit();
    mNetSignals.remove(netsignal.getUuid());
    emit netSignalRemoved(netsignal);
}

void Circuit::setNetSignalName(NetSignal& netsignal, const QString& newName, bool isAutoName) throw (Exception)
{
    Q_ASSERT(mNetSignals.contains(netsignal.getUuid()) == true);
    if ((newName == netsignal.getName()) && (isAutoName == netsignal.hasAutoName())) return;

    // check the validity of the new name
    if (newName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal.getUuid().toString(),
            QString(tr("The new net signal name must not be empty!")));
    }

    // check if there is no net signal with the same name in the list
    if (getNetSignalByName(newName))
    {
        throw RuntimeError(__FILE__, __LINE__, netsignal.getUuid().toString(),
            QString(tr("There is already a net signal with the name \"%1\"!")).arg(newName));
    }

    // apply the new name
    netsignal.setName(newName, isAutoName);
}

/*****************************************************************************************
 *  GenCompInstance Methods
 ****************************************************************************************/

GenCompInstance* Circuit::getGenCompInstanceByUuid(const QUuid& uuid) const noexcept
{
    return mGenCompInstances.value(uuid, nullptr);
}

GenCompInstance* Circuit::getGenCompInstanceByName(const QString& name) const noexcept
{
    foreach (GenCompInstance* genCompInstance, mGenCompInstances)
    {
        if (genCompInstance->getName() == name)
            return genCompInstance;
    }
    return nullptr;
}

GenCompInstance* Circuit::createGenCompInstance(const library::GenericComponent& genComp,
                                                const library::GenCompSymbVar& symbVar,
                                                QString name) throw (Exception)
{
    if (name.isEmpty())
    {
        QString prefix = genComp.getPrefix(mProject.getSettings().getLocaleOrder(true));
        if (prefix.isEmpty()) prefix = "?";
        unsigned int i = 1;
        do
        {
            name = QString("%1%2").arg(prefix).arg(i++); // find a new unique component name
        } while (getGenCompInstanceByName(name));
    }
    else
    {
        if (getGenCompInstanceByName(name))
        {
            throw RuntimeError(__FILE__, __LINE__, name, QString(tr("The component "
                "name \"%1\" does already exist in the circuit.")).arg(name));
        }
    }
    return new GenCompInstance(*this, genComp, symbVar, name);
}

void Circuit::addGenCompInstance(GenCompInstance& genCompInstance) throw (Exception)
{
    // check if there is no generic component with the same uuid in the list
    if (getGenCompInstanceByUuid(genCompInstance.getUuid()))
    {
        throw RuntimeError(__FILE__, __LINE__, genCompInstance.getUuid().toString(),
            QString(tr("There is already a component with the UUID \"%1\"!"))
            .arg(genCompInstance.getUuid().toString()));
    }

    // check if there is no generic component with the same name in the list
    if (getGenCompInstanceByName(genCompInstance.getName()))
    {
        throw RuntimeError(__FILE__, __LINE__, genCompInstance.getUuid().toString(),
            QString(tr("There is already a component with the name \"%1\"!"))
            .arg(genCompInstance.getName()));
    }

    // add to circuit
    genCompInstance.addToCircuit();
    mGenCompInstances.insert(genCompInstance.getUuid(), &genCompInstance);
    emit genCompAdded(genCompInstance);
}

void Circuit::removeGenCompInstance(GenCompInstance& genCompInstance) throw (Exception)
{
    Q_ASSERT(mGenCompInstances.contains(genCompInstance.getUuid()) == true);

    // check if the generic component instance is not used by symbols/footprints
    if (genCompInstance.getPlacedSymbolsCount() > 0)
    {
        throw LogicError(__FILE__, __LINE__, genCompInstance.getUuid().toString(),
            QString(tr("The component \"%1\" is still used!")).arg(genCompInstance.getName()));
    }

    // remove from circuit
    genCompInstance.removeFromCircuit();
    mGenCompInstances.remove(genCompInstance.getUuid());
    emit genCompRemoved(genCompInstance);
}

void Circuit::setGenCompInstanceName(GenCompInstance& genComp, const QString& newName) throw (Exception)
{
    Q_ASSERT(mGenCompInstances.contains(genComp.getUuid()) == true);
    if (newName == genComp.getName()) return;

    // check the validity of the new name
    if (newName.isEmpty())
    {
        throw RuntimeError(__FILE__, __LINE__, genComp.getUuid().toString(),
            QString(tr("The new generic component name must not be empty!")));
    }

    // check if there is no generic component with the same name in the list
    if (getGenCompInstanceByName(newName))
    {
        throw RuntimeError(__FILE__, __LINE__, genComp.getUuid().toString(),
            QString(tr("There is already a component with the name \"%1\"!")).arg(newName));
    }

    // apply the new name
    genComp.setName(newName);
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
        XmlDomElement* root = serializeToXmlDomElement();
        XmlDomDocument doc(*root, true);
        mXmlFile->save(doc, toOriginal);
    }
    catch (Exception& e)
    {
        success = false;
        errors.append(e.getUserMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool Circuit::checkAttributesValidity() const noexcept
{
    return true;
}

XmlDomElement* Circuit::serializeToXmlDomElement() const throw (Exception)
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    QScopedPointer<XmlDomElement> root(new XmlDomElement("circuit"));
    //XmlDomElement* meta = root->appendChild("meta");
    XmlDomElement* netclasses = root->appendChild("netclasses");
    foreach (NetClass* netclass, mNetClasses)
        netclasses->appendChild(netclass->serializeToXmlDomElement());
    XmlDomElement* netsignals = root->appendChild("netsignals");
    foreach (NetSignal* netsignal, mNetSignals)
        netsignals->appendChild(netsignal->serializeToXmlDomElement());
    XmlDomElement* genericComponents = root->appendChild("generic_component_instances");
    foreach (GenCompInstance* instance, mGenCompInstances)
        genericComponents->appendChild(instance->serializeToXmlDomElement());
    return root.take();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
