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

#ifndef PROJECT_NETCLASS_H
#define PROJECT_NETCLASS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomElement>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace project {
class Project;
class Circuit;
}

/*****************************************************************************************
 *  Class NetClass
 ****************************************************************************************/

namespace project {

/**
 * @brief The NetClass class
 */
class NetClass final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit NetClass(Workspace& workspace, Project& project, Circuit& circuit,
                          const QDomElement& domElement);
        ~NetClass();

        // Getters
        const QUuid& getUuid() const {return mUuid;}
        const QString& getName() const {return mName;}

        // Static Methods
        static void loadFromCircuit(Workspace& workspace, Project& project,
                                    Circuit& circuit, const QDomElement& node,
                                    QHash<QUuid, NetClass*>& list);

    private:

        // make some methods inaccessible...
        NetClass();
        NetClass(const NetClass& other);
        NetClass& operator=(const NetClass& rhs);

        // General
        Workspace& mWorkspace;
        Project& mProject;
        Circuit& mCircuit;
        QDomElement mDomElement;

        // Attributes
        QUuid mUuid;
        QString mName;

};

} // namespace project

#endif // PROJECT_NETCLASS_H
