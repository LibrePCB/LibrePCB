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

#ifndef XMLFILE_H
#define XMLFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomDocument>
#include <QDomElement>
#include "../common/exceptions.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Class XmlFile
 ****************************************************************************************/

/**
 * @brief The XmlFile class represents a XML file and provides access over a QDomElement
 *
 * The constructor will try to open a XML file. Then the whole XML DOM tree is stored in
 * the private attribute #mDomDocument. With #getRoot() you can work with the XML root
 * node (read/write/add/remove nodes). With #save() the whole DOM tree can be written back
 * to the XML file. The destructor will close the file and delete the QDomDocument. With
 * the static method #create() you can create new, empty XML files.
 *
 * @note    This class is also able to load from and save to backup files ("~" at the end
 *          of the filename).
 *
 * @warning Do not use QDomElement objects from #getRoot() after the #XmlFile object is
 *          destroyed!
 *
 * @author ubruhin
 * @date 2014-08-13
 */
class XmlFile final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open a XML file
         *
         * This constructor tries to open an existing XML file and throws an exception
         * if an error occurs.
         *
         * @param filepath  The filepath to the XML file (always to a original file, not
         *                  to a backup file with "~" at the end of the filename!)
         * @param restore   If true and a backup (*~) of the specified XML file exists,
         *                  the backup will be opened instead of the original file. If no
         *                  backup exists or this parameter is false, the original file
         *                  will be opened.
         * @param rootName  If this is not an empty string, the constructor compares the
         *                  XML root node name with this value. If they are not identical,
         *                  the constructor throws an exception. This is useful to check
         *                  if the specified XML file has the expected content.
         *
         * @throw Exception If the specified XML file could not be opened successful, an
         *                  exception will be thrown.
         */
        XmlFile(const FilePath& filepath, bool restore,
                const QString& rootName = QString()) throw (Exception);

        /**
         * @brief The destructor
         */
        ~XmlFile() noexcept;


        // Getters

        /**
         * @brief Get the filepath to the XML file which was passed to the constructor
         *
         * @return The filepath to the XML file
         */
        const FilePath& getFilepath() const noexcept {return mFilepath;}

        /**
         * @brief Get the root of the XML file as a QDomElement
         *
         * Because the constructor has already checked whether this QDomElement is valid
         * or not (QDomElement::isNull()) this object is always valid. You do not have to
         * call QDomElement::isNull() to check this.
         *
         * @return The XML root element
         */
        QDomElement getRoot() const noexcept {return mDomDocument.documentElement();}


        // General Methods

        /**
         * @brief Save the whole XML DOM tree back to the XML file
         *
         * @param toOriginal    If true, the XML will be written to the original file. If
         *                      false, the XML will be written to the backup file (*~).
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        void save(bool toOriginal) throw (Exception);


        // Static Methods

        /**
         * @brief Create a new XML file with the XML header and a root node
         *
         * @param filepath  The filepath of the new XML file
         * @param rootName  The name of the XML root node (no special characters!!)
         *
         * @throw Exception If an error occurs, this method throws an exception.
         */
        static void create(const FilePath& filepath, const QString& rootName) throw (Exception);


    private:

        // make some methods inaccessible...
        XmlFile();
        XmlFile(const XmlFile& other);
        XmlFile& operator=(const XmlFile& rhs);

        // Private Methods

        /**
         * @brief Save a QDomDocument to a XML file
         *
         * @param domDocument   The QDomDocument to save
         * @param filepath      The filepath to the XML file
         *
         * @throw Exception     If an error occurs, this method throws an exception.
         */
        static void saveDomDocument(const QDomDocument& domDocument,
                                    const FilePath& filepath) throw (Exception);


        // General Attributes

        /**
         * @brief The filepath which was passed to the constructor
         */
        FilePath mFilepath;

        /**
         * @brief The whole XML DOM tree
         */
        QDomDocument mDomDocument;

};

#endif // XMLFILE_H
