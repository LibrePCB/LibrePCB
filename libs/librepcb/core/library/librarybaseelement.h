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

#ifndef LIBREPCB_CORE_LIBRARYBASEELEMENT_H
#define LIBREPCB_CORE_LIBRARYBASEELEMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/transactionaldirectory.h"
#include "../serialization/serializablekeyvaluemap.h"
#include "../types/uuid.h"
#include "../types/version.h"
#include "./msg/libraryelementcheckmessage.h"

#include <QObject>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LibraryBaseElement
 ******************************************************************************/

/**
 * @brief The LibraryBaseElement class
 */
class LibraryBaseElement : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryBaseElement() = delete;
  LibraryBaseElement(const LibraryBaseElement& other) = delete;
  LibraryBaseElement(const QString& shortElementName,
                     const QString& longElementName, const Uuid& uuid,
                     const Version& version, const QString& author,
                     const ElementName& name_en_US,
                     const QString& description_en_US,
                     const QString& keywords_en_US);
  LibraryBaseElement(const QString& shortElementName,
                     const QString& longElementName, bool dirnameMustBeUuid,
                     std::unique_ptr<TransactionalDirectory> directory,
                     const SExpression& root);
  virtual ~LibraryBaseElement() noexcept;

  // Getters: General
  const TransactionalDirectory& getDirectory() const noexcept {
    return *mDirectory;
  }
  TransactionalDirectory& getDirectory() noexcept { return *mDirectory; }

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
  const QSet<SExpression>& getMessageApprovals() const noexcept {
    return mMessageApprovals;
  }

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
  void setMessageApprovals(const QSet<SExpression>& approvals) noexcept {
    mMessageApprovals = approvals;
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

protected:  // Methods
  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  virtual void serialize(SExpression& root) const;

  void serializeMessageApprovals(SExpression& root) const;

  static Version readFileFormat(const TransactionalDirectory& directory,
                                const QString& fileName);

protected:  // Data
  // General Attributes
  const QString mShortElementName;  ///< e.g. "lib", "cmpcat"
  const QString mLongElementName;  ///< e.g. "library", "component_category"
  std::unique_ptr<TransactionalDirectory> mDirectory;

  // General Library Element Attributes
  Uuid mUuid;
  Version mVersion;
  QString mAuthor;
  QDateTime mCreated;
  bool mIsDeprecated;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  LocalizedKeywordsMap mKeywords;

  // Library element check
  QSet<SExpression> mMessageApprovals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
