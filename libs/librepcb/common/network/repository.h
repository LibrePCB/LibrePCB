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

#ifndef LIBREPCB_REPOSITORY_H
#define LIBREPCB_REPOSITORY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobject.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Repository
 ****************************************************************************************/

/**
 * @brief The Repository class provides access to a LibrePCB API server
 *
 * @author ubruhin
 * @date 2016-08-10
 */
class Repository final : public QObject, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Repository() = delete;
        explicit Repository(const Repository& other) noexcept;
        explicit Repository(const QUrl& url) noexcept;
        explicit Repository(const DomElement& domElement);
        ~Repository() noexcept;

        // Getters
        bool isValid() const noexcept {return mUrl.isValid();}
        const QUrl& getUrl() const noexcept {return mUrl;}

        // Setters
        bool setUrl(const QUrl& url) noexcept;

        // General Methods
        void requestLibraryList() const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const override;

        // Operators
        Repository& operator=(const Repository& rhs) = delete;


    signals:

        void libraryListReceived(const QJsonArray& libs);
        void errorWhileFetchingLibraryList(const QString& errorMsg);


    private: // Methods

        void requestLibraryList(const QUrl& url) const noexcept;
        void requestedDataReceived(const QByteArray& data) noexcept;
        bool checkAttributesValidity() const noexcept;


    private: // Data

        QUrl mUrl;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_REPOSITORY_H
