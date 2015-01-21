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
#include "textfile.h"

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
 *          of the filename). See @ref doc_project_save for more details.
 *
 * @warning Do not use QDomElement objects from #getRoot() after the #XmlFile object is
 *          destroyed! Dostroying the XmlFile object will also destroy the whole DOM tree!
 *
 * @author ubruhin
 * @date 2014-08-13
 */
class XmlFile final : public TextFile
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
         * @param readOnly  If true, the file will be opened read-only (see #mIsReadOnly)
         * @param rootName  If this is not an empty string, the constructor compares the
         *                  XML root node name with this value. If they are not identical,
         *                  the constructor throws an exception. This is useful to check
         *                  if the specified XML file has the expected content. If you
         *                  want to open a XML file with unknown content, use an empty
         *                  QString for this parameter and check the root node name with
         *                  <tt>getRoot().nodeName()</tt> afterwards.
         *
         * @throw Exception If the specified XML file could not be opened successful, an
         *                  exception will be thrown.
         */
        XmlFile(const FilePath& filepath, bool restore = false, bool readOnly = false,
                const QString& rootName = QString()) throw (Exception);

        /**
         * @brief The destructor
         *
         * Removes the temporary file
         */
        ~XmlFile() noexcept;


        // Getters

        /**
         * @brief Get the DOM document of the XML file
         *
         * @return A reference to the QDomDocument of the XML file
         */
        QDomDocument& getDocument() noexcept {return mDomDocument;}

        /**
         * @brief Get the root of the XML file as a QDomElement
         *
         * @note    Because the constructor has already checked whether this QDomElement
         *          is valid or not (<tt>QDomElement::isNull()</tt>) this object is always
         *          valid. You do not have to call <tt>QDomElement::isNull()</tt> to check
         *          this (OK, if you "damage" the DOM tree after it was loaded from the
         *          file, this method could return an invalid QDomElement, but if this
         *          happens, we have a big problem anyway... ;-) ).
         *
         * @return The XML root element
         */
        QDomElement& getRoot() noexcept {return mDomRoot;}

        /**
         * @brief Get the version of the file (the root node's attribute "file_version")
         *
         * @return The file version (or -1 if no version is defined in the file)
         */
        int getFileVersion() const noexcept {return mFileVersion;}


        // Setters

        /**
         * @brief Set the version of the file (the root node's attribute "file_version")
         *
         * @param version   The new version number
         */
        void setFileVersion(int version) noexcept;


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
         * If the file does already exist, it will be overwritten.
         *
         * @note This method will NOT create the original file specified with "filepath"!
         *       Instead it will only create it's backup file (*.xml~). You need to call
         *       #save() afterwards if you also want to create the original file.
         *
         * @param filepath  The filepath of the new XML file
         * @param rootName  The name of the XML root node (no special characters!!)
         * @param version   The file version, or -1 if no version number should be added
         *
         * @return A pointer to the new XmlFile object
         *
         * @throw Exception If an error occurs, this method throws an exception.
         */
        static XmlFile* create(const FilePath& filepath, const QString& rootName,
                               int version = -1) throw (Exception);


    private:

        // make some methods inaccessible...
        XmlFile();
        XmlFile(const XmlFile& other);
        XmlFile& operator=(const XmlFile& rhs);


        // General Attributes

        /**
         * @brief The whole XML DOM tree
         */
        QDomDocument mDomDocument;

        /**
         * @brief The root element of #mDomDocument
         */
        QDomElement mDomRoot;

        /**
         * @brief The file version (attribute "file_version" in the root node)
         *
         * If the file does not contain the version number, this attribute is -1.
         */
        int mFileVersion;

};

#endif // XMLFILE_H
