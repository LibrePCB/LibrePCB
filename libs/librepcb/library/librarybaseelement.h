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
#include <memory>
#include <QObject>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/serializablekeyvaluemap.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/version.h>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class DomDocument;
class DomElement;

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
        const LocalizedNameMap& getNames() const noexcept {return mNames;}
        const LocalizedDescriptionMap& getDescriptions() const noexcept {return mDescriptions;}
        const LocalizedKeywordsMap& getKeywords() const noexcept {return mKeywords;}
        QStringList getAllAvailableLocales() const noexcept;

        // Setters
        void setVersion(const Version& version) noexcept {mVersion = version;}
        void setAuthor(const QString& author) noexcept {mAuthor = author;}
        void setLastModified(const QDateTime& modified) noexcept {mLastModified = modified.toUTC();}
        void setDeprecated(bool deprecated) noexcept {mIsDeprecated = deprecated;}
        void setName(const QString& locale, const QString& name) noexcept {mNames.insert(locale, name);}
        void setDescription(const QString& locale, const QString& desc) noexcept {mDescriptions.insert(locale, desc);}
        void setKeywords(const QString& locale, const QString& keywords) noexcept {mKeywords.insert(locale, keywords);}

        // General Methods
        virtual void save() throw (Exception);
        virtual void saveTo(const FilePath& destination) throw (Exception);
        virtual void saveIntoParentDirectory(const FilePath& parentDir) throw (Exception);
        virtual void moveTo(const FilePath& destination) throw (Exception);
        virtual void moveIntoParentDirectory(const FilePath& parentDir) throw (Exception);

        // Operator Overloadings
        LibraryBaseElement& operator=(const LibraryBaseElement& rhs) = delete;

        // Static Methods
        template <typename ElementType>
        static bool isValidElementDirectory(const FilePath& dir) noexcept
        {return dir.getPathTo(".librepcb-" % ElementType::getShortElementName()).isExistingFile();}


    protected:

        // Protected Methods
        virtual void cleanupAfterLoadingElementFromFile() noexcept;
        virtual void copyTo(const FilePath& destination, bool removeSource) throw (Exception);

        /// @copydoc librepcb::SerializableObject::serialize()
        virtual void serialize(DomElement& root) const throw (Exception) override;
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
        std::unique_ptr<DomDocument> mLoadingXmlFileDocument;

        // General Library Element Attributes
        Uuid mUuid;
        Version mVersion;
        QString mAuthor;
        QDateTime mCreated;
        QDateTime mLastModified;
        bool mIsDeprecated;
        LocalizedNameMap mNames;
        LocalizedDescriptionMap mDescriptions;
        LocalizedKeywordsMap mKeywords;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_LIBRARYBASEELEMENT_H
