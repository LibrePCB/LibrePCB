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

#ifndef LIBRARY_LIBRARYBASEELEMENT_H
#define LIBRARY_LIBRARYBASEELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QObject>
#include "common/xmlfile.h"
#include "common/filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

/*****************************************************************************************
 *  Class LibraryBaseElement
 ****************************************************************************************/

namespace library {

/**
 * @brief The LibraryBaseElement class
 */
class LibraryBaseElement : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit LibraryBaseElement(Workspace* workspace, const FilePath& xmlFilePath,
                                    const QString& xmlRootNodeName);
        virtual ~LibraryBaseElement();

        // Getters

        const QUuid& getUuid() const {return mUuid;}
        const QString& getVersion() const {return mVersion;}
        const QString& getAuthor() const {return mAuthor;}
        const QDateTime& getCreated() const {return mCreated;}
        const QDateTime& getLastModified() const {return mLastModified;}
        const QUuid& getCategoryUuid() const {return mCategoryUuid;}
        QString getName(const QString& locale = QString()) const;
        QString getDescription(const QString& locale = QString()) const;
        QString getKeywords(const QString& locale = QString()) const;
        const QHash<QString, QString>& getNames() const {return mNames;}
        const QHash<QString, QString>& getDescriptions() const {return mDescriptions;}
        const QHash<QString, QString>& getKeywords() const {return mKeywords;}

    protected:

        // General

    private:

        // make some methods inaccessible...
        LibraryBaseElement();
        LibraryBaseElement(const LibraryBaseElement& other);
        LibraryBaseElement& operator=(const LibraryBaseElement& rhs);

        Workspace* mWorkspace;
        XmlFile* mXmlFile;

        //General Attributes
        QUuid mUuid;
        QString mVersion;
        QString mAuthor;
        QDateTime mCreated;
        QDateTime mLastModified;
        QUuid mCategoryUuid;
        QHash<QString, QString> mNames;
        QHash<QString, QString> mDescriptions;
        QHash<QString, QString> mKeywords;
};

} // namespace library

#endif // LIBRARY_LIBRARYBASEELEMENT_H
