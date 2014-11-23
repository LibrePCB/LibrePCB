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
#include <QDomElement>
#include "../common/xmlfile.h"
#include "../common/filepath.h"
#include "../common/version.h"

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
        explicit LibraryBaseElement(const FilePath& xmlFilePath,
                                    const QString& xmlRootNodeName) throw (Exception);
        virtual ~LibraryBaseElement() noexcept;

        // Getters: General
        const FilePath& getXmlFilepath() const noexcept {return mXmlFilepath;}
        int getXmlFileVersion() const noexcept {return mXmlFile->getFileVersion();}

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Version& getVersion() const noexcept {return mVersion;}
        const QString& getAuthor() const noexcept {return mAuthor;}
        const QDateTime& getCreated() const noexcept {return mCreated;}
        const QDateTime& getLastModified() const noexcept {return mLastModified;}
        QString getName(const QString& locale = QString()) const noexcept;
        QString getDescription(const QString& locale = QString()) const noexcept;
        QString getKeywords(const QString& locale = QString()) const noexcept;
        const QHash<QString, QString>& getNames() const noexcept {return mNames;}
        const QHash<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}
        const QHash<QString, QString>& getKeywords() const noexcept {return mKeywords;}


        // Static Methods

        /**
         * @brief Read locale-dependent strings from a DOM node and insert them in a QHash
         *
         * If you have DOM nodes with strings in different languages, this method helps
         * you to read these strings and create a QHash with all entries. This method will
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
         * @param xmlFilepath       The filepath to the XML file of the specified DOM node.
         *                          This path is only used for exception messages.
         * @param parentNode        The parent node ("my_parent_node" in the example)
         * @param childNodesName    The name of the subnodes to read ("my_subnode" in the example)
         * @param list              The QHash where the new locale/text pair will be
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
        static void readLocaleDomNodes(const FilePath& xmlFilepath, QDomElement parentNode,
                                       const QString& childNodesName,
                                       QHash<QString, QString>& list) throw (Exception);

        /**
         * @brief Get the string of a specific locale from a QHash
         *
         * This method can be used to extract a text in a specific language from a QHash
         * which was generated with #readLocaleDomNodes(). If the list doesn't contain a
         * translation for the selected language, the "nearest" other language will be
         * used instead (the language order is defined in the workspace settings, see
         * #WSI_LibraryLocaleOrder for details). If all these languages are not available
         * in the list, the language "en_US" is used instead.
         *
         * @param list          The list which contains all locales and their translations
         *                      (created with #readLocaleDomNodes())
         * @param locale        The locale you want to read (for example "de_CH"). Pass an
         *                      empty QString to use the locales from the workspace settings.
         * @param usedLocale    The locale which was really used (locale of the returned string)
         * @return              The string from the list in the specified language
         *
         * @throw Exception     If no translation is found in the list (not even "en_US"),
         *                      an exception will be thrown
         *
         * @see #readLocaleDomNodes(), #WSI_LibraryLocaleOrder
         */
        static QString localeStringFromList(const QHash<QString, QString>& list,
                                            const QString& locale, QString* usedLocale = 0) throw (Exception);


    private:

        // make some methods inaccessible...
        LibraryBaseElement();
        LibraryBaseElement(const LibraryBaseElement& other);
        LibraryBaseElement& operator=(const LibraryBaseElement& rhs);


    protected:

        // General Attributes
        FilePath mXmlFilepath;
        XmlFile* mXmlFile;
        QDomElement mDomRoot;

        // General Library Element Attributes
        QUuid mUuid;
        Version mVersion;
        QString mAuthor;
        QDateTime mCreated;
        QDateTime mLastModified;
        QHash<QString, QString> mNames;        ///< key: locale (like "en_US"), value: name
        QHash<QString, QString> mDescriptions; ///< key: locale (like "en_US"), value: description
        QHash<QString, QString> mKeywords;     ///< key: locale (like "en_US"), value: keywords
};

} // namespace library

#endif // LIBRARY_LIBRARYBASEELEMENT_H
