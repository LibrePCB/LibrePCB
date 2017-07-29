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

#ifndef LIBREPCB_PROJECT_ERCMSGLIST_H
#define LIBREPCB_PROJECT_ERCMSGLIST_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class SmartXmlFile;

namespace project {

class Project;
class ErcMsg;

/*****************************************************************************************
 *  Class ErcMsgList
 ****************************************************************************************/

/**
 * @brief The ErcMsgList class contains a list of ERC messages which are visible for the user
 */
class ErcMsgList final : public QObject, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        ErcMsgList() = delete;
        ErcMsgList(const ErcMsgList& other) = delete;
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
        
        // Operator Overloadings
        ErcMsgList& operator=(const ErcMsgList& rhs) = delete;


    signals:

        void ercMsgAdded(ErcMsg* ercMsg);
        void ercMsgRemoved(ErcMsg* ercMsg);
        void ercMsgChanged(ErcMsg* ercMsg);


    private: // Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;


        // General
        Project& mProject;

        // File "core/erc.xml"
        FilePath mXmlFilepath;
        QScopedPointer<SmartXmlFile> mXmlFile;

        // Misc
        QList<ErcMsg*> mItems; ///< contains all visible ERC messages
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_ERCMSGLIST_H
