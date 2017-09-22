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

#ifndef LIBREPCB_ATTRIBUTESUBSTITUTOR_H
#define LIBREPCB_ATTRIBUTESUBSTITUTOR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class AttributeProvider;

/*****************************************************************************************
 *  Class AttributeSubstitutor
 ****************************************************************************************/

/**
 * @brief The AttributeSubstitutor class substitutes attribute keys in strings with their
 *        actual values (e.g. replace "#NAME" by "U42", a component's name)
 *
 * Please read the documentation about the @ref doc_attributes_system to get an idea how
 * the @ref doc_attributes_system works in detail.
 *
 * @see librepcb::AttributeProvider
 * @see @ref doc_attributes_system
 *
 * @author ubruhin
 * @date 2017-09-19
 *
 * @todo Fix side-effect of the endless loop detection ("#FOO #FOO" is currently
 *       substituted by "#FOO " because of the endless loop detection, even if there is
 *       actually no endless loop).
 * @todo Properly implement multiple key substitution ("#FOO|BAR" is currently substituted
 *       by "#FOO", even if the attribute #FOO indirectly evaluates to an empty string).
 */
class AttributeSubstitutor final
{
    public:

        // Constructors / Destructor / Operator Overloadings
        AttributeSubstitutor() = delete;
        AttributeSubstitutor(const AttributeSubstitutor& other) = delete;
        AttributeSubstitutor& operator=(const AttributeSubstitutor& rhs) = delete;
        ~AttributeSubstitutor() = delete;


        // General Methods

        /**
         * @brief Substitute all attribute keys in a string with their attribute values
         *
         * In addition to attributes substitution, this method also performs escaping of
         * the '#' character.
         *
         * @param str       A string which can contain variables ("#NAME"). The attributes
         *                  will be substituted directly in this string.
         *
         * @return True if str was modified in some way, false if not
         */
        static QString substitute(QString str, const AttributeProvider* ap = nullptr) noexcept;


    private: // Methods

        /**
         * @brief Search the next variables ("#KEY|FALLBACK") in a given text
         *
         * @param text      A text which can contain variables
         * @param startPos  The search start index (use 0 to search in the whole text)
         * @param pos       The index of the next variable in the specified text (index of
         *                  the '#' character, or -1 if no variable found) will be written
         *                  into this variable.
         * @param length    If a variable is found, the length (incl. '#') will be written
         *                  into this variable.
         * @param keys      The variable key names (text after '#', split by '|')
         *
         * @return          true if a variable is found, false if not
         */
        static bool searchVariablesInText(const QString& text, int startPos, int& pos,
                                         int& length, QStringList& keys) noexcept;

        static bool isEscapedSharp(const QString& text, int startPos) noexcept;
        static int getLengthOfKeys(const QString& text, int startPos) noexcept;
        static bool getValueOfKey(const QString& key, QString& value,
                                  const AttributeProvider* ap) noexcept;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ATTRIBUTESUBSTITUTOR_H
