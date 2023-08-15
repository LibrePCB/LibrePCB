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

#ifndef LIBREPCB_CORE_WORKSPACELIBRARYDB_H
#define LIBREPCB_CORE_WORKSPACELIBRARYDB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../attribute/attribute.h"
#include "../attribute/attributetype.h"
#include "../fileio/filepath.h"
#include "../types/uuid.h"
#include "../types/version.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
class QSqlQuery;

namespace librepcb {

class Component;
class ComponentCategory;
class Device;
class Package;
class PackageCategory;
class SQLiteDatabase;
class Symbol;
class WorkspaceLibraryScanner;

/*******************************************************************************
 *  Class WorkspaceLibraryDb
 ******************************************************************************/

/**
 * @brief The WorkspaceLibraryDb class
 */
class WorkspaceLibraryDb final : public QObject {
  Q_OBJECT

public:
  struct Part {
    QString mpn;
    QString manufacturer;
    AttributeList attributes;

    bool operator==(const Part& rhs) const noexcept {
      return (mpn == rhs.mpn) && (manufacturer == rhs.manufacturer) &&
          (attributes == rhs.attributes);
    }
    bool operator<(const Part& rhs) const noexcept {
      if (mpn.isEmpty() != rhs.mpn.isEmpty()) {
        return mpn.count() < rhs.mpn.count();
      }
      if (mpn != rhs.mpn) {
        return mpn < rhs.mpn;
      }
      if (manufacturer != rhs.manufacturer) {
        return manufacturer < rhs.manufacturer;
      }
      QCollator collator;
      collator.setNumericMode(true);
      collator.setCaseSensitivity(Qt::CaseInsensitive);
      collator.setIgnorePunctuation(false);
      for (int i = 0; i < std::max(attributes.count(), rhs.attributes.count());
           ++i) {
        auto a = attributes.value(i);
        auto b = rhs.attributes.value(i);
        if (a && (!b)) {
          return false;
        } else if ((!a) && b) {
          return true;
        } else if (a->getKey() != b->getKey()) {
          return a->getKey() < b->getKey();
        } else if (&a->getType() != &b->getType()) {
          return a->getType().getName() < b->getType().getName();
        } else if (a->getValueTr(true) != b->getValueTr(true)) {
          return collator(a->getValueTr(true), b->getValueTr(true));
        }
      }
      return false;
    }
  };

  // Constructors / Destructor
  WorkspaceLibraryDb() = delete;
  WorkspaceLibraryDb(const WorkspaceLibraryDb& other) = delete;

  /**
   * @brief Constructor to open the library database of an existing workspace
   *
   * @param librariesPath   Path to the workspace libraries directory.
   *
   * @throw Exception If the library could not be opened, this constructor
   * throws an exception.
   */
  explicit WorkspaceLibraryDb(const FilePath& librariesPath);
  ~WorkspaceLibraryDb() noexcept;

  // Getters

  /**
   * @brief Get the file path of the SQLite database
   *
   * @return Path to the *.sqlite file
   */
  const FilePath& getFilePath() const noexcept { return mFilePath; }

  /**
   * @brief Check if there is currently a library scan in progress
   *
   * @return Whether a scan is in progress or not.
   */
  bool isScanInProgress() const noexcept {
    return getScanProgressPercent() < 100;
  }

  /**
   * @brief Get the current progress of the library rescan
   *
   * @return Progress in percent (100 = finished).
   */
  int getScanProgressPercent() const noexcept;

  /**
   * @brief Get elements, optionally matching some criteria
   *
   * @tparam ElementType  Type of the library element.
   *
   * @param uuid  If not nullopt, only elements with this UUID are returned.
   * @param lib   If valid, only elements from this library are returned.
   *              Attention: Must not be used when ElementType is Library!
   *
   * @return Version and filepath of all elements matching the criteria.
   */
  template <typename ElementType>
  QMultiMap<Version, FilePath> getAll(
      const tl::optional<Uuid>& uuid = tl::nullopt,
      const FilePath& lib = FilePath()) const {
    return getAll(getTable<ElementType>(), uuid, lib);
  }

  /**
   * @brief Get an element of a specific UUID and the highest version
   *
   * @param uuid  The UUID of the element to get.
   *
   * @return  Filepath of the element with the highest version number and the
   *          specified UUID. If no element is found, an invalid filepath
   *          will be returned.
   */
  template <typename ElementType>
  FilePath getLatest(const Uuid& uuid) const {
    return getLatestVersionFilePath(getAll<ElementType>(uuid));
  }

  /**
   * @brief Find elements by keyword
   *
   * @param keyword   Keyword to search for. Note that the translations for
   *                  all languages will be taken into account.
   *
   * @return  UUIDs of elements matching the filter, sorted alphabetically
   *          and without duplicates. Empty if no elements were found.
   */
  template <typename ElementType>
  QList<Uuid> find(const QString& keyword) const {
    return find(getTable<ElementType>(), keyword);
  }

  /**
   * @brief Find parts by keyword
   *
   * @param keyword   Keyword to search for.
   *
   * @return  All devices which contain parts matching the filter, sorted
   *          alphabetically and without duplicates. Empty if no elements
   *          were found.
   */
  QList<Uuid> findDevicesOfParts(const QString& keyword) const;

  /**
   * @brief Find parts of device by keyword
   *
   * @param device    Device to search for parts.
   * @param keyword   Keyword to search for.
   *
   * @return  All parts of the passed device matching the filter, sorted
   *          alphabetically and without duplicates. Empty if no elements
   *          were found or the device doesn't exist.
   */
  QList<Part> findPartsOfDevice(const Uuid& device,
                                const QString& keyword) const;

  /**
   * @brief Get translations of a specific element
   *
   * @tparam ElementType  Type of the library element.
   *
   * @param elemDir       Library element directory. If it does not exist,
   *                      all translations will be set to an empty string.
   * @param localeOrder   Locale order (highest priority first).
   * @param name          If not nullptr, name will be written here. Set to
   *                      an empty string if the requested data does not exist.
   * @param description   If not nullptr, desc. will be written here. Set to
   *                      an empty string if the requested data does not exist.
   * @param keywords      If not nullptr, keywords will be written here. Set to
   *                      an empty string if the requested data does not exist.
   *
   * @retval true         If the element and corresponding translations were
   *                      found in the database.
   * @retval false        If the element was not found or contains no
   *                      translations ar all.
   */
  template <typename ElementType>
  bool getTranslations(const FilePath& elemDir, const QStringList& localeOrder,
                       QString* name = nullptr, QString* description = nullptr,
                       QString* keywords = nullptr) const {
    return getTranslations(getTable<ElementType>(), elemDir, localeOrder, name,
                           description, keywords);
  }

  /**
   * @brief Get metadata of a specific element
   *
   * @tparam ElementType  Type of the library element.
   *
   * @param elemDir       Library element directory.
   * @param uuid          If not nullptr and the element was found, its
   *                      UUID will be written here.
   * @param version       If not nullptr and the element was found, its
   *                      version will be written here.
   * @param deprecated    If not nullptr and the element was found, its
   *                      deprecation flag will be written here.
   *
   * @retval true If the element was found in the database.
   * @retval false If the element was not found.
   */
  template <typename ElementType>
  bool getMetadata(const FilePath elemDir, Uuid* uuid = nullptr,
                   Version* version = nullptr,
                   bool* deprecated = nullptr) const {
    return getMetadata(getTable<ElementType>(), elemDir, uuid, version,
                       deprecated);
  }

  /**
   * @brief Get additional metadata of a specific library
   *
   * @param libDir        Library directory.
   * @param icon          If not nullptr and the library was found, its
   *                      icon will be written here.
   * @param manufacturer  If not nullptr and the library was found, its
   *                      manufacturer name will be written here (may be
   *                      empty).
   *
   * @retval true If the library was found in the database.
   * @retval false If the library was not found.
   */
  bool getLibraryMetadata(const FilePath libDir, QPixmap* icon = nullptr,
                          QString* manufacturer = nullptr) const;

  /**
   * @brief Get additional metadata of a specific category
   *
   * @tparam ElementType  Type of the library element.
   *
   * @param catDir        Category directory.
   * @param parent        If not nullptr and the category was found, its
   *                      parent will be written here.
   *
   * @retval true If the category was found in the database.
   * @retval false If the category was not found.
   */
  template <typename ElementType>
  bool getCategoryMetadata(const FilePath catDir,
                           tl::optional<Uuid>* parent = nullptr) const {
    static_assert(std::is_same<ElementType, ComponentCategory>::value ||
                      std::is_same<ElementType, PackageCategory>::value,
                  "Unsupported ElementType");
    return getCategoryMetadata(getTable<ElementType>(), catDir, parent);
  }

  /**
   * @brief Get additional metadata of a specific device
   *
   * @param devDir        Device directory.
   * @param pkgUuid       If not nullptr and the device was found, its
   *                      package UUID will be written here.
   * @param cmpUuid       If not nullptr and the device was found, its
   *                      component UUID will be written here.
   *
   * @retval true If the device was found in the database.
   * @retval false If the device was not found.
   */
  bool getDeviceMetadata(const FilePath& devDir, Uuid* cmpUuid = nullptr,
                         Uuid* pkgUuid = nullptr) const;

  /**
   * @brief Get children categories of a specific category
   *
   * @tparam ElementType  Type of the category.
   *
   * @param parent        Category to get the children of. If nullopt,
   *                      all root categories, and categories with inexistent
   *                      parent will be returned (this ensures that all
   *                      elements are discoverable by #getByCategory()).
   *
   * @return  UUIDs of children categories. Empty if the passed category
   *          doesn't exist.
   */
  template <typename ElementType>
  QSet<Uuid> getChilds(const tl::optional<Uuid>& parent) const {
    static_assert(std::is_same<ElementType, ComponentCategory>::value ||
                      std::is_same<ElementType, PackageCategory>::value,
                  "Unsupported ElementType");
    return getChilds(getTable<ElementType>(), parent);
  }

  /**
   * @brief Get elements of a specific category
   *
   * @tparam ElementType  Type of the library element.
   *
   * @param category      Category to get the elements of. If nullopt,
   *                      all elements with no category at all, or with only
   *                      inexistent categories are returned.
   * @param limit         If not -1, the number of results is limited to this
   *                      value. This can be used for performance reasons,
   *                      for example if you only want to know if there are
   *                      *any* elements in the given category, the limit can
   *                      be set to 1, which is much faster than retrieving
   *                      all results.
   *
   * @return UUIDs of elements. Empty if the passed category doesn't exist.
   */
  template <typename ElementType>
  QSet<Uuid> getByCategory(const tl::optional<Uuid>& category,
                           int limit = -1) const {
    static_assert(std::is_same<ElementType, Symbol>::value ||
                      std::is_same<ElementType, Package>::value ||
                      std::is_same<ElementType, Component>::value ||
                      std::is_same<ElementType, Device>::value,
                  "Unsupported ElementType");
    return getByCategory(getTable<ElementType>(),
                         getCategoryTable<ElementType>(), category, limit);
  }

  /**
   * @brief Get all devices of a specific component
   *
   * @param component   Component UUID to get the devices of.
   *
   * @return UUIDs of devices. Empty if the passed component doesn't exist.
   */
  QSet<Uuid> getComponentDevices(const Uuid& component) const;

  /**
   * @brief Get all parts of a specific device
   *
   * @param device      Device UUID to get the parts of.
   *
   * @return All parts. Empty if the passed device doesn't exist.
   */
  QList<Part> getDeviceParts(const Uuid& device) const;

  // General Methods

  /**
   * @brief Rescan the whole library directory and update the SQLite database
   */
  void startLibraryRescan() noexcept;

  // Operator Overloadings
  WorkspaceLibraryDb& operator=(const WorkspaceLibraryDb& rhs) = delete;

signals:
  void scanStarted();
  void scanLibraryListUpdated(int libraryCount);
  void scanProgressUpdate(int percent);
  void scanSucceeded(int elementCount);
  void scanFailed(QString errorMsg);
  void scanFinished();

private:
  // Private Methods
  QMultiMap<Version, FilePath> getAll(const QString& elementsTable,
                                      const tl::optional<Uuid>& uuid,
                                      const FilePath& lib) const;
  FilePath getLatestVersionFilePath(
      const QMultiMap<Version, FilePath>& list) const noexcept;
  QList<Uuid> find(const QString& elementsTable, const QString& keyword) const;
  bool getTranslations(const QString& elementsTable, const FilePath& elemDir,
                       const QStringList& localeOrder, QString* name,
                       QString* description, QString* keywords) const;
  bool getMetadata(const QString& elementsTable, const FilePath elemDir,
                   Uuid* uuid, Version* version, bool* deprecated) const;
  bool getCategoryMetadata(const QString& categoriesTable,
                           const FilePath catDir,
                           tl::optional<Uuid>* parent) const;
  AttributeList getPartAttributes(int partId) const;
  QSet<Uuid> getChilds(const QString& categoriesTable,
                       const tl::optional<Uuid>& categoryUuid) const;
  QSet<Uuid> getByCategory(const QString& elementsTable,
                           const QString& categoryTable,
                           const tl::optional<Uuid>& category, int limit) const;
  static QSet<Uuid> getUuidSet(QSqlQuery& query);
  int getDbVersion() const noexcept;
  template <typename ElementType>
  static QString getTable() noexcept;
  template <typename ElementType>
  static QString getCategoryTable() noexcept;

  // Attributes
  const FilePath mLibrariesPath;  ///< Path to workspace libraries directory.
  const FilePath mFilePath;  ///< Path to the SQLite database file.
  QScopedPointer<SQLiteDatabase> mDb;  ///< The SQLite database.
  QScopedPointer<WorkspaceLibraryScanner> mLibraryScanner;

  // Constants
  static const int sCurrentDbVersion = 5;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
