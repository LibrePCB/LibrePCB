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

#ifndef PROJECT_ERCMSGLIST_H
#define PROJECT_ERCMSGLIST_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/file_io/if_xmlserializableobject.h"
#include "../../common/exceptions.h"
#include "../../common/filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SmartXmlFile;

namespace project {
class Project;
class ErcMsg;
}

/*****************************************************************************************
 *  Class ErcMsgList
 ****************************************************************************************/

namespace project {

/**
 * @brief The ErcMsgList class contains a list of ERC messages which are visible for the user
 */
class ErcMsgList final : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ErcMsgList(Project& project, bool restore, bool readOnly, bool create) throw (Exception);
        ~ErcMsgList() noexcept;

        // Getters
        const QList<ErcMsg*>& getItems() const noexcept {return mItems;}

        // General Methods
        void add(ErcMsg* ercMsg) noexcept;
        void remove(ErcMsg* ercMsg) noexcept;
        void update(ErcMsg* ercMsg) noexcept;
        void restoreIgnoreState() noexcept;
        bool save(bool toOriginal, QStringList& errors) noexcept;

    signals:

        void ercMsgAdded(ErcMsg* ercMsg);
        void ercMsgRemoved(ErcMsg* ercMsg);
        void ercMsgChanged(ErcMsg* ercMsg);


    private:

        // make some methods inaccessible...
        ErcMsgList();
        ErcMsgList(const ErcMsgList& other);
        ErcMsgList& operator=(const ErcMsgList& rhs);

        // Private Methods

        /**
         * @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
         */
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


        // General
        Project& mProject;

        // File "core/erc.xml"
        FilePath mXmlFilepath;
        SmartXmlFile* mXmlFile;

        // Misc
        QList<ErcMsg*> mItems; ///< contains all visible ERC messages
};

} // namespace project

#endif // PROJECT_ERCMSGLIST_H
