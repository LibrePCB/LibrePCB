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

#ifndef LIBREPCB_SERIALIZABLEKEYVALUEMAP_H
#define LIBREPCB_SERIALIZABLEKEYVALUEMAP_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "serializableobject.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class SerializableKeyValueMap
 ****************************************************************************************/

/**
 * @brief The SerializableKeyValueMap class provides an easy way to serialize and
 *        deserialize ordered key value pairs
 *
 * For example if you have DOM nodes with strings in different languages, this class helps
 * you to read these strings. This class will also check if an entry with an empty key
 * (this is used to specify the default value) exists in the DOM node.
 *
 * Example of a valid DOM node:
 * @code
 * <my_parent_node>
 *     <my_subnode my_key="">hello</my_subnode>
 *     <my_subnode my_key="foo">foobar</my_subnode>
 * </my_parent_node>
 * @endcode
 */
template <typename T>
class SerializableKeyValueMap final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(SerializableKeyValueMap)

    public:

        // Constructors / Destructor
        SerializableKeyValueMap() noexcept {}
        SerializableKeyValueMap(const SerializableKeyValueMap<T>& other) noexcept :
            mValues(other.mValues) {}
        explicit SerializableKeyValueMap(const DomElement& domElement) {
            loadFromDomElement(domElement); // can throw
        }
        ~SerializableKeyValueMap() noexcept {}

        // Getters
        QStringList keys() const noexcept {
            return mValues.keys();
        }
        QString getDefaultValue() const noexcept {
            return mValues.value("");
        }
        bool contains(const QString& key) const noexcept {
            return mValues.contains(key);
        }
        QString value(const QString& key) const noexcept {
            return mValues.value(key);
        }
        QString value(const QStringList& keyOrder, QString* usedKey = nullptr) const noexcept {
            // search in the specified key order
            foreach (const QString& key, keyOrder) {
                if (mValues.contains(key)) {
                    if (usedKey) *usedKey = key;
                    return mValues.value(key);
                }
            }
            // use empty key as fallback
            if (usedKey) *usedKey = "";
            return mValues.value("");
        }

        // General Methods

        void setDefaultValue(const QString& value) noexcept {
            insert("", value);
        }

        void insert(const QString& key, const QString& value) noexcept {
            mValues.insert(key, value);
        }

        void clear() noexcept {
            mValues.clear();
        }

        void loadFromDomElement(const DomElement& domElement) {
            mValues.clear();
            for (const DomElement* child = domElement.getFirstChild(T::tagname, true);
                 child; child = child->getNextSibling(T::tagname))
            {
                QString key = child->getAttribute<QString>(T::keyname, false);
                QString value = child->getText<QString>(false);
                if (mValues.contains(key)) {
                    throw RuntimeError(__FILE__, __LINE__, QString(),
                        QString(tr("Key \"%1\" defined multiple times in \"%2\"."))
                        .arg(key, domElement.getDocFilePath().toNative()));
                }
                mValues.insert(key, value);
            }
            if (!mValues.contains(QString(""))) {
                throw RuntimeError(__FILE__, __LINE__, QString(),
                    QString(tr("The file \"%1\" has no default %2 defined."))
                    .arg(domElement.getDocFilePath().toNative(), T::tagname));
            }
        }

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override {
            foreach (const QString& key, mValues.keys()) {
                DomElement* child = root.appendChild(T::tagname);
                child->setAttribute(T::keyname, key);
                child->setText(mValues.value(key));
            }
        }

        // Operator Overloadings
        SerializableKeyValueMap<T>& operator=(const SerializableKeyValueMap<T>& rhs) noexcept {
            mValues = rhs.mValues;
            return *this;
        }
        bool operator==(const SerializableKeyValueMap<T>& rhs) const noexcept {
            return mValues == rhs.mValues;
        }
        bool operator!=(const SerializableKeyValueMap<T>& rhs) const noexcept {
            return mValues != rhs.mValues;
        }


    private: // Data
        QMap<QString, QString> mValues;
};

/*****************************************************************************************
 *  Class LocalizedNameMap
 ****************************************************************************************/

struct LocalizedNameMapConstants {
    static constexpr const char* tagname = "name";
    static constexpr const char* keyname = "locale";
};
using LocalizedNameMap = SerializableKeyValueMap<LocalizedNameMapConstants>;

/*****************************************************************************************
 *  Class LocalizedDescriptionMap
 ****************************************************************************************/

struct LocalizedDescriptionMapConstants {
    static constexpr const char* tagname = "description";
    static constexpr const char* keyname = "locale";
};
using LocalizedDescriptionMap = SerializableKeyValueMap<LocalizedDescriptionMapConstants>;

/*****************************************************************************************
 *  Class LocalizedKeywordsMap
 ****************************************************************************************/

struct LocalizedKeywordsMapConstants {
    static constexpr const char* tagname = "keywords";
    static constexpr const char* keyname = "locale";
};
using LocalizedKeywordsMap = SerializableKeyValueMap<LocalizedKeywordsMapConstants>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_SERIALIZABLEKEYVALUEMAP_H

