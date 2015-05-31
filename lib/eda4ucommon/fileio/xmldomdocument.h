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

#ifndef XMLDOMDOCUMENT_H
#define XMLDOMDOCUMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomDocument>
#include "../exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

/*****************************************************************************************
 *  Class XmlDomDocument
 ****************************************************************************************/

/**
 * @brief The XmlDomDocument class represents a XML DOM document with the whole DOM tree
 *
 * @todo Use libxml2 instead of Qt's DOM classes
 * @todo Use XSD schema files to validate the opened XML file (with libxml2)
 * @todo Save the DOM document as canonical XML to file (with libxml2 c14n)
 *
 * @author ubruhin
 * @date 2015-02-01
 */
class XmlDomDocument final
{
        Q_DECLARE_TR_FUNCTIONS(XmlDomDocument)

    public:

        // Constructors / Destructor

        /**
         * @brief Constructor to create a new DOM document with a root element
         *
         * @param root              The root element which will be added to the document.
         *                          The document will take the ownership over the root
         *                          element object!
         * @param setAppVersion     If true, this constructor will automatically add the
         *                          applications major version number to the root element
         *                          as attribute "version". You can also use
         *                          #setFileVersion() instead.
         */
        explicit XmlDomDocument(XmlDomElement& root, bool setAppVersion) noexcept;

        /**
         * @brief Constructor to create the whole DOM tree from the content of a XML file
         *
         * @param xmlFileContent    The content of the XML file to load
         * @param filepath          The filepath of the XML file (needed for the exceptions)
         *
         * @throw Exception         If parsing the XML file has failed.
         */
        explicit XmlDomDocument(const QByteArray& xmlFileContent, const FilePath& filepath) throw (Exception);

        /**
         * @brief Destructor (destroys the whole DOM tree)
         */
        ~XmlDomDocument() noexcept;


        // Getters

        /**
         * @brief Get the filepath which was passed to the constructor
         *
         * @return The filepath
         */
        const FilePath& getFilePath() const noexcept {return mFilePath;}

        /**
         * @brief Get the root XML DOM element
         *
         * @return The root element of the document
         */
        XmlDomElement& getRoot() const noexcept {Q_ASSERT(mRootElement); return *mRootElement;}

        /**
         * @brief Get the file version number of the document (attribute "version" of the
         *        root element)
         *
         * @return  The file version number
         *
         * @throw   If the attribute "version" of the root node is invalid or does not exist
         */
        uint getFileVersion() const throw (Exception);


        // Setters

        /**
         * @brief Set the file version number of the document (attribute "version" of the
         *        root element)
         *
         * @param version   The file version
         */
        void setFileVersion(uint version) noexcept;


        // General Methods

        /**
         * @brief Export the whole DOM tree as a QByteArray to write back to the XML file
         *
         * @return The XML DOM tree which can be written into an XML file
         */
        QByteArray toByteArray() const noexcept;


    private:

        // make some methods inaccessible...
        XmlDomDocument();
        XmlDomDocument(const XmlDomDocument& other);
        XmlDomDocument& operator=(const XmlDomDocument& rhs);


        // General
        FilePath mFilePath;             ///< the filepath from the constructor
        XmlDomElement* mRootElement;    ///< the root DOM element
};

#endif // XMLDOMDOCUMENT_H
