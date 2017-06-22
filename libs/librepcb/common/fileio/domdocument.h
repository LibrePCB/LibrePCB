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

#ifndef LIBREPCB_DOMDOCUMENT_H
#define LIBREPCB_DOMDOCUMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QDomDocument>
#include "../exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class DomElement;

/*****************************************************************************************
 *  Class DomDocument
 ****************************************************************************************/

/**
 * @brief The DomDocument class represents a DOM document with the whole DOM tree
 *
 * @author ubruhin
 * @date 2015-02-01
 */
class DomDocument final
{
        Q_DECLARE_TR_FUNCTIONS(DomDocument)

    public:

        // Constructors / Destructor
        DomDocument() = delete;
        DomDocument(const DomDocument& other) = delete;

        /**
         * @brief Constructor to create a new DOM document with a root element
         *
         * @param root              The root element which will be added to the document.
         *                          The document will take the ownership over the root
         *                          element object!
         */
        explicit DomDocument(DomElement& root) noexcept;

        /**
         * @brief Constructor to create the whole DOM tree from the content of a file
         *
         * @param fileContent       The content of the file to load
         * @param filepath          The filepath of the file (needed for the exceptions)
         *
         * @throw Exception         If parsing the file has failed.
         */
        explicit DomDocument(const QByteArray& fileContent, const FilePath& filepath) throw (Exception);

        /**
         * @brief Destructor (destroys the whole DOM tree)
         */
        ~DomDocument() noexcept;


        // Getters

        /**
         * @brief Get the filepath which was passed to the constructor
         *
         * @return The filepath
         */
        const FilePath& getFilePath() const noexcept {return mFilePath;}

        /**
         * @brief Get the root DOM element
         *
         * @return The root element of the document
         */
        DomElement& getRoot() const noexcept {Q_ASSERT(mRootElement); return *mRootElement;}

        /**
         * @brief Get the root DOM element and check it's tag name
         *
         * @param expectedName  The expected name of the root element. If it differs,
         *                      an exception will be thrown.
         *
         * @return The root element of the document
         */
        DomElement& getRoot(const QString& expectedName) const throw (Exception);


        // General Methods

        /**
         * @brief Export the whole DOM tree as a QByteArray to write back to the file
         *
         * @return The DOM tree which can be written into a file
         */
        QByteArray toByteArray() const throw (Exception);


        // Operator Overloadings
        DomDocument& operator=(const DomDocument& rhs) = delete;


    private:

        // General
        FilePath mFilePath;                         ///< the filepath from the constructor
        QScopedPointer<DomElement> mRootElement; ///< the root DOM element
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_DOMDOCUMENT_H
