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

#ifndef LIBREPCB_CORE_WORKSPACELIBRARYDBWRITER_H
#define LIBREPCB_CORE_WORKSPACELIBRARYDBWRITER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"
#include "../types/elementname.h"
#include "../types/simplestring.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Attribute;
class Component;
class ComponentCategory;
class Device;
class Package;
class PackageCategory;
class SQLiteDatabase;
class Symbol;
class Uuid;
class Version;

/*******************************************************************************
 *  Class WorkspaceLibraryDbWriter
 ******************************************************************************/

/**
 * @brief Database write functions for ::librepcb::WorkspaceLibraryDb
 */
class WorkspaceLibraryDbWriter final {
public:
  // Constructors / Destructor
  WorkspaceLibraryDbWriter() = delete;
  WorkspaceLibraryDbWriter(const WorkspaceLibraryDbWriter& other) = delete;
  WorkspaceLibraryDbWriter(const FilePath& librariesRoot, SQLiteDatabase& db);
  ~WorkspaceLibraryDbWriter() noexcept;

  // General Methods

  /**
   * @brief Create all tables to initialize the database
   *
   * This has to be done only once, after creating a new database.
   */
  void createAllTables();

  /**
   * @brief Add an integer value to the "internal" table
   *
   * @param key     The key to add.
   * @param value   The value to add.
   */
  void addInternalData(const QString& key, int value);

  /**
   * @brief Add a library
   *
   * @param fp            Filepath of the library.
   * @param uuid          UUID of the library.
   * @param version       Version of the library.
   * @param deprecated    Whether the library is deprecated or not.
   * @param iconPng       Icon as a PNG.
   * @param manufacturer  Name of the manufacturer of this library (optional).
   * @return ID of the added library.
   */
  int addLibrary(const FilePath& fp, const Uuid& uuid, const Version& version,
                 bool deprecated, const QByteArray& iconPng,
                 const QString& manufacturer);

  /**
   * @brief Update library metadata
   *
   * @param fp            Filepath of the library to update.
   * @param uuid          New UUID of the library.
   * @param version       New version of the library.
   * @param deprecated    Whether the library is deprecated or not.
   * @param iconPng       New icon as a PNG.
   * @param manufacturer  Name of the manufacturer of this library (optional).
   */
  void updateLibrary(const FilePath& fp, const Uuid& uuid,
                     const Version& version, bool deprecated,
                     const QByteArray& iconPng, const QString& manufacturer);

  /**
   * @brief Add a library element
   *
   * @tparam ElementType  Type of element to add.
   * @param libId         ID of the library containing this element.
   * @param fp            Filepath of the element.
   * @param uuid          UUID of the element.
   * @param version       Version of the element.
   * @param deprecated    Whether the element is deprecated or not.
   * @param generatedBy   The generator name if generated or imported.
   * @return ID of the added element.
   */
  template <typename ElementType>
  int addElement(int libId, const FilePath& fp, const Uuid& uuid,
                 const Version& version, bool deprecated,
                 const QString& generatedBy) {
    static_assert(std::is_same<ElementType, Symbol>::value ||
                      std::is_same<ElementType, Package>::value ||
                      std::is_same<ElementType, Component>::value,
                  "Unsupported ElementType");
    return addElement(getElementTable<ElementType>(), libId, fp, uuid, version,
                      deprecated, generatedBy);
  }

  /**
   * @brief #addElement() specialized for categories
   *
   * @tparam ElementType  Type of category to add.
   * @param libId         ID of the library containing this category.
   * @param fp            Filepath of the category.
   * @param uuid          UUID of the category.
   * @param version       Version of the category.
   * @param deprecated    Whether the category is deprecated or not.
   * @param parent        Parent of the category.
   * @return ID of the added category.
   */
  template <typename ElementType>
  int addCategory(int libId, const FilePath& fp, const Uuid& uuid,
                  const Version& version, bool deprecated,
                  const tl::optional<Uuid>& parent) {
    static_assert(std::is_same<ElementType, ComponentCategory>::value ||
                      std::is_same<ElementType, PackageCategory>::value,
                  "Unsupported ElementType");
    return addCategory(getElementTable<ElementType>(), libId, fp, uuid, version,
                       deprecated, parent);
  }

  /**
   * @brief #addElement() specialized for devices
   *
   * @param libId         ID of the library containing this device.
   * @param fp            Filepath of the device.
   * @param uuid          UUID of the device.
   * @param version       Version of the device.
   * @param deprecated    Whether the device is deprecated or not.
   * @param generatedBy   The generator name if generated or imported.
   * @param component     Component UUID of the device.
   * @param package       Package UUID of the device.
   * @return ID of the added device.
   */
  int addDevice(int libId, const FilePath& fp, const Uuid& uuid,
                const Version& version, bool deprecated,
                const QString& generatedBy, const Uuid& component,
                const Uuid& package);

  /**
   * @brief Add a part to a previously added device
   *
   * @param devId         ID of the device containing this part.
   * @param mpn           Manufacturer part number.
   * @param manufacturer  Manufacturer name.
   * @return ID of the added part.
   */
  int addPart(int devId, const QString& mpn, const QString& manufacturer);

  /**
   * @brief Add an attribute to a previously added part
   *
   * @param partId        ID of the part containing this attribute.
   * @param attribute     Attribute to add.
   * @return ID of the added attribute.
   */
  int addPartAttribute(int partId, const Attribute& attribute);

  /**
   * @brief Remove a library element
   *
   * @note  This will automatically remove its translations and categories
   *        as well.
   *
   * @tparam ElementType  Type of element to remove.
   * @param fp            Filepath of the element to remove.
   */
  template <typename ElementType>
  void removeElement(const FilePath& fp) {
    removeElement(getElementTable<ElementType>(), fp);
  }

  /**
   * @brief Remove all library elements of a specific type
   *
   * @note  This will automatically remove their translations and categories
   *        as well.
   *
   * @tparam ElementType  Type of elements to remove.
   */
  template <typename ElementType>
  void removeAllElements() {
    removeAllElements(getElementTable<ElementType>());
  }

  /**
   * @brief Add a translation for a library element
   *
   * @tparam ElementType  Type of element to add translations.
   * @param elementId     ID of the element to add translations.
   * @param locale        Locale of the translations.
   * @param name          Element name.
   * @param description   Eleemnt description.
   * @param keywords      Element keywords.
   * @return ID of the added translation.
   */
  template <typename ElementType>
  int addTranslation(int elementId, const QString& locale,
                     const tl::optional<ElementName>& name,
                     const tl::optional<QString>& description,
                     const tl::optional<QString>& keywords) {
    return addTranslation(getElementTable<ElementType>(), elementId, locale,
                          name, description, keywords);
  }

  /**
   * @brief Remove all translations for a library element type
   *
   * @tparam ElementType  Type of element to remove translationss.
   */
  template <typename ElementType>
  void removeAllTranslations() {
    removeAllTranslations(getElementTable<ElementType>());
  }

  /**
   * @brief Add a library element to a category
   *
   * @tparam ElementType  Type of element to add to the category.
   * @param elementId     ID of the element to add to the category.
   * @param category      Category UUID.
   * @return ID of the added category.
   */
  template <typename ElementType>
  int addToCategory(int elementId, const Uuid& category) {
    static_assert(std::is_same<ElementType, Symbol>::value ||
                      std::is_same<ElementType, Package>::value ||
                      std::is_same<ElementType, Component>::value ||
                      std::is_same<ElementType, Device>::value,
                  "Unsupported ElementType");
    return addToCategory(getElementTable<ElementType>(), elementId, category);
  }

  /**
   * @brief Add a resource for a library element
   *
   * @tparam ElementType  Type of element of the resource.
   * @param elementId     ID of the element of the resource.
   * @param name          Resource name
   * @param mediaType     Resource media type.
   * @param url           Resource URL.
   * @return ID of the added resource.
   */
  template <typename ElementType>
  int addResource(int elementId, const QString& name, const QString& mediaType,
                  const QUrl& url) {
    static_assert(std::is_same<ElementType, Component>::value ||
                      std::is_same<ElementType, Device>::value,
                  "Unsupported ElementType");
    return addResource(getElementTable<ElementType>(), elementId, name,
                       mediaType, url);
  }

  /**
   * @brief Add an alternative name to a previously added package
   *
   * @param pkgId         ID of the package for this alternative name.
   * @param name          Alternative name (mandatory).
   * @param reference     Origin of the alternative name (optional).
   * @return ID of the added part.
   */
  int addAlternativeName(int pkgId, const ElementName& name,
                         const SimpleString& reference);

  // Helper Functions

  /**
   * @brief Get the table name of an element type
   *
   * @tparam ElementType  Type of element to get the table name of.
   * @return Table name (e.g. "symbols" for ::librepcb::Symbol).
   */
  template <typename ElementType>
  static QString getElementTable() noexcept;

  /**
   * @brief Get the category table name of an element type
   *
   * @tparam ElementType  Type of element to get the category table name of.
   * @return  Category table name (e.g. "component_categories" for
   *          ::librepcb::Symbol).
   */
  template <typename ElementType>
  static QString getCategoryTable() noexcept;

  // Operator Overloadings
  WorkspaceLibraryDbWriter& operator=(const WorkspaceLibraryDbWriter& rhs) =
      delete;

private:  // Methods
  int addElement(const QString& elementsTable, int libId, const FilePath& fp,
                 const Uuid& uuid, const Version& version, bool deprecated,
                 const QString& generatedBy);
  int addCategory(const QString& categoriesTable, int libId, const FilePath& fp,
                  const Uuid& uuid, const Version& version, bool deprecated,
                  const tl::optional<Uuid>& parent);
  void removeElement(const QString& elementsTable, const FilePath& fp);
  void removeAllElements(const QString& elementsTable);
  int addTranslation(const QString& elementsTable, int elementId,
                     const QString& locale,
                     const tl::optional<ElementName>& name,
                     const tl::optional<QString>& description,
                     const tl::optional<QString>& keywords);
  void removeAllTranslations(const QString& elementsTable);
  int addToCategory(const QString& elementsTable, int elementId,
                    const Uuid& category);
  int addResource(const QString& elementsTable, int elementId,
                  const QString& name, const QString& mediaType,
                  const QUrl& url);
  QString filePathToString(const FilePath& fp) const noexcept;
  static QString nonEmptyOrNull(const QString& s) noexcept;
  static QString nonNull(const QString& s) noexcept;

private:  // Data
  FilePath mLibrariesRoot;
  SQLiteDatabase& mDb;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
