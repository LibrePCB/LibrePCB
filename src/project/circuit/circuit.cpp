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

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Circuit::Circuit(Workspace& workspace, Project& project, bool restore) throw (Exception) :
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
        tmpNode = root.firstChildElement("netclasses").firstChildElement("netclass");
        while (!tmpNode.isNull())
        {
            NetClass* netclass = new NetClass(tmpNode);
            addNetClass(netclass, false);
            tmpNode = tmpNode.nextSiblingElement("netclass");
        }
        qDebug() << mNetClasses.count() << "netclasses successfully loaded!";

        // Load all netsignals
        tmpNode = root.firstChildElement("netsignals").firstChildElement("netsignal");
        while (!tmpNode.isNull())
        {
            NetSignal* netsignal = new NetSignal(*this, tmpNode);
            addNetSignal(netsignal, false);
            tmpNode = tmpNode.nextSiblingElement("netsignal");
        }
        qDebug() << mNetSignals.count() << "netsignals successfully loaded!";
    }
    catch (...)
    {
        // free allocated memory and rethrow the exception
        qDeleteAll(mNetSignals);    mNetSignals.clear();
        qDeleteAll(mNetClasses);    mNetClasses.clear();
        delete mXmlFile;            mXmlFile = 0;
        throw;
    }

    qDebug() << "circuit successfully loaded!";
}

Circuit::~Circuit() noexcept
{
    qDeleteAll(mNetSignals);    mNetSignals.clear();
    qDeleteAll(mNetClasses);    mNetClasses.clear();
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

    if (toDomTree)
    {
        QDomElement parent = mXmlFile->getRoot().firstChildElement("netclasses");
        netclass->addToDomTree(parent);
    }

    mNetClasses.insert(netclass->getUuid(), netclass);
}

void Circuit::removeNetClass(NetClass* netclass) throw (Exception)
{
    Q_CHECK_PTR(netclass);
    Q_ASSERT(mNetClasses.contains(netclass->getUuid()));

    // the netclass cannot be removed if there are already netsignals with that netclass!
    foreach (NetSignal* netsignal, mNetSignals)
    {
        if (netsignal->getNetClass() == netclass)
        {
            throw RuntimeError(__FILE__, __LINE__, QString("%1:%2")
                .arg(netclass->getUuid().toString(), netsignal->getUuid().toString()),
                QString(tr("There are already signals in the netclass \"%1\"!"))
                .arg(netclass->getName()));
        }
    }

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netclasses");
    netclass->removeFromDomTree(parent);
    mNetClasses.remove(netclass->getUuid());
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

    if (toDomTree)
    {
        QDomElement parent = mXmlFile->getRoot().firstChildElement("netsignals");
        netsignal->addToDomTree(parent);
    }

    mNetSignals.insert(netsignal->getUuid(), netsignal);
}

void Circuit::removeNetSignal(NetSignal* netsignal) throw (Exception)
{
    Q_CHECK_PTR(netsignal);
    Q_ASSERT(mNetSignals.contains(netsignal->getUuid()));

    // TODO: check if there are component signals connected with that signal

    QDomElement parent = mXmlFile->getRoot().firstChildElement("netsignals");
    netsignal->removeFromDomTree(parent);
    mNetSignals.remove(netsignal->getUuid());
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
 *  End of File
 ****************************************************************************************/

} // namespace project
