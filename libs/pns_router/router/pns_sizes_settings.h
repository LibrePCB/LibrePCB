/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014 CERN
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

#ifndef __PNS_SIZES_SETTINGS_H
#define __PNS_SIZES_SETTINGS_H

#include <map>
#include <core/optional.h>

#include "../class_track.h" // for VIATYPE_T

class BOARD;
class BOARD_DESIGN_SETTINGS;

namespace PNS {

class ITEM;

class SIZES_SETTINGS {

public:
    SIZES_SETTINGS() :
        m_trackWidth( 155000 ),
        m_diffPairWidth( 125000 ),
        m_diffPairGap( 180000 ),
        m_diffPairViaGap( 180000 ),
        m_viaDiameter( 600000 ),
        m_viaDrill( 250000 ),
        m_diffPairViaGapSameAsTraceGap( true ),
		m_widthFromRules( false),
        m_viaType( VIA_THROUGH )
    {};

    ~SIZES_SETTINGS() {};

    void ClearLayerPairs();
    void AddLayerPair( int aL1, int aL2 );

    int TrackWidth() const { return m_trackWidth; }
    void SetTrackWidth( int aWidth ) { m_trackWidth = aWidth; }

    int DiffPairWidth() const { return m_diffPairWidth; }
    int DiffPairGap() const { return m_diffPairGap; }

    int DiffPairViaGap() const {
        if( m_diffPairViaGapSameAsTraceGap )
            return m_diffPairGap;
        else
            return m_diffPairViaGap;
    }

    bool DiffPairViaGapSameAsTraceGap() const { return m_diffPairViaGapSameAsTraceGap; }
    bool WidthFromRules() const { return m_widthFromRules; }

    void SetDiffPairWidth( int aWidth ) { m_diffPairWidth = aWidth; }
    void SetDiffPairGap( int aGap ) { m_diffPairGap = aGap; }
    void SetDiffPairViaGapSameAsTraceGap ( bool aEnable ) { m_diffPairViaGapSameAsTraceGap = aEnable; }
    void SetDiffPairViaGap( int aGap ) { m_diffPairViaGap = aGap; }

    void SetWidthFromRules( int aEnable ) { m_widthFromRules = aEnable; }

    int ViaDiameter() const { return m_viaDiameter; }
    void SetViaDiameter( int aDiameter ) { m_viaDiameter = aDiameter; }

    int ViaDrill() const { return m_viaDrill; }
    void SetViaDrill( int aDrill ) { m_viaDrill = aDrill; }

    OPT<int> PairedLayer( int aLayerId )
    {
        if( m_layerPairs.find(aLayerId) == m_layerPairs.end() )
            return OPT<int>();

        return m_layerPairs[aLayerId];
    }

    int GetLayerTop() const;
    int GetLayerBottom() const;

    void SetViaType( VIATYPE_T aViaType ) { m_viaType = aViaType; }
    VIATYPE_T ViaType() const { return m_viaType; }

    static int inheritTrackWidth( ITEM* aItem );

private:

    int m_trackWidth;
    int m_diffPairWidth;
    int m_diffPairGap;
    int m_diffPairViaGap;
    int m_viaDiameter;
    int m_viaDrill;

    bool m_diffPairViaGapSameAsTraceGap;
    bool m_widthFromRules;

    VIATYPE_T m_viaType;

    std::map<int, int> m_layerPairs;
};

}

#endif // __PNS_SIZES_SETTINGS_H
