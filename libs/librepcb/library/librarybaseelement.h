/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "./msg/libraryelementcheckmessage.h"

#include <librepcb/common/fileio/serializablekeyvaluemap.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/uuid.h>
#include <librepcb/common/version.h>

#include <QObject>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Class LibraryBaseElement
 ******************************************************************************/

/**
 * @brief The LibraryBaseElement class
 */
class LibraryBaseElement : public QObject, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryBaseElement() = delete;
  LibraryBaseElement(const LibraryBaseElement& other) = delete;
  LibraryBaseElement(bool dirnameMustBeUuid, const QString& shortElementName,
                     const QString& longElementName, const Uuid& uuid,
                     const Version& version, const QString& author,
                     const ElementName& name_en_US,
                     const QString& description_en_US,
                     const QString& keywords_en_US);
  LibraryBaseElement(std::unique_ptr<TransactionalDirectory> directory,
                     bool dirnameMustBeUuid, const QString& shortElementName,
                     const QString& longElementName);
  virtual ~LibraryBaseElement() noexcept;

  // Getters: General
  const TransactionalDirectory& getDirectory() const noexcept {
    return *mDirectory;
  }
  TransactionalDirectory& getDirectory() noexcept { return *mDirectory; }
  const QString& getShortElementName() const noexcept {
    return mShortElementName;
  }
  const QString& getLongElementName() const noexcept {
    return mLongElementName;
  }

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Version& getVersion() const noexcept { return mVersion; }
  const QString& getAuthor() const noexcept { return mAuthor; }
  const QDateTime& getCreated() const noexcept { return mCreated; }
  bool isDeprecated() const noexcept { return mIsDeprecated; }
  const LocalizedNameMap& getNames() const noexcept { return mNames; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }
  const LocalizedKeywordsMap& getKeywords() const noexcept { return mKeywords; }
  QStringList getAllAvailableLocales() const noexcept;

  // Setters
  void setVersion(const Version& version) noexcept { mVersion = version; }
  void setAuthor(const QString& author) noexcept { mAuthor = author; }
  void setDeprecated(bool deprecated) noexcept { mIsDeprecated = deprecated; }
  void setNames(const LocalizedNameMap& names) noexcept { mNames = names; }
  void setDescriptions(const LocalizedDescriptionMap& descriptions) noexcept {
    mDescriptions = descriptions;
  }
  void setKeywords(const LocalizedKeywordsMap& keywords) noexcept {
    mKeywords = keywords;
  }

  // General Methods
  virtual LibraryElementCheckMessageList runChecks() const;
  virtual void save();
  virtual void saveTo(TransactionalDirectory& dest);
  virtual void moveTo(TransactionalDirectory& dest);
  virtual void saveIntoParentDirectory(TransactionalDirectory& dest);
  virtual void moveIntoParentDirectory(TransactionalDirectory& dest);

  // Operator Overloadings
  LibraryBaseElement& operator=(const LibraryBaseElement& rhs) = delete;

  // Static Methods
  template <typename ElementType>
  static bool isValidElementDirectory(const FilePath& dir) noexcept {
    return dir.getPathTo(".librepcb-" % ElementType::getShortElementName())
        .isExistingFile();
  }
  template <typename ElementType>
  static bool isValidElementDirectory(const TransactionalDirectory& dir,
                                      const QString& path) noexcept {
    return dir.fileExists((path.isEmpty() ? path : path % "/") % ".librepcb-" %
                          ElementType::getShortElementName());
  }

protected:
  // Protected Methods
  virtual void cleanupAfterLoadingElementFromFile() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  virtual void serialize(SExpression& root) const override;

  // General Attributes
  std::unique_ptr<TransactionalDirectory> mDirectory;
  bool mDirectoryNameMustBeUuid;
  QString mShortElementName;  ///< e.g. "lib", "cmpcat", "sym"
  QString mLongElementName;  ///< e.g. "library", "component_category", "symbol"

  // Members required for loading elements from file
  SExpression mLoadingFileDocument;
  Version mLoadingFileFormat;

  // General Library Element Attributes
  Uuid mUuid;
  Version mVersion;
  QString mAuthor;
  QDateTime mCreated;
  bool mIsDeprecated;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  LocalizedKeywordsMap mKeywords;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_LIBRARYBASEELEMENT_H
