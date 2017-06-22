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

#ifndef LIBREPCB_LIBRARY_LIBRARY_H
#define LIBREPCB_LIBRARY_LIBRARY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include "librarybaseelement.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Library
 ****************************************************************************************/

/**
 * @brief   The Library class represents a library directory
 *
 * @author  ubruhin
 * @date    2016-08-03
 */
class Library final : public LibraryBaseElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Library() = delete;
        Library(const Library& other) = delete;
        Library(const Uuid& uuid, const Version& version, const QString& author,
                const QString& name_en_US, const QString& description_en_US,
                const QString& keywords_en_US) throw (Exception);
        Library(const FilePath& libDir, bool readOnly) throw (Exception);
        ~Library() noexcept;

        // Getters
        const QUrl& getUrl() const noexcept {return mUrl;}
        const QList<Uuid>& getDependencies() const noexcept {return mDependencies;}
        const QPixmap& getIcon() const noexcept {return mIcon;}

        // Setters
        void setUrl(const QUrl& url) noexcept {mUrl = url;}

        // General Methods
        void addDependency(const Uuid& uuid) noexcept;
        void removeDependency(const Uuid& uuid) noexcept;
        template <typename ElementType>
        QList<FilePath> searchForElements() const noexcept;

        // Operator Overloadings
        Library& operator=(const Library& rhs) = delete;

        // Static Methods
        static QString getShortElementName() noexcept {return QStringLiteral("lib");}
        static QString getLongElementName() noexcept {return QStringLiteral("library");}


    private: // Methods

        // Private Methods
        virtual void copyTo(const FilePath& destination, bool removeSource) throw (Exception) override;
        /// @copydoc librepcb::SerializableObject::serialize()
        virtual void serialize(DomElement& root) const throw (Exception) override;
        virtual bool checkAttributesValidity() const noexcept override;


    private: // Data

        QUrl mUrl;
        QList<Uuid> mDependencies;
        QPixmap mIcon;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_LIBRARY_H
