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

#ifndef LIBREPCB_PROJECT_BI_STROKETEXT_H
#define LIBREPCB_PROJECT_BI_STROKETEXT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/common/geometry/path.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class StrokeTextGraphicsItem;
class LineGraphicsItem;

namespace project {

class Project;
class Board;
class BI_Footprint;

/*****************************************************************************************
 *  Class BI_StrokeText
 ****************************************************************************************/

/**
 * @brief The BI_StrokeText class
 */
class BI_StrokeText final : public BI_Base, public SerializableObject,
                            public IF_StrokeTextObserver
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_StrokeText() = delete;
        BI_StrokeText(const BI_StrokeText& other) = delete;
        BI_StrokeText(Board& board, const BI_StrokeText& other);
        BI_StrokeText(Board& board, const SExpression& node);
        BI_StrokeText(Board& board, const StrokeText& text);
        ~BI_StrokeText() noexcept;

        // Getters
        StrokeText& getText() noexcept {return *mText;}
        const StrokeText& getText() const noexcept {return *mText;}
        bool isSelectable() const noexcept override;

        // General Methods
        BI_Footprint* getFootprint() const noexcept {return mFootprint;}
        void setFootprint(BI_Footprint* footprint) noexcept;
        void updateGraphicsItems() noexcept;
        void addToBoard() override;
        void removeFromBoard() override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::StrokeText;}
        const Point& getPosition() const noexcept override;
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_StrokeText& operator=(const BI_StrokeText& rhs) = delete;


    private slots:
        void boardAttributesChanged();


    private: // Methods
        void init();
        void updatePaths() noexcept;
        void strokeTextLayerNameChanged(const QString& newLayerName) noexcept override {Q_UNUSED(newLayerName); updateGraphicsItems();}
        void strokeTextTextChanged(const QString& newText) noexcept override {Q_UNUSED(newText);}
        void strokeTextPositionChanged(const Point& newPos) noexcept override {Q_UNUSED(newPos); updateGraphicsItems();}
        void strokeTextRotationChanged(const Angle& newRot) noexcept override {Q_UNUSED(newRot);}
        void strokeTextHeightChanged(const Length& newHeight) noexcept override {Q_UNUSED(newHeight);}
        void strokeTextStrokeWidthChanged(const Length& newStrokeWidth) noexcept override {Q_UNUSED(newStrokeWidth);}
        void strokeTextLetterSpacingChanged(const StrokeTextSpacing& spacing) noexcept override {Q_UNUSED(spacing);}
        void strokeTextLineSpacingChanged(const StrokeTextSpacing& spacing) noexcept override {Q_UNUSED(spacing);}
        void strokeTextAlignChanged(const Alignment& newAlign) noexcept override {Q_UNUSED(newAlign);}
        void strokeTextMirroredChanged(bool mirrored) noexcept override {Q_UNUSED(mirrored);}
        void strokeTextAutoRotateChanged(bool newAutoRotate) noexcept override {Q_UNUSED(newAutoRotate);}
        void strokeTextPathsChanged(const QVector<Path>& paths) noexcept override {Q_UNUSED(paths);}


    private: // Data
        BI_Footprint* mFootprint;
        QScopedPointer<StrokeText> mText;
        QScopedPointer<StrokeTextGraphicsItem> mGraphicsItem;
        QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_STROKETEXT_H
