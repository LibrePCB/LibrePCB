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

#ifndef LIBREPCB_CORE_ATTRIBUTEPROVIDER_H
#define LIBREPCB_CORE_ATTRIBUTEPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Interface AttributeProvider
 ******************************************************************************/

/**
 * @brief The AttributeProvider class defines an interface for classes which
 * provides some attributes which can be used as variables in texts (like
 * "{{NAME}}")
 *
 * For example library symbols can contain text elements which contains
 * variables, for example the most importants texts "{{NAME}}" and "{{VALUE}}".
 * All these variables will be parsed and replaced with their values when such a
 * text is displayed in a schematic of a project.
 *
 * Please read the documentation about the @ref doc_attributes_system to get an
 * idea how the @ref doc_attributes_system works in detail.
 *
 * To get the values from the attributes of an object, their class must inherit
 * from ::librepcb::AttributeProvider and override at least one of the methods
 * #getUserDefinedAttributeValue(), #getBuiltInAttributeValue() and
 * #getAttributeProviderParents(), depending on what kind of attributes it
 * provides.
 *
 * @see ::librepcb::AttributeSubstitutor
 * @see @ref doc_attributes_system
 */
class AttributeProvider {
public:
  // Constructors / Destructor / Operator Overloadings
  AttributeProvider() noexcept {}
  AttributeProvider(const AttributeProvider& other) = delete;
  AttributeProvider& operator=(const AttributeProvider& rhs) = delete;
  virtual ~AttributeProvider() noexcept {}

  /**
   * @brief Get the value of an attribute which can be used in texts (like
   * "{{NAME}}")
   *
   * @param key   The attribute key name (e.g. "NAME" in "{{NAME}}").
   *
   * @return The value of the specified attribute (empty if attribute not found)
   */
  QString getAttributeValue(const QString& key) const noexcept;

  /**
   * @brief Get the value of a user defined attribute (if available)
   *
   * @param key   The attribute name (e.g. "NAME" for "{{NAME}}")
   *
   * @return The value of the attribute (empty string if not found)
   */
  virtual QString getUserDefinedAttributeValue(const QString& key) const
      noexcept {
    Q_UNUSED(key);
    return QString();
  }

  /**
   * @brief Get the value of a built-in attribute (if available)
   *
   * @param key   The attribute name (e.g. "NAME" for "{{NAME}}")
   *
   * @return The value of the attribute (empty string if not found)
   */
  virtual QString getBuiltInAttributeValue(const QString& key) const noexcept {
    Q_UNUSED(key);
    return QString();
  }

  /**
   * @brief Get all parent attribute providers (fallback if attribute not found)
   *
   * @return All parent attribute provider objects (empty and nullptr are
   * allowed)
   */
  virtual QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept {
    return QVector<const AttributeProvider*>();
  }

signals:

  /**
   * @brief This signal is emitted when the value of attributes has changed
   *
   * All derived classes must emit this signal when some attributes have changed
   * their values (only attributes which can be fetched with
   * #getAttributeValue(), inclusive all attributes from all "parent" classes).
   */
  virtual void attributesChanged() = 0;

private:
  QString getAttributeValue(const QString& key,
                            QVector<const AttributeProvider*>& backtrace) const
      noexcept;
};

// Make sure that the AttributeProvider class does not contain any data (except
// the vptr). Otherwise it could introduce issues when using multiple
// inheritance.
static_assert(sizeof(AttributeProvider) == sizeof(void*),
              "AttributeProvider must not contain any data!");

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
