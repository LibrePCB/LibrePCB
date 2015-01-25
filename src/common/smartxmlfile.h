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

#ifndef SMARTXMLFILE_H
#define SMARTXMLFILE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomDocument>
#include <QDomElement>
#include "smartfile.h"

/*****************************************************************************************
 *  Class SmartXmlFile
 ****************************************************************************************/

/**
 * @brief The SmartXmlFile class represents a XML file and provides access over a QDomElement
 *
 * The constructor will try to open or create a XML file. Then the whole XML DOM tree is
 * stored in the private attribute #mDomDocument. With #getRoot() you can work with the
 * XML root node (read/write/add/remove nodes). With SmartFile#save() the whole DOM tree
 * can be written back to the XML file. The destructor will close the file and delete the
 * QDomDocument.
 *
 * @note See class #SmartFile for more information.
 *
 * @warning Do not use QDomElement objects from #getRoot() after the #SmartXmlFile object is
 *          destroyed! Dostroying the SmartXmlFile object will also destroy the whole DOM tree!
 *
 * @author ubruhin
 * @date 2014-08-13
 */
class SmartXmlFile final : public SmartFile
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open an existing XML file
         *
         * This constructor tries to open an existing file and throws an exception if an
         * error occurs.
         *
         * @param filepath  See SmartFile#SmartFile()
         * @param restore   See SmartFile#SmartFile()
         * @param readOnly  See SmartFile#SmartFile()
         * @param rootName  If this is not an empty string, the constructor compares the
         *                  XML root node name with this value. If they are not identical,
         *                  the constructor throws an exception. This is useful to check
         *                  if the specified XML file has the expected content. If you
         *                  want to open a XML file with unknown content, use an empty
         *                  QString for this parameter and check the root node name with
         *                  <tt>getRoot().nodeName()</tt> afterwards.
         * @param version   If you pass a number greater than -1, the constructor will
         *                  check if the file version of the opened file equals to that
         *                  number and throws an exception if not.
         *
         * @throw Exception If the specified text file could not be opened successful, an
         *                  exception will be thrown.
         */
        SmartXmlFile(const FilePath& filepath, bool restore, bool readOnly,
                     const QString& rootName = QString(), int version = -1) throw (Exception) :
            SmartXmlFile(filepath, restore, readOnly, false, rootName, version, -1) {}

        /**
         * @copydoc SmartFile#~SmartFile()
         */
        ~SmartXmlFile() noexcept;


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


        // Static Methods

        /**
         * @brief Create a new text file
         *
         * @note    This method will NOT immediately create the file! The file will be
         *          created after calling SmartFile#save().
         *
         * @param filepath  The filepath to the file to create (always to the original file,
         *                  not to the backup file with "~" at the end of the filename!)
         * @param rootName  The name of the new root node (must not be empty)
         * @param version   The file version of the created file (-1 if not needed)
         *
         * @return The #SmartXmlFile object of the created file
         *
         * @throw Exception If an error occurs
         */
        static SmartXmlFile* create(const FilePath &filepath, const QString& rootName,
                                    int version = -1) throw (Exception);


    private:

        // make some methods inaccessible...
        SmartXmlFile();
        SmartXmlFile(const SmartXmlFile& other);
        SmartXmlFile& operator=(const SmartXmlFile& rhs);


        // Private Methods

        /**
         * @brief Constructor to create or open a XML file
         *
         * @param filepath          See SmartFile#SmartFile()
         * @param restore           See SmartFile#SmartFile()
         * @param readOnly          See SmartFile#SmartFile()
         * @param create            See SmartFile#SmartFile()
         * @param rootName          The name of the new root node (must not be empty)
         * @param expectedVersion   The expected file version of the opened file
         *                          (-1 if not needed)
         * @param createVersion     The file version of the created file (-1 if not needed)
         *
         * @throw Exception See SmartFile#SmartFile()
         */
        SmartXmlFile(const FilePath& filepath, bool restore, bool readOnly, bool create,
                     const QString& rootName, int expectedVersion, int createVersion) throw (Exception);

        /**
         * @copydoc SmartFile#saveToFile()
         */
        void saveToFile(const FilePath& filepath) throw (Exception);


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

#endif // SMARTXMLFILE_H
