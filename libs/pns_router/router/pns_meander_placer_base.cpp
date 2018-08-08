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

#include "pns_router.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"

namespace PNS {

MEANDER_PLACER_BASE::MEANDER_PLACER_BASE( ROUTER* aRouter ) :
        PLACEMENT_ALGO( aRouter )
{
    m_currentWidth = 0;
}


MEANDER_PLACER_BASE::~MEANDER_PLACER_BASE()
{
}


void MEANDER_PLACER_BASE::AmplitudeStep( int aSign )
{
    int a = m_settings.m_maxAmplitude + aSign * m_settings.m_step;
    a = std::max( a,  m_settings.m_minAmplitude );

    m_settings.m_maxAmplitude = a;
}


void MEANDER_PLACER_BASE::SpacingStep( int aSign )
{
    int s = m_settings.m_spacing + aSign * m_settings.m_step;
    s = std::max( s, 2 * m_currentWidth );

    m_settings.m_spacing = s;
}


void MEANDER_PLACER_BASE::UpdateSettings( const MEANDER_SETTINGS& aSettings )
{
    m_settings = aSettings;
}


void MEANDER_PLACER_BASE::cutTunedLine( const SHAPE_LINE_CHAIN& aOrigin,
                                            const VECTOR2I& aTuneStart,
                                            const VECTOR2I& aCursorPos,
                                            SHAPE_LINE_CHAIN& aPre,
                                            SHAPE_LINE_CHAIN& aTuned,
                                            SHAPE_LINE_CHAIN& aPost )
{
    VECTOR2I cp ( aCursorPos );

    if ( cp == aTuneStart ) // we don't like tuning segments with 0 length
    {
        int idx = aOrigin.FindSegment( cp );
        if( idx >= 0 )
        {
            const SEG& s = aOrigin.CSegment( idx );
            cp += (s.B - s.A).Resize(2);
        } else
            cp += VECTOR2I (2, 5); // some arbitrary value that is not 45 degrees oriented
    }

    VECTOR2I n = aOrigin.NearestPoint( cp );
    VECTOR2I m = aOrigin.NearestPoint( aTuneStart );

    SHAPE_LINE_CHAIN l( aOrigin );
    l.Split( n );
    l.Split( m );

    int i_start = l.Find( m );
    int i_end = l.Find( n );

    if( i_start > i_end )
    {
        l = l.Reverse();
        i_start = l.Find( m );
        i_end = l.Find( n );
    }

    aPre = l.Slice( 0, i_start );
    aPost = l.Slice( i_end, -1 );
    aTuned = l.Slice( i_start, i_end );

    aTuned.Simplify();
}


void MEANDER_PLACER_BASE::tuneLineLength( MEANDERED_LINE& aTuned, int aElongation )
{
    int remaining = aElongation;
    bool finished = false;

    for( MEANDER_SHAPE* m : aTuned.Meanders() )
    {
        if( m->Type() != MT_CORNER )
        {
            if( remaining >= 0 )
                remaining -= m->MaxTunableLength() - m->BaselineLength();

            if( remaining < 0 )
            {
                if( !finished )
                {
                    MEANDER_TYPE newType;

                    if( m->Type() == MT_START || m->Type() == MT_SINGLE )
                        newType = MT_SINGLE;
                    else
                        newType = MT_FINISH;

                    m->SetType( newType );
                    m->Recalculate();

                    finished = true;
                } else {
                    m->MakeEmpty();
                }
            }
        }
    }

    remaining = aElongation;
    int meanderCount = 0;

    for(MEANDER_SHAPE* m : aTuned.Meanders())
    {
        if( m->Type() != MT_CORNER && m->Type() != MT_EMPTY )
        {
            if(remaining >= 0)
            {
                remaining -= m->MaxTunableLength() - m->BaselineLength();
                meanderCount ++;
            }
        }
    }

    int balance = 0;

    if( meanderCount )
        balance = -remaining / meanderCount;

    if( balance >= 0 )
    {
        for( MEANDER_SHAPE* m : aTuned.Meanders() )
        {
            if( m->Type() != MT_CORNER && m->Type() != MT_EMPTY )
            {
                m->Resize( std::max( m->Amplitude() - balance / 2, m_settings.m_minAmplitude ) );
            }
        }
    }
}


const MEANDER_SETTINGS& MEANDER_PLACER_BASE::MeanderSettings() const
{
    return m_settings;
}


int MEANDER_PLACER_BASE::compareWithTolerance( int aValue, int aExpected, int aTolerance ) const
{
    if( aValue < aExpected - aTolerance )
        return -1;
    else if( aValue > aExpected + aTolerance )
        return 1;
    else
        return 0;
}

}
