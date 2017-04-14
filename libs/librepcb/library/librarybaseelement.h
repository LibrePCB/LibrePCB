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

#ifndef LIBREPCB_LIBRARY_LIBRARYBASEELEMENT_H
#define LIBREPCB_LIBRARY_LIBRARYBASEELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QObject>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/version.h>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class XmlDomDocument;
class XmlDomElement;

namespace library {

/*****************************************************************************************
 *  Class LibraryBaseElement
 ****************************************************************************************/

/**
 * @brief The LibraryBaseElement class
 */
class LibraryBaseElement : public QObject, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryBaseElement() = delete;
        LibraryBaseElement(const LibraryBaseElement& other) = delete;
        LibraryBaseElement(bool dirnameMustBeUuid, const QString& shortElementName,
                           const QString& longElementName, const Uuid& uuid,
                           const Version& version, const QString& author,
                           const QString& name_en_US, const QString& description_en_US,
                           const QString& keywords_en_US) throw (Exception);
        LibraryBaseElement(const FilePath& elementDirectory, bool dirnameMustBeUuid,
                           const QString& shortElementName, const QString& longElementName,
                           bool readOnly) throw (Exception);
        virtual ~LibraryBaseElement() noexcept;

        // Getters: General
        const FilePath& getFilePath() const noexcept {return mDirectory;}
        bool isOpenedReadOnly() const noexcept {return mOpenedReadOnly;}

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const Version& getVersion() const noexcept {return mVersion;}
        const QString& getAuthor() const noexcept {return mAuthor;}
        const QDateTime& getCreated() const noexcept {return mCreated;}
        const QDateTime& getLastModified() const noexcept {return mLastModified;}
        bool isDeprecated() const noexcept {return mIsDeprecated;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        QString getKeywords(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}
        const QMap<QString, QString>& getKeywords() const noexcept {return mKeywords;}
        QStringList getAllAvailableLocales() const noexcept;

        // Setters
        void setVersion(const Version& version) noexcept {mVersion = version;}
        void setAuthor(const QString& author) noexcept {mAuthor = author;}
        void setLastModified(const QDateTime& modified) noexcept {mLastModified = modified.toUTC();}
        void setDeprecated(bool deprecated) noexcept {mIsDeprecated = deprecated;}
        void setName(const QString& locale, const QString& name) noexcept {mNames[locale] = name;}
        void setDescription(const QString& locale, const QString& desc) noexcept {mDescriptions[locale] = desc;}
        void setKeywords(const QString& locale, const QString& keywords) noexcept {mKeywords[locale] = keywords;}

        // General Methods
        virtual void save() throw (Exception);
        virtual void saveTo(const FilePath& destination) throw (Exception);
        virtual void saveIntoParentDirectory(const FilePath& parentDir) throw (Exception);
        virtual void moveTo(const FilePath& destination) throw (Exception);
        virtual void moveIntoParentDirectory(const FilePath& parentDir) throw (Exception);

        // Operator Overloadings
        LibraryBaseElement& operator=(const LibraryBaseElement& rhs) = delete;

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
         * @param throwIfValueEmpty If true and at least one value is an empty string,
         *                          an exception will be thrown.
         *
         * @throw Exception     This method will throw an exception in the following cases:
         *  - The attribute "locale" of a node does not exist or its value is empty
         *  - There are multiple nodes with the same locale
         *  - There is no entry with the locale "en_US"
         *  - There was an empty entry and "throwIfValueEmpty" is set to "true"
         *
         * @see #localeStringFromList()
         */
        static void readLocaleDomNodes(const XmlDomElement& parentNode,
                                       const QString& childNodesName,
                                       QMap<QString, QString>& list,
                                       bool throwIfValueEmpty) throw (Exception);

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

        template <typename ElementType>
        static bool isValidElementDirectory(const FilePath& dir) noexcept
        {return dir.getPathTo(".librepcb-" % ElementType::getShortElementName()).isExistingFile();}


    protected:

        // Protected Methods
        virtual void cleanupAfterLoadingElementFromFile() noexcept;
        virtual void copyTo(const FilePath& destination, bool removeSource) throw (Exception);

        /// @copydoc librepcb::SerializableObject::serialize()
        virtual void serialize(XmlDomElement& root) const throw (Exception) override;
        virtual bool checkAttributesValidity() const noexcept;


        // General Attributes
        mutable FilePath mDirectory;
        mutable bool mDirectoryIsTemporary;
        bool mOpenedReadOnly;
        bool mDirectoryNameMustBeUuid;
        QString mShortElementName; ///< e.g. "lib", "cmpcat", "sym"
        QString mLongElementName; ///< e.g. "library", "component_category", "symbol"

        // Members required for loading elements from file
        Version mLoadingElementFileVersion;
        QSharedPointer<XmlDomDocument> mLoadingXmlFileDocument;

        // General Library Element Attributes
        Uuid mUuid;
        Version mVersion;
        QString mAuthor;
        QDateTime mCreated;
        QDateTime mLastModified;
        bool mIsDeprecated;
        QMap<QString, QString> mNames;        ///< key: locale (like "en_US"), value: name
        QMap<QString, QString> mDescriptions; ///< key: locale (like "en_US"), value: description
        QMap<QString, QString> mKeywords;     ///< key: locale (like "en_US"), value: keywords
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_LIBRARYBASEELEMENT_H
