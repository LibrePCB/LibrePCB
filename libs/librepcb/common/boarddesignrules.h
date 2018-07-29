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

#ifndef LIBREPCB_BOARDDESIGNRULES_H
#define LIBREPCB_BOARDDESIGNRULES_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "fileio/serializableobject.h"
#include "elementname.h"
#include "exceptions.h"
#include "units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class BoardDesignRules
 ****************************************************************************************/

/**
 * @brief The BoardDesignRules class
 *
 * @author ubruhin
 * @date 2016-04-01
 */
class BoardDesignRules final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(BoardDesignRules)

    public:

        // Constructors / Destructor
        BoardDesignRules() noexcept;
        BoardDesignRules(const BoardDesignRules& other);
        explicit BoardDesignRules(const SExpression& node);
        ~BoardDesignRules() noexcept;

        // Getters : General Attributes
        const ElementName& getName() const noexcept {return mName;}
        const QString& getDescription() const noexcept {return mDescription;}

        // Getters: Stop Mask
        const UnsignedRatio& getStopMaskClearanceRatio() const noexcept {return mStopMaskClearanceRatio;}
        const UnsignedLength& getStopMaskClearanceMin() const noexcept {return mStopMaskClearanceMin;}
        const UnsignedLength& getStopMaskClearanceMax() const noexcept {return mStopMaskClearanceMax;}
        const UnsignedLength& getStopMaskMaxViaDiameter() const noexcept {return mStopMaskMaxViaDrillDiameter;}

        // Getters: Cream Mask
        const UnsignedRatio& getCreamMaskClearanceRatio() const noexcept {return mCreamMaskClearanceRatio;}
        const UnsignedLength& getCreamMaskClearanceMin() const noexcept {return mCreamMaskClearanceMin;}
        const UnsignedLength& getCreamMaskClearanceMax() const noexcept {return mCreamMaskClearanceMax;}

        // Getters: Restring
        const UnsignedRatio& getRestringPadRatio() const noexcept {return mRestringPadRatio;}
        const UnsignedLength& getRestringPadMin() const noexcept {return mRestringPadMin;}
        const UnsignedLength& getRestringPadMax() const noexcept {return mRestringPadMax;}
        const UnsignedRatio& getRestringViaRatio() const noexcept {return mRestringViaRatio;}
        const UnsignedLength& getRestringViaMin() const noexcept {return mRestringViaMin;}
        const UnsignedLength& getRestringViaMax() const noexcept {return mRestringViaMax;}


        // Setters: General Attributes
        void setName(const ElementName& name) noexcept {mName = name;}
        void setDescription(const QString& desc) noexcept {mDescription = desc;}

        // Setters: Stop Mask
        void setStopMaskClearanceRatio(const UnsignedRatio& ratio) noexcept {mStopMaskClearanceRatio = ratio;}
        void setStopMaskClearanceMin(const UnsignedLength& min) noexcept {mStopMaskClearanceMin = min;}
        void setStopMaskClearanceMax(const UnsignedLength& max) noexcept {mStopMaskClearanceMax = max;}
        void setStopMaskMaxViaDiameter(const UnsignedLength& dia) noexcept {mStopMaskMaxViaDrillDiameter = dia;}

        // Setters: Clear Mask
        void setCreamMaskClearanceRatio(const UnsignedRatio& ratio) noexcept {mCreamMaskClearanceRatio = ratio;}
        void setCreamMaskClearanceMin(const UnsignedLength& min) noexcept {mCreamMaskClearanceMin = min;}
        void setCreamMaskClearanceMax(const UnsignedLength& max) noexcept {mCreamMaskClearanceMax = max;}

        // Setters: Restring
        void setRestringPadRatio(const UnsignedRatio& ratio) noexcept {mRestringPadRatio = ratio;}
        void setRestringPadMin(const UnsignedLength& min) noexcept {mRestringPadMin = min;}
        void setRestringPadMax(const UnsignedLength& max) noexcept {mRestringPadMax = max;}
        void setRestringViaRatio(const UnsignedRatio& ratio) noexcept {mRestringViaRatio = ratio;}
        void setRestringViaMin(const UnsignedLength& min) noexcept {mRestringViaMin = min;}
        void setRestringViaMax(const UnsignedLength& max) noexcept {mRestringViaMax = max;}

        // General Methods
        void restoreDefaults() noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Helper Methods
        bool doesViaRequireStopMask(const Length& drillDia) const noexcept;
        UnsignedLength calcStopMaskClearance(const Length& padSize) const noexcept;
        UnsignedLength calcCreamMaskClearance(const Length& padSize) const noexcept;
        UnsignedLength calcPadRestring(const Length& drillDia) const noexcept;
        UnsignedLength calcViaRestring(const Length& drillDia) const noexcept;

        // Operator Overloadings
        BoardDesignRules& operator=(const BoardDesignRules& rhs) noexcept;


    private:
        bool checkAttributesValidity() const noexcept;


        // General Attributes
        ElementName mName;
        QString mDescription;

        // Stop Mask
        UnsignedRatio mStopMaskClearanceRatio;
        UnsignedLength mStopMaskClearanceMin;
        UnsignedLength mStopMaskClearanceMax;
        UnsignedLength mStopMaskMaxViaDrillDiameter;

        // Cream Mask
        UnsignedRatio mCreamMaskClearanceRatio;
        UnsignedLength mCreamMaskClearanceMin;
        UnsignedLength mCreamMaskClearanceMax;

        // Restring
        UnsignedRatio mRestringPadRatio;
        UnsignedLength mRestringPadMin;
        UnsignedLength mRestringPadMax;
        UnsignedRatio mRestringViaRatio;
        UnsignedLength mRestringViaMin;
        UnsignedLength mRestringViaMax;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_BOARDDESIGNRULES_H
