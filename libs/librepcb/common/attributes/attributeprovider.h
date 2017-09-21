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

#ifndef LIBREPCB_ATTRIBUTEPROVIDER_H
#define LIBREPCB_ATTRIBUTEPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Interface AttributeProvider
 ****************************************************************************************/

/**
 * @brief The AttributeProvider class defines an interface for classes which provides
 *        some attributes which can be used as variables in texts (like "${NAME}")
 *
 * For example library symbols can contain text elements which contains variables, for
 * example the most importants texts "${NAME}" and "${VALUE}". All these variables will be
 * parsed and replaced with their values when such a text is displayed in a schematic of a
 * project.
 *
 * The main goal of this interface is to provide the method #replaceVariablesWithAttributes()
 * which will replace all variables in a text with their values. To get the values from
 * attributes, the pure virtual method #getAttributeValue() have to be implemented in all
 * classes which inherit from librepcb::AttributeProvider.
 *
 * To resolve a variable like "${NAME}", the class #project#ComponentInstance must
 * inherit from this interface class. The method #getAttributeValue() must be implemented
 * and should return the name of the component instance (like "U123") when the attribute
 * "${NAME}" was requested.
 *
 * @author ubruhin
 * @date 2015-01-10
 */
class AttributeProvider
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         */
        AttributeProvider() {}

        /**
         * @brief Destructor
         */
        virtual ~AttributeProvider() {}


        // General Methods

        /**
         * @brief Replace all variables in a text with their attribute values
         *
         * This method fetches all attribute values with #getAttributeValue() for all
         * variables found in the given text and replaces them in the text.
         *
         * @param rawText           A text which can contain variables ("${KEY}"). The
         *                          variables will be replaced directly in this QString.
         * @param passToParents     If true, the method #getAttributeValue() may also
         *                          call the same method of a "parent attribute provider"
         *                          class to fetch the requested attribute value (for
         *                          example #project#Project is a "parent" class of
         *                          #project#ComponentInstance).
         *
         * @return The count of replaced variables in the text
         */
        int replaceVariablesWithAttributes(QString& rawText, bool passToParents) const noexcept;

        /**
         * @brief Get the value of an attribute which can be used in texts (like "${NAME}")
         *
         * @param attrKey           The attribute name (e.g. "FOOBAR" in "${FOOBAR}").
         * @param passToParents     See #replaceVariablesWithAttributes()
         * @param value             The value of the specified attribute will be written
         *                          into this variable, but only if the attribute was
         *                          found, otherwise the variable won't be modified.
         *
         * @return true if the attribute was found, false if not
         */
        virtual bool getAttributeValue(const QString& attrKey, bool passToParents,
                                       QString& value) const noexcept = 0;


    signals:

        /**
         * @brief This signal is emited when the value of attributes has changed
         *
         * All derived classes must emit this signal when some attributes have changed
         * their values (only attributes which can be fetched with #getAttributeValue(),
         * inclusive all attributes from all "parent" classes).
         */
        virtual void attributesChanged() = 0;


    private:

        // make some methods inaccessible...
        AttributeProvider(const AttributeProvider& other);
        AttributeProvider& operator=(const AttributeProvider& rhs);


        // Private Methods

        /**
         * @brief Search the next variable ("${KEY}") in a given text
         *
         * @param text      A text which can contain variables
         * @param startPos  The search start index (use 0 to search in the whole text)
         * @param pos       The index of the next variable in the specified text (index of
         *                  the "$" character, or -1 if no variable found) will be written
         *                  into this variable.
         * @param length    If a variable is found, the length (incl. "${" and "}") will
         *                  be written into this variable.
         * @param varName   The variable name (text between "{" and "}"), e.g. "FOOBAR"
         *                  for the example above.
         *
         * @return          true if a variable is found, false if not
         */
        static bool searchVariableInText(const QString& text, int startPos, int& pos,
                                         int& length, QString& varName) noexcept;
};

// Make sure that the AttributeProvider class does not contain any data (except the vptr).
// Otherwise it could introduce issues when using multiple inheritance.
static_assert(sizeof(AttributeProvider) == sizeof(void*),
              "AttributeProvider must not contain any data!");

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ATTRIBUTEPROVIDER_H
