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

#ifndef LIBREPCB_TEXT_H
#define LIBREPCB_TEXT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../units/all_length_units.h"
#include "../alignment.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Interface IF_TextObserver
 ****************************************************************************************/

/**
 * @brief The IF_TextObserver class
 *
 * @author ubruhin
 * @date 2017-01-02
 */
class IF_TextObserver
{
    public:
        virtual void textLayerNameChanged(const QString& newLayerName) noexcept = 0;
        virtual void textTextChanged(const QString& newText) noexcept = 0;
        virtual void textPositionChanged(const Point& newPos) noexcept = 0;
        virtual void textRotationChanged(const Angle& newRot) noexcept = 0;
        virtual void textHeightChanged(const PositiveLength& newHeight) noexcept = 0;
        virtual void textAlignChanged(const Alignment& newAlign) noexcept = 0;

    protected:
        IF_TextObserver() noexcept {}
        explicit IF_TextObserver(const IF_TextObserver& other) = delete;
        virtual ~IF_TextObserver() noexcept {}
        IF_TextObserver& operator=(const IF_TextObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class Text
 ****************************************************************************************/

/**
 * @brief The Text class
 */
class Text final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Text)

    public:

        // Constructors / Destructor
        Text() = delete;
        Text(const Text& other) noexcept;
        Text(const Uuid& uuid, const Text& other) noexcept;
        Text(const Uuid& uuid, const QString& layerName, const QString& text,
             const Point& pos, const Angle& rotation, const PositiveLength& height,
             const Alignment& align) noexcept;
        explicit Text(const SExpression& node);
        ~Text() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getLayerName() const noexcept {return mLayerName;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const PositiveLength& getHeight() const noexcept {return mHeight;}
        const Alignment& getAlign() const noexcept {return mAlign;}
        const QString& getText() const noexcept {return mText;}

        // Setters
        void setLayerName(const QString& name) noexcept;
        void setText(const QString& text) noexcept;
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rotation) noexcept;
        void setHeight(const PositiveLength& height) noexcept;
        void setAlign(const Alignment& align) noexcept;

        // General Methods
        void registerObserver(IF_TextObserver& object) const noexcept;
        void unregisterObserver(IF_TextObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Text& rhs) const noexcept;
        bool operator!=(const Text& rhs) const noexcept {return !(*this == rhs);}
        Text& operator=(const Text& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        QString mLayerName;
        QString mText;
        Point mPosition;
        Angle mRotation;
        PositiveLength mHeight;
        Alignment mAlign;

        // Misc
        mutable QSet<IF_TextObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Class TextList
 ****************************************************************************************/

struct TextListNameProvider {static constexpr const char* tagname = "text";};
using TextList = SerializableObjectList<Text, TextListNameProvider>;
using CmdTextInsert = CmdListElementInsert<Text, TextListNameProvider>;
using CmdTextRemove = CmdListElementRemove<Text, TextListNameProvider>;
using CmdTextsSwap = CmdListElementsSwap<Text, TextListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_TEXT_H
