/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_MEANDER_PLACER_BASE_H
#define __PNS_MEANDER_PLACER_BASE_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;

/**
 * Class MEANDER_PLACER_BASE
 *
 * Base class for Single trace & Differenial pair meandering tools, as
 * both of them share a lot of code.
 */
class MEANDER_PLACER_BASE : public PLACEMENT_ALGO
{
public:
    ///> Result of the length tuning operation
    enum TUNING_STATUS {
        TOO_SHORT = 0,
        TOO_LONG,
        TUNED
    };

    MEANDER_PLACER_BASE( ROUTER* aRouter );
    virtual ~MEANDER_PLACER_BASE();

    /**
     * Function TuningInfo()
     *
     * Returns a string describing the status and length of the
     * tuned traces.
     */
    virtual const std::string TuningInfo() const = 0;

    /**
     * Function TuningStatus()
     *
     * Returns the tuning status (too short, too long, etc.)
     * of the trace(s) being tuned.
     */
    virtual TUNING_STATUS TuningStatus() const = 0;

    /**
     * Function AmplitudeStep()
     *
     * Increases/decreases the current meandering amplitude by one step.
     * @param aSign direction (negative = decrease, positive = increase).
     */
    virtual void AmplitudeStep( int aSign );

    /**
     * Function SpacingStep()
     *
     * Increases/decreases the current meandering spcing by one step.
     * @param aSign direction (negative = decrease, positive = increase).
     */
    virtual void SpacingStep( int aSign );

    /**
     * Function MeanderSettings()
     *
     * Returns the current meandering configuration.
     * @return the settings
     */
    virtual const MEANDER_SETTINGS& MeanderSettings() const;

    /*
     * Function UpdateSettings()
     *
     * Sets the current meandering configuration.
     * @param aSettings the settings
     */
    virtual void UpdateSettings( const MEANDER_SETTINGS& aSettings);

    /**
     * Function CheckFit()
     *
     * Checks if it's ok to place the shape aShape (i.e.
     * if it doesn't cause DRC violations or collide with
     * other meanders).
     * @param aShape the shape to check
     * @return true if the shape fits
     */
    virtual bool CheckFit( MEANDER_SHAPE* aShape )
    {
        return false;
    }

protected:

    /**
     * Function cutTunedLine()
     *
     * Extracts the part of a track to be meandered, depending on the
     * starting point and the cursor position.
     * @param aOrigin the original line
     * @param aTuneStart point where we start meandering (start click coorinates)
     * @param aCursorPos current cursor position
     * @param aPre part before the beginning of meanders
     * @param aTuned part to be meandered
     * @param aPost part after the end of meanders
     */
    void cutTunedLine(  const SHAPE_LINE_CHAIN& aOrigin,
                        const VECTOR2I&         aTuneStart,
                        const VECTOR2I&         aCursorPos,
                        SHAPE_LINE_CHAIN&       aPre,
                        SHAPE_LINE_CHAIN&       aTuned,
                        SHAPE_LINE_CHAIN&       aPost );

    /**
     * Function tuneLineLength()
     *
     * Takes a set of meanders in aTuned and tunes their length to
     * extend the original line length by aElongation.
     */
    void tuneLineLength( MEANDERED_LINE& aTuned, int aElongation );

    /**
     * Function compareWithTolerance()
     *
     * Compares aValue against aExpected with given tolerance.
     */
    int compareWithTolerance ( int aValue, int aExpected, int aTolerance = 0 ) const;

    ///> width of the meandered trace(s)
    int m_currentWidth;
    ///> meandering settings
    MEANDER_SETTINGS m_settings;
    ///> current end point
    VECTOR2I m_currentEnd;
};

}

#endif    // __PNS_MEANDER_PLACER_BASE_H
