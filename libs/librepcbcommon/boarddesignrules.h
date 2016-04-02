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

#ifndef LIBREPCB_BOARDDESIGNRULES_H
#define LIBREPCB_BOARDDESIGNRULES_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "fileio/if_xmlserializableobject.h"
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
class BoardDesignRules final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(BoardDesignRules)

    public:

        // Constructors / Destructor
        BoardDesignRules() noexcept;
        BoardDesignRules(const BoardDesignRules& other);
        explicit BoardDesignRules(const XmlDomElement& domElement) noexcept;
        ~BoardDesignRules() noexcept;

        // Getters : General Attributes
        const QString& getName() const noexcept {return mName;}
        const QString& getDescription() const noexcept {return mDescription;}

        // Getters: Stop Mask
        qreal getStopMaskClearanceRatio() const noexcept {return mStopMaskClearanceRatio;}
        const Length& getStopMaskClearanceMin() const noexcept {return mStopMaskClearanceMin;}
        const Length& getStopMaskClearanceMax() const noexcept {return mStopMaskClearanceMax;}
        const Length& getStopMaskMaxViaDiameter() const noexcept {return mStopMaskMaxViaDrillDiameter;}

        // Getters: Cream Mask
        qreal getCreamMaskClearanceRatio() const noexcept {return mCreamMaskClearanceRatio;}
        const Length& getCreamMaskClearanceMin() const noexcept {return mCreamMaskClearanceMin;}
        const Length& getCreamMaskClearanceMax() const noexcept {return mCreamMaskClearanceMax;}

        // Getters: Restring
        qreal getRestringPadRatio() const noexcept {return mRestringPadRatio;}
        const Length& getRestringPadMin() const noexcept {return mRestringPadMin;}
        const Length& getRestringPadMax() const noexcept {return mRestringPadMax;}
        qreal getRestringViaRatio() const noexcept {return mRestringViaRatio;}
        const Length& getRestringViaMin() const noexcept {return mRestringViaMin;}
        const Length& getRestringViaMax() const noexcept {return mRestringViaMax;}


        // Setters: General Attributes
        void setName(const QString& name) noexcept {if (!name.isEmpty()) mName = name;}
        void setDescription(const QString& desc) noexcept {mDescription = desc;}

        // Setters: Stop Mask
        void setStopMaskClearanceRatio(qreal ratio) noexcept {if (ratio > 0) mStopMaskClearanceRatio = ratio;}
        void setStopMaskClearanceMin(const Length& min) noexcept {if (min >= 0) mStopMaskClearanceMin = min;}
        void setStopMaskClearanceMax(const Length& max) noexcept {if (max >= 0) mStopMaskClearanceMax = max;}
        void setStopMaskMaxViaDiameter(const Length& dia) noexcept {if (dia >= 0) mStopMaskMaxViaDrillDiameter = dia;}

        // Setters: Clear Mask
        void setCreamMaskClearanceRatio(qreal ratio) noexcept {if (ratio > 0) mCreamMaskClearanceRatio = ratio;}
        void setCreamMaskClearanceMin(const Length& min) noexcept {if (min >= 0) mCreamMaskClearanceMin = min;}
        void setCreamMaskClearanceMax(const Length& max) noexcept {if (max >= 0) mCreamMaskClearanceMax = max;}

        // Setters: Restring
        void setRestringPadRatio(qreal ratio) noexcept {if (ratio > 0) mRestringPadRatio = ratio;}
        void setRestringPadMin(const Length& min) noexcept {if (min >= 0) mRestringPadMin = min;}
        void setRestringPadMax(const Length& max) noexcept {if (max >= 0) mRestringPadMax = max;}
        void setRestringViaRatio(qreal ratio) noexcept {if (ratio > 0) mRestringViaRatio = ratio;}
        void setRestringViaMin(const Length& min) noexcept {if (min >= 0) mRestringViaMin = min;}
        void setRestringViaMax(const Length& max) noexcept {if (max >= 0) mRestringViaMax = max;}

        // General Methods
        void restoreDefaults() noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Helper Methods
        bool doesViaRequireStopMask(const Length& drillDia) const noexcept;
        Length calcStopMaskClearance(const Length& padSize) const noexcept;
        Length calcCreamMaskClearance(const Length& padSize) const noexcept;
        Length calcPadRestring(const Length& drillDia) const noexcept;
        Length calcViaRestring(const Length& drillDia) const noexcept;

        // Operator Overloadings
        BoardDesignRules& operator=(const BoardDesignRules& rhs) noexcept;


    private:

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General Attributes
        QString mName;
        QString mDescription;

        // Stop Mask
        qreal mStopMaskClearanceRatio;
        Length mStopMaskClearanceMin;
        Length mStopMaskClearanceMax;
        Length mStopMaskMaxViaDrillDiameter;

        // Cream Mask
        qreal mCreamMaskClearanceRatio;
        Length mCreamMaskClearanceMin;
        Length mCreamMaskClearanceMax;

        // Restring
        qreal mRestringPadRatio;
        Length mRestringPadMin;
        Length mRestringPadMax;
        qreal mRestringViaRatio;
        Length mRestringViaMin;
        Length mRestringViaMax;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_BOARDDESIGNRULES_H
