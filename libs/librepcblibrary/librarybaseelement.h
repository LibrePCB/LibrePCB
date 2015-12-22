/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBRARY_LIBRARYBASEELEMENT_H
#define LIBRARY_LIBRARYBASEELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QObject>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/version.h>
#include <librepcbcommon/uuid.h>

/*****************************************************************************************
 *  Class LibraryBaseElement
 ****************************************************************************************/

namespace library {

/**
 * @brief The LibraryBaseElement class
 */
class LibraryBaseElement : public QObject, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit LibraryBaseElement(const QString& xmlFileNamePrefix,
                                    const QString& xmlRootNodeName,
                                    const Uuid& uuid = Uuid::createRandom(),
                                    const Version& version = Version(),
                                    const QString& author = QString(),
                                    const QString& name_en_US = QString(),
                                    const QString& description_en_US = QString(),
                                    const QString& keywords_en_US = QString()) throw (Exception);
        explicit LibraryBaseElement(const FilePath& elementDirectory,
                                    const QString& xmlFileNamePrefix,
                                    const QString& xmlRootNodeName) throw (Exception);
        virtual ~LibraryBaseElement() noexcept;

        // Getters: General
        const FilePath& getDirectory() const noexcept {return mDirectory;}
        const FilePath& getXmlFilepath() const noexcept {return mXmlFilepath;}

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Version& getVersion() const noexcept {return mVersion;}
        const QString& getAuthor() const noexcept {return mAuthor;}
        const QDateTime& getCreated() const noexcept {return mCreated;}
        const QDateTime& getLastModified() const noexcept {return mLastModified;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        QString getKeywords(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}
        const QMap<QString, QString>& getKeywords() const noexcept {return mKeywords;}
        QStringList getAllAvailableLocales() const noexcept;

        // Setters
        void setUuid(const Uuid& uuid) noexcept {mUuid = uuid;}
        void setName(const QString& locale, const QString& name) noexcept {mNames[locale] = name;}
        void setDescription(const QString& locale, const QString& desc) noexcept {mDescriptions[locale] = desc;}
        void setKeywords(const QString& locale, const QString& keywords) noexcept {mKeywords[locale] = keywords;}
        void setVersion(const Version& version) noexcept {mVersion = version;}
        void setAuthor(const QString& author) noexcept {mAuthor = author;}

        // General Methods
        void save() const throw (Exception);
        void saveTo(const FilePath& parentDir) const throw (Exception);

        // Static Methods

        /**
         * @brief Read locale-dependent strings from a DOM node and insert them in a QMap
         *
         * If you have DOM nodes with strings in different languages, this method helps
         * you to read these strings and create a QMap with all entries. This method will
         * also check if an entry with the language "en_US" exists in the DOM node.
         *
         * Example of a valid DOM node:
         * @code
         * <my_parent_node>
         *     <my_subnode locale="en_US">the value</my_subnode>
         *     <my_subnode locale="de_DE">der wert</my_subnode>
         * </my_parent_node>
         * @endcode
         *
         * @note
         *  - The parent node name ("my_parent_node") is arbitrary
         *  - The subnode name ("my_subnode") is arbitrary
         *  - It's allowed to have other subnodes, they will not be parsed/modified
         *  - The subnode must contain at least the attribute which is called "locale"
         *  - At least one entry with the locale "en_US" must exist!
         *
         * @param parentNode        The parent node ("my_parent_node" in the example)
         * @param childNodesName    The name of the subnodes to read ("my_subnode" in the example)
         * @param list              The QMap where the new locale/text pair will be
         *                          inserted. The locales (values of "locale" attributes)
         *                          are the keys of the list, the node texts ("the value"
         *                          in the example) are the values of the list.
         *
         * @throw Exception     This method will throw an exception in the following cases:
         *  - The attribute "locale" of a node does not exist or its value is empty
         *  - There are multiple nodes with the same locale
         *  - There is no entry with the locale "en_US"
         *
         * @see #localeStringFromList()
         */
        static void readLocaleDomNodes(const XmlDomElement& parentNode,
                                       const QString& childNodesName,
                                       QMap<QString, QString>& list) throw (Exception);

        /**
         * @brief Get the string of a specific locale from a QMap
         *
         * This method can be used to extract a text in a specific language from a QMap
         * which was generated with #readLocaleDomNodes(). If the list doesn't contain a
         * translation for the selected language, the "nearest" other language will be
         * used instead. Therefore you can to pass a QStringList with the locales to use.
         * If all these languages are not available in the QMap, always the language
         * "en_US" is used instead (if available, otherwise an exception will be thrown).
         *
         * @param list          The list which contains all locales and their translations
         *                      (created with #readLocaleDomNodes())
         * @param localeOrder   The locale you want to read (for example "de_CH") must be
         *                      on top (index 0) of this list. To use fallback locales,
         *                      the list can have more than one item (order is important!).
         * @param usedLocale    The locale which was really used (locale of the returned
         *                      string). Pass nullptr if this return value is not needed.
         *
         * @return              The string from the list in the specified language
         *
         * @throw Exception     If no translation is found in the list (not even "en_US")
         *
         * @see #readLocaleDomNodes(), #WSI_LibraryLocaleOrder
         */
        static QString localeStringFromList(const QMap<QString, QString>& list,
                                            const QStringList& localeOrder,
                                            QString* usedLocale = nullptr) throw (Exception);

        /**
         * @brief Check whether a directory contains a valid library element or not
         *
         * @param dir   The element's root directory ("{UUID}.type")
         *
         * @return True if there is a valid element, false if not (or the library element's
         *              file version is newer than the application's major version)
         */
        static bool isDirectoryValidElement(const FilePath& dir) noexcept;


    private:

        // make some methods inaccessible...
        LibraryBaseElement(const LibraryBaseElement& other);
        LibraryBaseElement& operator=(const LibraryBaseElement& rhs);


    protected:

        // Protected Methods
        void readFromFile() throw (Exception);
        virtual void parseDomTree(const XmlDomElement& root) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        virtual XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        virtual bool checkAttributesValidity() const noexcept override;


        // General Attributes
        mutable FilePath mDirectory;
        mutable FilePath mVersionFilepath;
        mutable FilePath mXmlFilepath;
        QString mXmlFileNamePrefix;
        QString mXmlRootNodeName;
        bool mDomTreeParsed;

        // General Library Element Attributes
        Uuid mUuid;
        Version mVersion;
        QString mAuthor;
        QDateTime mCreated;
        QDateTime mLastModified;
        QMap<QString, QString> mNames;        ///< key: locale (like "en_US"), value: name
        QMap<QString, QString> mDescriptions; ///< key: locale (like "en_US"), value: description
        QMap<QString, QString> mKeywords;     ///< key: locale (like "en_US"), value: keywords
};

} // namespace library

#endif // LIBRARY_LIBRARYBASEELEMENT_H
