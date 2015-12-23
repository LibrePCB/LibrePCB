/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef XMLDOMELEMENT_H
#define XMLDOMELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomElement>
#include "../exceptions.h"
#include "filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomDocument;

/*****************************************************************************************
 *  Class XmlDomElement
 ****************************************************************************************/

/**
 * @brief The XmlDomElement class represents one element in a XML DOM tree
 *
 * Each #XmlDomElement represents either a text element or an element with childs.
 * Example:
 * @code{.xml}
 * <root_element>                   <!-- element with childs (cannot include text) -->
 *     <child>                      <!-- element with childs (cannot include text) -->
 *         <text>Some Text</text>   <!-- text element (cannot contain childs) -->
 *     </child>
 *     <empty_child></empty_child>  <!-- could be a text element or an element with childs -->
 * </root_element>
 * @endcode
 *
 * @note This class provides some template methods to call them with different types.
 *       We don't use method overloading because this way we don't need to include
 *       the header files for our own types (e.g. #Uuid, #Version, ...).
 *
 * @todo Use libxml2 instead of Qt's DOM classes.
 * @todo Add more template instances (provide more type conversions from/to QString)
 *
 * @author ubruhin
 * @date 2015-02-01
 */
class XmlDomElement final
{
        Q_DECLARE_TR_FUNCTIONS(XmlDomElement)

    public:

        // Constructors / Destructor

        /**
         * @brief Constructor to create a new DOM element
         *
         * @param name      The tag name of the element
         * @param text      The text if a text element should be created
         */
        explicit XmlDomElement(const QString& name, const QString& text = QString()) noexcept;

        /**
         * @brief Destructor (destroys all child elements)
         */
        ~XmlDomElement() noexcept;


        // General Methods

        /**
         * @brief Get the DOM document of this DOM element
         *
         * @param docOfTree     If true and this element is not the root element, this
         *                      method will try to return the document of the whole tree.
         *
         * @retval XmlDomDocument*  A pointer to the DOM document of this DOM element.
         * @retval nullptr          If "docOfTree == false" and this element has no
         *                          document or is not the root element in the document.
         * @retval nullptr          If the whole DOM tree of this element has no document.
         */
        XmlDomDocument* getDocument(bool docOfTree) const noexcept;

        /**
         * @brief Set the document of this element
         *
         * @warning This method should be called only from class #XmlDomDocument!
         *
         * @param doc   The document or nullptr
         */
        void setDocument(XmlDomDocument* doc) noexcept;

        /**
         * @brief Get the filepath of the DOM documents XML file (if available)
         *
         * @return The filepath of the documents XML file (invalid if no document available)
         *
         * @note If no document is available or the document is not saved to disc (newly
         *       created document), this method will return an invalid #FilePath object!
         */
        FilePath getDocFilePath() const noexcept;

        /**
         * @brief Get the parent element
         *
         * @retval XmlDomElement*   Pointer to the parent element
         * @retval nullptr          If this element has no parent
         */
        XmlDomElement* getParent() const noexcept {return mParent;}

        /**
         * @brief Get the tag name of this element
         *
         * @return The tag name
         */
        const QString& getName() const noexcept {return mName;}

        /**
         * @brief Set the tag name of this element
         *
         * @param name  The new name (see #isValidXmlTagName() for allowed characters)
         */
        void setName(const QString& name) noexcept {Q_ASSERT(isValidXmlTagName(name)); mName = name;}


        // Text Handling Methods

        /**
         * @brief Set the text of this text element
         *
         * @tparam T    The value of this type will be converted to a QString.
         *              Available types:
         *              bool, QString, QDateTime, #Uuid, #Version, #Length (tbc)
         *
         * @warning This method must be called only on elements without child elements!
         *
         * @param value         The value in the template type T
         */
        template <typename T>
        void setText(const T& value) noexcept;

        /**
         * @brief Get the text of this text element in the specified type
         *
         * @tparam T    The text will be converted to this type. Available types:
         *              bool, QString, QDateTime, #Uuid, #Version, #Length (tbc)
         *
         * @param throwIfEmpty  If true and the text is empty, an exception will be thrown.
         *                      If false and the text is empty, defaultValue will be returned.
         *
         * @retval T            The text of this text element in the template type T
         * @retval defaultValue If the text is empty and "throwIfEmpty == false"
         *
         * @throw Exception     If this is not a text element
         * @throw Exception     If converting the text into the type T was not successful
         * @throw Exception     If the text is empty and "throwIfEmpty == true"
         */
        template <typename T>
        T getText(bool throwIfEmpty, const T& defaultValue = T()) const throw (Exception);


        // Attribute Handling Methods

        /**
         * @brief Set or add an attribute to this element
         *
         * @tparam T        The text will be converted in this type. Available types:
         *                  bool, const char*, QString, #Uuid, #LengthUnit, #Length,
         *                  #Angle, #HAlign, #VAlign (tbc)
         *
         * @param name      The tag name (see #isValidXmlTagName() for allowed characters)
         * @param value     The attribute value
         */
        template <typename T>
        void setAttribute(const QString& name, const T& value) noexcept;

        /**
         * @brief Check whether this element has a specific attribute or not
         *
         * @param name  The tag name (see #isValidXmlTagName() for allowed characters)
         *
         * @retval true     If the attribute exists
         * @retval false    If the attribute does not exist
         */
        bool hasAttribute(const QString& name) const noexcept;

        /**
         * @brief Get the value of a specific attribute in the specified type
         *
         * @tparam T    The value will be converted in this type. Available types:
         *              bool, uint, int, QString, #Uuid, #LengthUnit, #Length, #Angle,
         *              #HAlign, #VAlign (tbc)
         *
         * @param name          The tag name (see #isValidXmlTagName() for allowed characters)
         * @param throwIfEmpty  If true and the value is empty, an exception will be thrown
         *                      If false and the value is empty, defaultValue will be returned.
         *
         * @retval T            The value of this text element in the template type T
         * @retval defaultValue If the value is empty and "throwIfEmpty == false"
         *
         * @throw Exception     If the specified attribute does not exist
         * @throw Exception     If converting the value into the type T was not successful
         * @throw Exception     If the value is empty and "throwIfEmpty == true"
         */
        template <typename T>
        T getAttribute(const QString& name, bool throwIfEmpty, const T& defaultValue = T()) const throw (Exception);


        // Child Handling Methods

        /**
         * @brief Check whether this element has childs or not
         *
         * @retval true     If this element has child elements
         * @retval false    If this element has no child elements
         */
        bool hasChilds() const noexcept {return !mChilds.isEmpty();}

        /**
         * @brief Get the child count of this element
         *
         * @return  The count of child elements
         */
        int getChildCount() const noexcept {return mChilds.count();}

        /**
         * @brief Remove a child element from the DOM tree
         *
         * @param child         The child to remove
         * @param deleteChild   If true, this method will also delete the child object
         */
        void removeChild(XmlDomElement* child, bool deleteChild) noexcept;

        /**
         * @brief Append a child to the end of the child list of this element
         *
         * @param child     The child to append
         */
        void appendChild(XmlDomElement* child) noexcept;

        /**
         * @brief Create and append the new child to the end of the child list of this element
         *
         * @param name  The tag name of the element to create and append
         *
         * @return The created and appended child element
         */
        XmlDomElement* appendChild(const QString& name) noexcept;

        /**
         * @brief Create a new text child and append it to the list of childs
         *
         * @tparam T        This type will be converted to the text string. Available types:
         *                  bool, QString, QDateTime, #Uuid, #Version (tbc)
         *
         * @param name      The tag name (see #isValidXmlTagName() for allowed characters)
         * @param value     The attribute value which will be converted to a QString
         */
        template <typename T>
        XmlDomElement* appendTextChild(const QString& name, const T& value) noexcept;

        /**
         * @brief Get the first child element of this element
         *
         * @param throwIfNotFound   If true and this element has no childs, an exception
         *                          will be thrown
         *
         * @retval XmlDomElement*   The first child element
         * @retval nullptr          If there is no child and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no child element
         */
        XmlDomElement* getFirstChild(bool throwIfNotFound = false) const throw (Exception);

        /**
         * @brief Get the first child element with a specific name
         *
         * @param name              The tag name of the child to search
         * @param throwIfNotFound   If true and this element has no childs with the
         *                          specified name, an exception will be thrown
         *
         * @retval XmlDomElement*   The first child element with the specified name
         * @retval nullptr          If there is no such child and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no such child element
         */
        XmlDomElement* getFirstChild(const QString& name, bool throwIfNotFound) const throw (Exception);

        /**
         * @brief Get the first child element with a specific path/name (recursive)
         *
         * This method is very useful to search a child in a DOM tree recursively.
         *
         * Example:
         * @code{.cpp}
         * // "root" contains the root node of a DOM document (or any other node in a DOM tree)
         * XmlDomElement* root = doc.getRoot();
         * // get the text of the first "category" child of the element "meta/categories".
         * // the return value of getFirstChild() is always a valid pointer as the method
         * // would throw an exception of the specified path or child does not exist.
         * QString value1 = root->getFirstChild("meta/categories/category", true, true)->getText();
         * // the next line does the same, but the character "*" defines that the first
         * // child of "meta/categories" is returned, no matter what tag name it has.
         * // Note: the white space between '/' and '*' is added to avoid compiler warnings
         * // ("...within comment") and must not be added when using this method.
         * QString value2 = root->getFirstChild("meta/categories/ *", true, true)->getText();
         * @endcode
         *
         * @param pathName              The path + name to the child to search. As child
         *                              name you can use "*" to specify that any child
         *                              name is allowed.
         * @param throwIfPathNotExist   If true and the specified path (the left part of
         *                              the last slash in pathName) does not exist
         * @param throwIfChildNotFound  If true and the specified child (the right part of
         *                              the last slash in pathName) does not exist
         *
         * @retval XmlDomElement*   The first child element with the specified path/name
         * @retval nullptr          If there is no such path and "throwIfPathNotExist == false"
         * @retval nullptr          If there is no such child and "throwIfChildNotFound == false"
         *
         * @throw Exception If "throwIfPathNotExist == true" and the path does not exist
         * @throw Exception If "throwIfChildNotFound == true" and the child does not exist
         */
        XmlDomElement* getFirstChild(const QString& pathName, bool throwIfPathNotExist,
                                     bool throwIfChildNotFound) const throw (Exception);

        /**
         * @brief Get the previous child (with a specific name) of a specific child
         *
         * @param child             The specified child
         * @param name              The name of the previous child to search. Use a NULL
         *                          QString (QString()) if the child name is not relevant.
         * @param throwIfNotFound   If true and there is no such previous child, an
         *                          exception will be thrown
         *
         * @retval XmlDomElement*   The previous child (with the specified name)
         * @retval nullptr          If there is no such child and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no such previous child
         */
        XmlDomElement* getPreviousChild(const XmlDomElement* child, const QString& name = QString(),
                                        bool throwIfNotFound = false) const throw (Exception);

        /**
         * @brief Get the next child (with a specific name) of a specific child
         *
         * @param child             The specified child
         * @param name              The name of the next child to search. Use a NULL
         *                          QString (QString()) if the child name is not relevant.
         * @param throwIfNotFound   If true and there is no such next child, an
         *                          exception will be thrown
         *
         * @retval XmlDomElement*   The next child (with the specified name)
         * @retval nullptr          If there is no such child and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no such next child
         */
        XmlDomElement* getNextChild(const XmlDomElement* child, const QString& name = QString(),
                                    bool throwIfNotFound = false) const throw (Exception);


        // Sibling Handling Methods

        /**
         * @brief Get the previous sibling element (with a specific name)
         *
         * @param name              The name of the previous sibling to search. Use a NULL
         *                          QString (QString()) if the child name is not relevant.
         * @param throwIfNotFound   If true and there is no such previous sibling, an
         *                          exception will be thrown
         *
         * @retval XmlDomElement*   The previous sibling element (with the specified name)
         * @retval nullptr          If there is no such sibling and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no such previous sibling
         */
        XmlDomElement* getPreviousSibling(const QString& name = QString(),
                                          bool throwIfNotFound = false) const throw (Exception);

        /**
         * @brief Get the next sibling element (with a specific name)
         *
         * @param name              The name of the next sibling to search. Use a NULL
         *                          QString (QString()) if the child name is not relevant.
         * @param throwIfNotFound   If true and there is no such next sibling, an
         *                          exception will be thrown
         *
         * @retval XmlDomElement*   The next sibling element (with the specified name)
         * @retval nullptr          If there is no such sibling and "throwIfNotFound == false"
         *
         * @throw Exception If "throwIfNotFound == true" and there is no such next sibling
         */
        XmlDomElement* getNextSibling(const QString& name = QString(),
                                      bool throwIfNotFound = false) const throw (Exception);


        // QDomElement Converter Methods

        /**
         * @brief Construct a QDomElement object from this XmlDomElement (recursively)
         *
         * @param domDocument   The DOM Document of the newly created QDomElement
         *
         * @return The created QDomElement (which is added to the specified DOM document)
         */
        QDomElement toQDomElement(QDomDocument& domDocument) const noexcept;

        /**
         * @brief Construct a XmlDomElement object from a QDomElement object (recursively)
         *
         * @param domElement    The QDomElement to copy
         * @param doc           The DOM Document of the newly created XmlDomElement (only
         *                      needed for the root element)
         *
         * @return The created XmlDomElement (the caller takes the ownership!)
         */
        static XmlDomElement* fromQDomElement(QDomElement domElement, XmlDomDocument* doc = nullptr) noexcept;


    private:

        // make some methods inaccessible...
        XmlDomElement() = delete;
        XmlDomElement(const XmlDomElement& other) = delete;
        XmlDomElement& operator=(const XmlDomElement& rhs) = delete;


        // Private Methods

        /**
         * @brief Private constructor to create a XmlDomElement from a QDomElement
         *
         * @param domElement    The QDomElement to copy
         * @param parent        The parent of the newly created XmlDomElement
         * @param doc           The DOM Document of the newly created XmlDomElement (only
         *                      needed for the root element)
         */
        explicit XmlDomElement(QDomElement domElement, XmlDomElement* parent = nullptr,
                               XmlDomDocument* doc = nullptr) noexcept;

        /**
         * @brief Check if a QString represents a valid XML tag name for elements and attributes
         *
         * Valid characters:
         *  - a-z
         *  - A-Z
         *  - _ (underscore)
         *
         * @param name  The tag name to check
         *
         * @retval true     If valid
         * @retval false    If invalid
         */
        static bool isValidXmlTagName(const QString& name) noexcept;


        // Attributes
        XmlDomDocument* mDocument;  ///< the DOM document of the tree (only needed in the root node, otherwise nullptr)
        XmlDomElement* mParent;     ///< the parent element (if available, otherwise nullptr)
        QString mName;              ///< the tag name of this element
        QString mText;              ///< the text of this element (only if there are no childs)
        QList<XmlDomElement*> mChilds;      ///< all child elements (only if there is no text)
        QHash<QString, QString> mAttributes;///< all attributes of this element (key, value) in arbitrary order
};

#endif // XMLDOMELEMENT_H
