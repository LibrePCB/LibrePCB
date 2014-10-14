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

#ifndef PROJECT_CIRCUIT_H
#define PROJECT_CIRCUIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/exceptions.h"
#include "../../common/filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlFile;

namespace project {
class Project;
class NetClass;
class NetSignal;
class GenericComponentInstance;
}

/*****************************************************************************************
 *  Class Circuit
 ****************************************************************************************/

namespace project {

/**
 * @brief The Circuit class
 *
 * @author ubruhin
 * @date 2014-07-03
 */
class Circuit final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Circuit(Project& project, bool restore, bool isNew = false) throw (Exception);
        ~Circuit() noexcept;

        // Getters
        Project& getProject() const noexcept {return mProject;}

        // NetClass Methods
        const QHash<QUuid, NetClass*>& getNetClasses() const noexcept {return mNetClasses;}
        NetClass* getNetClassByUuid(const QUuid& uuid) const noexcept;
        NetClass* getNetClassByName(const QString& name) const noexcept;
        NetClass* createNetClass(const QString& name) throw (Exception);
        void addNetClass(NetClass* netclass, bool toDomTree = true) throw (Exception);
        void removeNetClass(NetClass* netclass, bool fromDomTree = true,
                            bool deleteNetClass = false) throw (Exception);
        void execEditNetClassesDialog(QWidget* parent = 0) noexcept;

        // NetSignal Methods
        NetSignal* getNetSignalByUuid(const QUuid& uuid) const noexcept;
        NetSignal* getNetSignalByName(const QString& name) const noexcept;
        NetSignal* createNetSignal(const QUuid& netclass) throw (Exception);
        void addNetSignal(NetSignal* netsignal, bool toDomTree = true) throw (Exception);
        void removeNetSignal(NetSignal* netsignal, bool fromDomTree = true,
                             bool deleteNetSignal = false) throw (Exception);

        // GenericComponentInstance Methods
        GenericComponentInstance* getGenCompInstanceByUuid(const QUuid& uuid) const noexcept;
        GenericComponentInstance* getGenCompInstanceByName(const QString& name) const noexcept;
        GenericComponentInstance* createGenCompInstance(const QUuid& genComp,
                                                        const QString& name) throw (Exception);
        void addGenCompInstance(GenericComponentInstance* genCompInstance, bool toDomTree = true) throw (Exception);
        void removeGenCompInstance(GenericComponentInstance* genCompInstance, bool fromDomTree = true,
                             bool deleteGenCompInstance = false) throw (Exception);

        // General Methods
        bool save(bool toOriginal, QStringList& errors) noexcept;

        // Static Methods
        static Circuit* create(Project& project) throw (Exception);


    private:

        // make some methods inaccessible...
        Circuit();
        Circuit(const Circuit& other);
        Circuit& operator=(const Circuit& rhs);

        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)

        // File "core/circuit.xml"
        FilePath mXmlFilepath;
        XmlFile* mXmlFile;

        QHash<QUuid, NetClass*> mNetClasses;
        QHash<QUuid, NetSignal*> mNetSignals;
        QHash<QUuid, GenericComponentInstance*> mGenCompInstances;
};

} // namespace project

#endif // PROJECT_CIRCUIT_H
