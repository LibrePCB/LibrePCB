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

#ifndef IF_ATTRIBUTEPROVIDER_H
#define IF_ATTRIBUTEPROVIDER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Interface IF_AttributeProvider
 ****************************************************************************************/

/**
 * @brief The IF_AttributeProvider class defines an interface for classes which provides
 *        some attributes which can be used as variables in texts (like "${NS::KEY}")
 *
 * For example library symbols can contain text elements which contains variables, for
 * example the most importants texts "${SYM::NAME}" and "${CMP::VALUE}". All these
 * variables will be parsed and replaced with their values when such a text is displayed
 * in a schematic of a project.
 *
 * The main goal of this interface is to provide the method #replaceVariablesWithAttributes()
 * which will replace all variables in a text with their values. To get the values from
 * attributes, the pure virtual method #getAttributeValue() have to be implemented in all
 * classes which inherit from #IF_AttributeProvider.
 *
 * To resolve a variable like "${CMP::NAME}", the class project#GenCompInstance must
 * inherit from this interface class. The method #getAttributeValue() must be implemented
 * and should return the name of the generic component instance (like "U123") when the
 * attribute "${CMP::NAME}" was requested.
 *
 * @author ubruhin
 * @date 2015-01-10
 */
class IF_AttributeProvider
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default Constructor
         */
        IF_AttributeProvider() {}

        /**
         * @brief Destructor
         */
        virtual ~IF_AttributeProvider() {}


        // General Methods

        /**
         * @brief Replace all variables in a text with their attribute values
         *
         * This method fetches all attribute values with #getAttributeValue() for all
         * variables found in the given text and replaces them in the text.
         *
         * @param rawText           A text which can contain variables ("${NS::KEY}"). The
         *                          variables will be replaced directly in this QString.
         * @param passToParents     If true, the method #getAttributeValue() may also
         *                          call the same method of a "parent attribute provider"
         *                          class to fetch the requested attribute value (for
         *                          example project#Project is a "parent" class of
         *                          project#GenCompInstance).
         *
         * @return The count of replaced variables in the text
         */
        uint replaceVariablesWithAttributes(QString& rawText, bool passToParents) const noexcept;

        /**
         * @brief Get the value of an attribute which can be used in texts (like "${CMP::NAME}")
         *
         * @param attrNS            The attribute namespace (e.g. "HELLO" in "${HELLO::WORLD}").
         * @param attrKey           The attribute name (e.g. "WORLD" in "${HELLO::WORLD}").
         * @param passToParents     See #replaceVariablesWithAttributes()
         * @param value             The value of the specified attribute will be written
         *                          into this variable, but only if the attribute was
         *                          found, otherwise the variable won't be modified.
         *
         * @return true if the attribute was found, false if not
         */
        virtual bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                                       bool passToParents, QString& value) const noexcept = 0;


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
        IF_AttributeProvider(const IF_AttributeProvider& other);
        IF_AttributeProvider& operator=(const IF_AttributeProvider& rhs);


        // Private Methods

        /**
         * @brief Search the next variable ("${NS::KEY}") in a given text
         *
         * @param text      A text which can contain variables
         * @param startPos  The search start index (use 0 to search in the whole text)
         * @param pos       The index of the next variable in the specified text (index of
         *                  the "$" character, or -1 if no variable found) will be written
         *                  into this variable.
         * @param length    If a variable is found, the length (incl. "${" and "}") will
         *                  be written into this variable.
         * @param varNS     The variable namespace (text between "${" and "::"), for
         *                  example "HELLO" if the text was "12${HELLO::WORLD}34".
         *                  If no namespace is used (e.g. "12${WORLD}34"), this variable
         *                  will be empty.
         * @param varName   The variable name (text between "::" and "}"), e.g. "WORLD"
         *                  for the example above.
         *
         * @return          true if a variable is found, false if not
         */
        static bool searchVariableInText(const QString& text, int startPos, int& pos,
                                         int& length, QString& varNS, QString& varName) noexcept;
};

#endif // IF_ATTRIBUTEPROVIDER_H
