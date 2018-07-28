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

#ifndef LIBREPCB_STROKETEXT_H
#define LIBREPCB_STROKETEXT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../geometry/path.h"
#include "../graphics/graphicslayername.h"
#include "../units/all_length_units.h"
#include "../alignment.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class AttributeProvider;
class StrokeFont;

/*****************************************************************************************
 *  Class StrokeTextSpacing
 ****************************************************************************************/

/**
 * @brief The StrokeTextSpacing class
 */
class StrokeTextSpacing final
{
        Q_DECLARE_TR_FUNCTIONS(StrokeTextSpacing)

    public:

        // Constructors / Destructor
        StrokeTextSpacing() noexcept :
            mAuto(true), mRatio(Ratio::percent100()) {}
        StrokeTextSpacing(const StrokeTextSpacing& other) noexcept :
            mAuto(other.mAuto), mRatio(other.mRatio) {}
        explicit StrokeTextSpacing(const Ratio& ratio) noexcept :
            mAuto(false), mRatio(ratio) {}
        ~StrokeTextSpacing() noexcept {}

        // General Methods
        bool isAuto() const noexcept {return mAuto;}
        const Ratio& getRatio() const noexcept {return mRatio;}

        // Operator Overloadings
        bool operator==(const StrokeTextSpacing& rhs) const noexcept {
            if (mAuto != rhs.mAuto) return false;
            return mAuto ? true : (mRatio == rhs.mRatio);
        }
        bool operator!=(const StrokeTextSpacing& rhs) const noexcept {return !(*this == rhs);}
        StrokeTextSpacing& operator=(const StrokeTextSpacing& rhs) noexcept {
            mAuto = rhs.mAuto;
            mRatio = rhs.mRatio;
            return *this;
        }


    private: // Data
        bool mAuto;
        Ratio mRatio;
};

/*****************************************************************************************
 *  Non-Member Functions
 ****************************************************************************************/

template <>
inline SExpression serializeToSExpression(const StrokeTextSpacing& obj) {
    if (obj.isAuto()) {
        return SExpression::createToken("auto");
    } else {
        return serializeToSExpression(obj.getRatio());
    }
}

template <>
inline StrokeTextSpacing deserializeFromSExpression(const SExpression& sexpr, bool throwIfEmpty) {
    QString str = sexpr.getStringOrToken(throwIfEmpty);
    if (str == "auto") {
        return StrokeTextSpacing();
    } else {
        return StrokeTextSpacing(deserializeFromSExpression<Ratio>(sexpr, throwIfEmpty)); // can throw
    }
}

/*****************************************************************************************
 *  Interface IF_StrokeTextObserver
 ****************************************************************************************/

/**
 * @brief The IF_StrokeTextObserver class
 */
class IF_StrokeTextObserver
{
    public:
        virtual void strokeTextLayerNameChanged(const GraphicsLayerName& newLayerName) noexcept = 0;
        virtual void strokeTextTextChanged(const QString& newText) noexcept = 0;
        virtual void strokeTextPositionChanged(const Point& newPos) noexcept = 0;
        virtual void strokeTextRotationChanged(const Angle& newRot) noexcept = 0;
        virtual void strokeTextHeightChanged(const PositiveLength& newHeight) noexcept = 0;
        virtual void strokeTextStrokeWidthChanged(const UnsignedLength& newWidth) noexcept = 0;
        virtual void strokeTextLetterSpacingChanged(const StrokeTextSpacing& spacing) noexcept = 0;
        virtual void strokeTextLineSpacingChanged(const StrokeTextSpacing& spacing) noexcept = 0;
        virtual void strokeTextAlignChanged(const Alignment& newAlign) noexcept = 0;
        virtual void strokeTextMirroredChanged(bool newMirrored) noexcept = 0;
        virtual void strokeTextAutoRotateChanged(bool newAutoRotate) noexcept = 0;
        virtual void strokeTextPathsChanged(const QVector<Path>& paths) noexcept = 0;

    protected:
        IF_StrokeTextObserver() noexcept {}
        explicit IF_StrokeTextObserver(const IF_StrokeTextObserver& other) = delete;
        virtual ~IF_StrokeTextObserver() noexcept {}
        IF_StrokeTextObserver& operator=(const IF_StrokeTextObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class StrokeText
 ****************************************************************************************/

/**
 * @brief The StrokeText class
 */
class StrokeText final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(StrokeText)

    public:

        // Constructors / Destructor
        StrokeText() = delete;
        StrokeText(const StrokeText& other) noexcept;
        StrokeText(const Uuid& uuid, const StrokeText& other) noexcept;
        StrokeText(const Uuid& uuid, const GraphicsLayerName& layerName, const QString& text,
                   const Point& pos, const Angle& rotation, const PositiveLength& height,
                   const UnsignedLength& strokeWidth, const StrokeTextSpacing& letterSpacing,
                   const StrokeTextSpacing& lineSpacing, const Alignment& align,
                   bool mirrored, bool autoRotate) noexcept;
        explicit StrokeText(const SExpression& node);
        ~StrokeText() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const GraphicsLayerName& getLayerName() const noexcept {return mLayerName;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const PositiveLength& getHeight() const noexcept {return mHeight;}
        const UnsignedLength& getStrokeWidth() const noexcept {return mStrokeWidth;}
        const StrokeTextSpacing& getLetterSpacing() const noexcept {return mLetterSpacing;}
        const StrokeTextSpacing& getLineSpacing() const noexcept {return mLineSpacing;}
        const Alignment& getAlign() const noexcept {return mAlign;}
        bool getMirrored() const noexcept {return mMirrored;}
        bool getAutoRotate() const noexcept {return mAutoRotate;}
        const QString& getText() const noexcept {return mText;}
        const QVector<Path>& getPaths() const noexcept;
        bool needsAutoRotation() const noexcept;
        Length calcLetterSpacing() const noexcept;
        Length calcLineSpacing() const noexcept;

        // Setters
        void setLayerName(const GraphicsLayerName& name) noexcept;
        void setText(const QString& text) noexcept;
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rotation) noexcept;
        void setHeight(const PositiveLength& height) noexcept;
        void setStrokeWidth(const UnsignedLength& strokeWidth) noexcept;
        void setLetterSpacing(const StrokeTextSpacing& spacing) noexcept;
        void setLineSpacing(const StrokeTextSpacing& spacing) noexcept;
        void setAlign(const Alignment& align) noexcept;
        void setMirrored(bool mirrored) noexcept;
        void setAutoRotate(bool autoRotate) noexcept;

        // General Methods
        void setAttributeProvider(const AttributeProvider* provider) noexcept;
        void setFont(const StrokeFont* font) noexcept;
        const StrokeFont* getCurrentFont() const noexcept {return mFont;}
        void updatePaths() noexcept;
        void registerObserver(IF_StrokeTextObserver& object) const noexcept;
        void unregisterObserver(IF_StrokeTextObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const StrokeText& rhs) const noexcept;
        bool operator!=(const StrokeText& rhs) const noexcept {return !(*this == rhs);}
        StrokeText& operator=(const StrokeText& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        GraphicsLayerName mLayerName;
        QString mText;
        Point mPosition;
        Angle mRotation;
        PositiveLength mHeight;
        UnsignedLength mStrokeWidth;
        StrokeTextSpacing mLetterSpacing;
        StrokeTextSpacing mLineSpacing;
        Alignment mAlign;
        bool mMirrored;
        bool mAutoRotate;

        // Misc
        mutable QSet<IF_StrokeTextObserver*> mObservers; ///< A list of all observer objects
        const AttributeProvider* mAttributeProvider; ///< for substituting placeholders in text
        const StrokeFont* mFont; ///< font used for calculating paths
        QVector<Path> mPaths; ///< stroke paths without transformations (mirror/rotate/translate)
        QVector<Path> mPathsRotated; ///< same as #mPaths, but rotated by 180°
};

/*****************************************************************************************
 *  Class StrokeTextList
 ****************************************************************************************/

struct StrokeTextListNameProvider {static constexpr const char* tagname = "stroke_text";};
using StrokeTextList = SerializableObjectList<StrokeText, StrokeTextListNameProvider>;
using CmdStrokeTextInsert = CmdListElementInsert<StrokeText, StrokeTextListNameProvider>;
using CmdStrokeTextRemove = CmdListElementRemove<StrokeText, StrokeTextListNameProvider>;
using CmdStrokeTextsSwap = CmdListElementsSwap<StrokeText, StrokeTextListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_STROKETEXT_H
