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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

#include <geometry/direction45.h>
#include <geometry/shape.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>


#include "pns_diff_pair.h"
#include "pns_router.h"
#include "pns_solid.h"
#include "pns_utils.h"
#include "pns_debug_decorator.h"

namespace PNS {

class LINE;

DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( ITEM* aPrimP, ITEM* aPrimN )
{
    m_primP = aPrimP->Clone();
    m_primN = aPrimN->Clone();

    m_anchorP = m_primP->Anchor( 0 );
    m_anchorN = m_primN->Anchor( 0 );
}


void DP_PRIMITIVE_PAIR::SetAnchors( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
}


DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
    m_primP = m_primN = NULL;
}


DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( const DP_PRIMITIVE_PAIR& aOther )
{
    m_primP = m_primN = NULL;

    if( aOther.m_primP )
        m_primP = aOther.m_primP->Clone();

    if( aOther.m_primN )
        m_primN = aOther.m_primN->Clone();

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;
}


DP_PRIMITIVE_PAIR& DP_PRIMITIVE_PAIR::operator=( const DP_PRIMITIVE_PAIR& aOther )
{
    if( aOther.m_primP )
        m_primP = aOther.m_primP->Clone();
    if( aOther.m_primN )
        m_primN = aOther.m_primN->Clone();

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;

    return *this;
}


DP_PRIMITIVE_PAIR::~DP_PRIMITIVE_PAIR()
{
    delete m_primP;
    delete m_primN;
}


bool DP_PRIMITIVE_PAIR::Directional() const
{
    if( !m_primP )
        return false;

    return m_primP->OfKind( ITEM::SEGMENT_T );
}


DIRECTION_45 DP_PRIMITIVE_PAIR::anchorDirection( ITEM* aItem, const VECTOR2I& aP ) const
{
    if( !aItem->OfKind ( ITEM::SEGMENT_T ) )
        return DIRECTION_45();

    SEGMENT* s = static_cast<SEGMENT*>( aItem );

    if( s->Seg().A == aP )
        return DIRECTION_45( s->Seg().A - s->Seg().B );
    else
        return DIRECTION_45( s->Seg().B - s->Seg().A );
}

void DP_PRIMITIVE_PAIR::CursorOrientation( const VECTOR2I& aCursorPos, VECTOR2I& aMidpoint, VECTOR2I& aDirection ) const
{
    assert( m_primP && m_primN );

    VECTOR2I aP, aN, dir, midpoint;

    if ( m_primP->OfKind( ITEM::SEGMENT_T ) && m_primN->OfKind( ITEM::SEGMENT_T ) )
    {
        aP = m_primP->Anchor( 1 );
        aN = m_primN->Anchor( 1 );
        midpoint = ( aP + aN ) / 2;
        SEG s = static_cast <SEGMENT*>( m_primP )->Seg();

        if ( s.B != s.A )
        {
            dir = s.B - s.A;
        }
        else
        {
            dir = VECTOR2I( 0, 1 );
        }

        dir = dir.Resize( ( aP - aN ).EuclideanNorm() );

    }
    else
    {
        aP = m_primP->Anchor( 0 );
        aN = m_primN->Anchor( 0 );
        midpoint = ( aP + aN ) / 2;
        dir = ( aP - aN ).Perpendicular();

        if ( dir.Dot( aCursorPos - midpoint ) < 0 )
            dir = -dir;
    }

    aMidpoint = midpoint;
    aDirection = dir;
}


DIRECTION_45 DP_PRIMITIVE_PAIR::DirP() const
{
    return anchorDirection( m_primP, m_anchorP );
}


DIRECTION_45 DP_PRIMITIVE_PAIR::DirN() const
{
    return anchorDirection( m_primN, m_anchorN );
}


static DIRECTION_45::AngleType angle( const VECTOR2I &a, const VECTOR2I &b )
{
    DIRECTION_45 dir_a( a );
    DIRECTION_45 dir_b( b );

    return dir_a.Angle( dir_b );
}


static bool checkGap( const SHAPE_LINE_CHAIN &p, const SHAPE_LINE_CHAIN &n, int gap )
{
    int i, j;

    for( i = 0; i < p.SegmentCount(); i++ )
    {
        for( j = 0; j < n.SegmentCount() ; j++ )
        {
            int dist = p.CSegment( i ).Distance( n.CSegment( j ) );

            if( dist < gap - 100 )
                return false;
        }
    }

    return true;
}


void DP_GATEWAY::Reverse()
{
    m_entryN = m_entryN.Reverse();
    m_entryP = m_entryP.Reverse();
}


bool DIFF_PAIR::BuildInitial( const DP_GATEWAY& aEntry, const DP_GATEWAY &aTarget, bool aPrefDiagonal )
{
    SHAPE_LINE_CHAIN p = DIRECTION_45().BuildInitialTrace ( aEntry.AnchorP(), aTarget.AnchorP(), aPrefDiagonal );
    SHAPE_LINE_CHAIN n = DIRECTION_45().BuildInitialTrace ( aEntry.AnchorN(), aTarget.AnchorN(), aPrefDiagonal );

    int mask = aEntry.AllowedAngles() | DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_OBTUSE;

    SHAPE_LINE_CHAIN sum_n, sum_p;
    m_p = p;
    m_n = n;

    if( aEntry.HasEntryLines() )
    {
        if( !aEntry.Entry().CheckConnectionAngle( *this, mask ) )
            return false;

        sum_p = aEntry.Entry().CP();
        sum_n = aEntry.Entry().CN();
        sum_p.Append( p );
        sum_n.Append( n );
    }
    else
    {
        sum_p = p;
        sum_n = n;
    }

    mask = aTarget.AllowedAngles() | DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_OBTUSE;

    m_p = sum_p;
    m_n = sum_n;

    if( aTarget.HasEntryLines() )
    {
        DP_GATEWAY t(aTarget) ;
        t.Reverse();

        if( !CheckConnectionAngle( t.Entry(), mask ) )
            return false;

        sum_p.Append( t.Entry().CP() );
        sum_n.Append( t.Entry().CN() );
    }

    m_p = sum_p;
    m_n = sum_n;

    if( !checkGap ( p, n, m_gapConstraint ) )
        return false;

    if( p.SelfIntersecting() || n.SelfIntersecting() )
        return false;

    if( p.Intersects( n ) )
        return false;

    return true;
}


bool DIFF_PAIR::CheckConnectionAngle( const DIFF_PAIR& aOther, int aAllowedAngles ) const
{
    bool checkP, checkN;

    if( m_p.SegmentCount() == 0 || aOther.m_p.SegmentCount() == 0 )
        checkP = true;
    else
    {
        DIRECTION_45 p0( m_p.CSegment( -1 ) );
        DIRECTION_45 p1( aOther.m_p.CSegment( 0 ) );

        checkP = ( p0.Angle( p1 ) & aAllowedAngles ) != 0;
    }

    if( m_n.SegmentCount() == 0 || aOther.m_n.SegmentCount() == 0 )
        checkN = true;
    else
    {
        DIRECTION_45 n0( m_n.CSegment( -1 ) );
        DIRECTION_45 n1( aOther.m_n.CSegment( 0 ) );

        checkN = ( n0.Angle( n1 ) & aAllowedAngles ) != 0;
    }

    return checkP && checkN;
}


const DIFF_PAIR DP_GATEWAY::Entry() const
{
    return DIFF_PAIR( m_entryP, m_entryN, 0 );
}


void DP_GATEWAYS::BuildOrthoProjections( DP_GATEWAYS& aEntries,
        const VECTOR2I& aCursorPos, int aOrthoScore )
{
    for( const DP_GATEWAY& g : aEntries.Gateways() )
    {
        VECTOR2I midpoint( ( g.AnchorP() + g.AnchorN() ) / 2 );
        SEG guide_s( midpoint, midpoint + VECTOR2I( 1, 0 ) );
        SEG guide_d( midpoint, midpoint + VECTOR2I( 1, 1 ) );

        VECTOR2I proj_s = guide_s.LineProject( aCursorPos );
        VECTOR2I proj_d = guide_d.LineProject( aCursorPos );

        int dist_s = ( proj_s - aCursorPos ).EuclideanNorm();
        int dist_d = ( proj_d - aCursorPos ).EuclideanNorm();


        VECTOR2I proj = ( dist_s < dist_d ? proj_s : proj_d );

        DP_GATEWAYS targets( m_gap );

        targets.m_viaGap = m_viaGap;
        targets.m_viaDiameter = m_viaDiameter;
        targets.m_fitVias = m_fitVias;

        targets.BuildForCursor( proj );

        for( DP_GATEWAY t : targets.Gateways() )
        {
            t.SetPriority( aOrthoScore );
            m_gateways.push_back( t );
        }
    }
}


bool DP_GATEWAYS::FitGateways( DP_GATEWAYS& aEntry, DP_GATEWAYS& aTarget,
        bool aPrefDiagonal, DIFF_PAIR& aDp )
{
    DP_CANDIDATE best;

    int n = 0;
    int bestScore = -1000;
    bool found = false;

    for( const DP_GATEWAY& g_entry : aEntry.Gateways() )
    {
        for( const DP_GATEWAY& g_target : aTarget.Gateways() )
        {
            n++;

            for( int attempt = 0; attempt < 2; attempt++ )
            {
                int score = ( attempt == 1 ? -3 : 0 );
                score += g_entry.Priority();
                score += g_target.Priority();

                if( score < bestScore )
                    continue;

                DIFF_PAIR l( m_gap );

                if( l.BuildInitial( g_entry, g_target, aPrefDiagonal ^ ( attempt ? true : false ) ) )
                {
                    best.p = l.CP();
                    best.n = l.CN();
                    bestScore = score;
                    found = true;
                }
            }
        }
    }


    if( found )
    {
        aDp.SetGap( m_gap );
        aDp.SetShape( best.p, best.n );
        return true;
    }

    return false;
}


bool DP_GATEWAYS::checkDiagonalAlignment( const VECTOR2I& a, const VECTOR2I& b ) const
{
    VECTOR2I dir ( std::abs (a.x - b.x), std::abs ( a.y - b.y ));

    return (dir.x == 0 && dir.y != 0) || (dir.x == dir.y) || (dir.y == 0 && dir.x != 0);
}


void DP_GATEWAYS::FilterByOrientation ( int aAngleMask, DIRECTION_45 aRefOrientation )
{
    m_gateways.erase(
        std::remove_if( m_gateways.begin(), m_gateways.end(), [aAngleMask, aRefOrientation]( const DP_GATEWAY& dp) {
            DIRECTION_45 orient( dp.AnchorP() - dp.AnchorN() );
            return !( orient.Angle( aRefOrientation ) & aAngleMask );
        } ), m_gateways.end()
    );
}

static VECTOR2I makeGapVector( VECTOR2I dir, int length )
{
    int l = length / 2;
    VECTOR2I rv;

    if( dir.EuclideanNorm() == 0 )
        return dir;

    do
	{
        rv = dir.Resize( l );
        l++;
    } while( ( rv * 2 ).EuclideanNorm() < length );

    return rv;
}

void DP_GATEWAYS::BuildFromPrimitivePair( const DP_PRIMITIVE_PAIR& aPair, bool aPreferDiagonal )
{
    VECTOR2I majorDirection;
    VECTOR2I p0_p, p0_n;
    int orthoFanDistance;
    int diagFanDistance;
    const SHAPE* shP = NULL;

    if( aPair.PrimP() == NULL )
    {
        BuildGeneric( aPair.AnchorP(), aPair.AnchorN(), true );
        return;
    }

    const int pvMask = ITEM::SOLID_T | ITEM::VIA_T;

    if( aPair.PrimP()->OfKind( pvMask ) && aPair.PrimN()->OfKind(  pvMask ) )
    {
        p0_p = aPair.AnchorP();
        p0_n = aPair.AnchorN();

        shP = aPair.PrimP()->Shape();
    }
    else if( aPair.PrimP()->OfKind( ITEM::SEGMENT_T ) && aPair.PrimN()->OfKind( ITEM::SEGMENT_T ) )
    {
        buildDpContinuation( aPair, aPreferDiagonal );

        return;
    }

    majorDirection = ( p0_p - p0_n ).Perpendicular();

    if( shP == NULL )
        return;

    switch( shP->Type() )
    {
    case SH_RECT:
    {
        int w = static_cast<const SHAPE_RECT*>( shP )->GetWidth();
        int h = static_cast<const SHAPE_RECT*>( shP )->GetHeight();

        if( w < h )
            std::swap( w, h );

        orthoFanDistance = ( w + 1 )* 3 / 2;
        diagFanDistance = ( w - h );
        break;
    }

    case SH_SEGMENT:
    {
        int w = static_cast<const SHAPE_SEGMENT*>( shP )->GetWidth();
        SEG s = static_cast<const SHAPE_SEGMENT*>( shP )->GetSeg();

        orthoFanDistance = w + ( s.B - s.A ).EuclideanNorm();
        diagFanDistance = ( s.B - s.A ).EuclideanNorm();
        break;
    }

    default:
        BuildGeneric ( p0_p, p0_n, true );
        return;
    }

    if( checkDiagonalAlignment( p0_p, p0_n ) )
    {
        int padDist = ( p0_p - p0_n ).EuclideanNorm();

        for( int k = 0; k < 2; k++ )
        {
            VECTOR2I dir, dp, dv;

            if( k == 0 )
            {
                dir = makeGapVector( majorDirection, orthoFanDistance );
                int d = ( padDist - m_gap );
                dp = makeGapVector( dir, d );
                dv = makeGapVector( p0_n - p0_p, d );
            }
            else
            {
                dir = makeGapVector( majorDirection, diagFanDistance );
                int d = ( padDist - m_gap );
                dp = makeGapVector( dir, d );
                dv = makeGapVector( p0_n - p0_p, d );
            }

            for( int i = 0; i < 2; i++ )
            {
                int sign = i ? -1 : 1;

                VECTOR2I gw_p( p0_p + sign * ( dir + dp ) + dv );
                VECTOR2I gw_n( p0_n + sign * ( dir + dp ) - dv );

                SHAPE_LINE_CHAIN entryP( p0_p, p0_p + sign * dir, gw_p );
                SHAPE_LINE_CHAIN entryN( p0_n, p0_n + sign * dir, gw_n );

                DP_GATEWAY gw( gw_p, gw_n, false );

                gw.SetEntryLines( entryP, entryN );
                gw.SetPriority( 100 - k );
                m_gateways.push_back( gw );
            }
        }
    }

    BuildGeneric( p0_p, p0_n, true );
}


void DP_GATEWAYS::BuildForCursor( const VECTOR2I& aCursorPos )
{
    int gap = m_fitVias ? m_viaGap + m_viaDiameter : m_gap;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        for( int i = 0; i < 4; i++ )
        {
            VECTOR2I dir;

            if( !attempt )
            {
                dir = makeGapVector( VECTOR2I( gap, gap ), gap );

                if( i % 2 == 0 )
                    dir.x = -dir.x;

                if( i / 2 == 0 )
                    dir.y = -dir.y;
            }
            else
            {
                if( i /2 == 0 )
                    dir = VECTOR2I( (gap + 1) / 2 * ( ( i % 2 ) ? -1 : 1 ), 0 );
                else
                    dir = VECTOR2I( 0, (gap + 1) / 2 * ( ( i % 2 ) ? -1 : 1) );
            }

            if( m_fitVias )
                BuildGeneric( aCursorPos + dir, aCursorPos - dir, true, true );
            else
                m_gateways.push_back( DP_GATEWAY( aCursorPos + dir,
                                      aCursorPos - dir, attempt ? true : false ) );

        }
    }
}


void DP_GATEWAYS::buildEntries( const VECTOR2I& p0_p, const VECTOR2I& p0_n )
{
    for( DP_GATEWAY &g : m_gateways )
    {
        if( !g.HasEntryLines() )
        {
            SHAPE_LINE_CHAIN lead_p = DIRECTION_45().BuildInitialTrace ( g.AnchorP(), p0_p, g.IsDiagonal() ).Reverse();
            SHAPE_LINE_CHAIN lead_n = DIRECTION_45().BuildInitialTrace ( g.AnchorN(), p0_n, g.IsDiagonal() ).Reverse();
            g.SetEntryLines( lead_p, lead_n );
        }
    }
}


void DP_GATEWAYS::buildDpContinuation( const DP_PRIMITIVE_PAIR& aPair, bool aIsDiagonal )
{
    DP_GATEWAY gw( aPair.AnchorP(), aPair.AnchorN(), aIsDiagonal );
    gw.SetPriority( 100 );
    m_gateways.push_back( gw );

    if( !aPair.Directional() )
        return;

    DIRECTION_45 dP = aPair.DirP();
    DIRECTION_45 dN = aPair.DirN();

    int gap = ( aPair.AnchorP() - aPair.AnchorN() ).EuclideanNorm();

    VECTOR2I vdP = aPair.AnchorP() + dP.Left().ToVector();
    VECTOR2I vdN = aPair.AnchorN() + dN.Left().ToVector();

    SEGMENT* sP = static_cast<SEGMENT*>( aPair.PrimP() );

    VECTOR2I t1, t2;

    auto vL = makeGapVector( dP.Left().ToVector(), ( gap + 1 ) / 2 );
    auto vR = makeGapVector( dP.Right().ToVector(), ( gap + 1 ) / 2 );

    if( sP->Seg().Side( vdP ) == sP->Seg().Side( vdN ) )
    {
        t1 = aPair.AnchorP() + vL;
        t2 = aPair.AnchorN() + vR;
    }
    else
    {
        t1 = aPair.AnchorP() + vR;
        t2 = aPair.AnchorN() + vL;
    }

    DP_GATEWAY gwL( t2, aPair.AnchorN(), !aIsDiagonal );
    SHAPE_LINE_CHAIN ep = dP.BuildInitialTrace( aPair.AnchorP(), t2, !aIsDiagonal );
    gwL.SetPriority( 10 );
    gwL.SetEntryLines( ep , SHAPE_LINE_CHAIN() );

    m_gateways.push_back( gwL );

    DP_GATEWAY gwR( aPair.AnchorP(), t1, !aIsDiagonal );
    SHAPE_LINE_CHAIN en = dP.BuildInitialTrace( aPair.AnchorN(), t1, !aIsDiagonal );
    gwR.SetPriority( 10) ;
    gwR.SetEntryLines( SHAPE_LINE_CHAIN(), en );

    m_gateways.push_back( gwR );
}


void DP_GATEWAYS::BuildGeneric( const VECTOR2I& p0_p, const VECTOR2I& p0_n, bool aBuildEntries, bool aViaMode )
{
    SEG st_p[2], st_n[2];
    SEG d_n[2], d_p[2];

    const int padToGapThreshold = 3;
    int padDist = ( p0_p - p0_p ).EuclideanNorm();

    st_p[0] = SEG(p0_p + VECTOR2I( -100, 0 ), p0_p + VECTOR2I( 100, 0 ) );
    st_n[0] = SEG(p0_n + VECTOR2I( -100, 0 ), p0_n + VECTOR2I( 100, 0 ) );
    st_p[1] = SEG(p0_p + VECTOR2I( 0, -100 ), p0_p + VECTOR2I( 0, 100 ) );
    st_n[1] = SEG(p0_n + VECTOR2I( 0, -100 ), p0_n + VECTOR2I( 0, 100 ) );
    d_p[0] = SEG( p0_p + VECTOR2I( -100, -100 ), p0_p + VECTOR2I( 100, 100 ) );
    d_p[1] = SEG( p0_p + VECTOR2I( 100, -100 ), p0_p + VECTOR2I( -100, 100 ) );
    d_n[0] = SEG( p0_n + VECTOR2I( -100, -100 ), p0_n + VECTOR2I( 100, 100 ) );
    d_n[1] = SEG( p0_n + VECTOR2I( 100, -100 ), p0_n + VECTOR2I( -100, 100 ) );

    // midpoint exit & side-by exits
    for( int i = 0; i < 2; i++ )
    {
        bool straightColl = st_p[i].Collinear( st_n[i] );
        bool diagColl = d_p[i].Collinear( d_n[i] );

        if( straightColl || diagColl )
        {
            VECTOR2I dir = makeGapVector( p0_n - p0_p, m_gap / 2 );
            VECTOR2I m = ( p0_p + p0_n ) / 2;
            int prio = ( padDist > padToGapThreshold * m_gap ? 2 : 1);

            if( !aViaMode )
            {
                m_gateways.push_back( DP_GATEWAY( m - dir, m + dir, diagColl, DIRECTION_45::ANG_RIGHT, prio ) );

                dir = makeGapVector( p0_n - p0_p, 2 * m_gap );
                m_gateways.push_back( DP_GATEWAY( p0_p - dir, p0_p - dir + dir.Perpendicular(), diagColl ) );
                m_gateways.push_back( DP_GATEWAY( p0_p - dir, p0_p - dir - dir.Perpendicular(), diagColl ) );
                m_gateways.push_back( DP_GATEWAY( p0_n + dir + dir.Perpendicular(), p0_n + dir, diagColl ) );
                m_gateways.push_back( DP_GATEWAY( p0_n + dir - dir.Perpendicular(), p0_n + dir, diagColl ) );
            }
        }
    }

    for( int i = 0; i < 2; i++ )
    {
        for( int j = 0; j < 2; j++ )
        {
            OPT_VECTOR2I ips[2];

            ips[0] = d_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( st_n[j] );

            if( d_n[i].Collinear( d_p[j] ) )
                ips[0] = OPT_VECTOR2I();
            if( st_p[i].Collinear( st_p[j] ) )
                ips[1] = OPT_VECTOR2I();

            // diagonal-diagonal and straight-straight cases - the most typical case if the pads
            // are on the same straight/diagonal line
            for( int k = 0; k < 2; k++ )
            {
                if( ips[k] )
                {
                    const VECTOR2I m( *ips[k] );

                    if( m != p0_p && m != p0_n )
                    {
                        int prio = ( padDist > padToGapThreshold * m_gap ? 10 : 20 );
                        VECTOR2I g_p( ( p0_p - m ).Resize( ceil( (double) m_gap * M_SQRT1_2 ) ) );
                        VECTOR2I g_n( ( p0_n - m ).Resize( ceil( (double) m_gap * M_SQRT1_2 ) ) );

                        m_gateways.push_back( DP_GATEWAY( m + g_p, m + g_n, k == 0 ? true : false, DIRECTION_45::ANG_OBTUSE, prio ) );
                    }
                }
            }

            ips[0] = st_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( d_n[j] );

//          diagonal-straight cases: 8 possibilities of "weirder" exists
            for( int k = 0; k < 2; k++ )
            {
                if( ips[k] )
                {
                    const VECTOR2I m( *ips[k] );

                    if( !aViaMode && m != p0_p && m != p0_n )
                    {
                        VECTOR2I g_p, g_n;

                        g_p = ( p0_p - m ).Resize( ceil( (double) m_gap * M_SQRT2 ) );
                        g_n = ( p0_n - m ).Resize( ceil( (double) m_gap ) );

                        if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                            m_gateways.push_back( DP_GATEWAY( m + g_p, m + g_n, true ) );

                        g_p = ( p0_p - m ).Resize( m_gap );
                        g_n = ( p0_n - m ).Resize( ceil( (double) m_gap * M_SQRT2 ) );

                        if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                            m_gateways.push_back( DP_GATEWAY( m + g_p, m + g_n, true ) );
                    }
                }
            }
        }
    }

    if( aBuildEntries )
        buildEntries( p0_p, p0_n );
}


DP_PRIMITIVE_PAIR DIFF_PAIR::EndingPrimitives()
{
    if( m_hasVias )
        return DP_PRIMITIVE_PAIR( &m_via_p, &m_via_n );
    else
    {
        const LINE lP( PLine() );
        const LINE lN( NLine() );

        SEGMENT sP( lP, lP.CSegment( -1 ) );
        SEGMENT sN( lN, lN.CSegment( -1 ) );

        DP_PRIMITIVE_PAIR dpair( &sP, &sN );
        dpair.SetAnchors( sP.Seg().B, sN.Seg().B );

        return dpair;
    }
}


bool commonParallelProjection( SEG n, SEG p, SEG &pClip, SEG& nClip )
{
    SEG n_proj_p( p.LineProject( n.A ), p.LineProject( n.B ) );

    int64_t t_a = 0;
    int64_t t_b = p.TCoef( p.B );

    int64_t tproj_a = p.TCoef( n_proj_p.A );
    int64_t tproj_b = p.TCoef( n_proj_p.B );

    if( t_b < t_a )
        std::swap( t_b, t_a );

    if( tproj_b < tproj_a )
        std::swap( tproj_b, tproj_a );

    if( t_b <= tproj_a )
        return false;

    if( t_a >= tproj_b )
        return false;

    int64_t t[4] = { 0, p.TCoef( p.B ), p.TCoef( n_proj_p.A ), p.TCoef( n_proj_p.B ) };
    std::vector<int64_t> tv( t, t + 4 );
    std::sort( tv.begin(), tv.end() ); // fixme: awful and disgusting way of finding 2 midpoints

    int64_t pLenSq = p.SquaredLength();

    VECTOR2I dp = p.B - p.A;
    pClip.A.x = p.A.x + rescale( (int64_t)dp.x, tv[1], pLenSq );
    pClip.A.y = p.A.y + rescale( (int64_t)dp.y, tv[1], pLenSq );

    pClip.B.x = p.A.x + rescale( (int64_t)dp.x, tv[2], pLenSq );
    pClip.B.y = p.A.y + rescale( (int64_t)dp.y, tv[2], pLenSq );

    nClip.A = n.LineProject( pClip.A );
    nClip.B = n.LineProject( pClip.B );

    return true;
}


double DIFF_PAIR::Skew() const
{
    return m_p.Length() - m_n.Length();
}


void DIFF_PAIR::CoupledSegmentPairs( COUPLED_SEGMENTS_VEC& aPairs ) const
{
    SHAPE_LINE_CHAIN p( m_p );
    SHAPE_LINE_CHAIN n( m_n );

    p.Simplify();
    n.Simplify();

    for( int i = 0; i < p.SegmentCount(); i++ )
    {
        for( int j = 0; j < n.SegmentCount(); j++ )
        {
            SEG sp = p.CSegment( i );
            SEG sn = n.CSegment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance( sn ) - m_width );

            if( sp.ApproxParallel( sn ) && m_gapConstraint.Matches( dist ) && commonParallelProjection( sp, sn, p_clip, n_clip ) )
            {
                const COUPLED_SEGMENTS spair( p_clip, sp, i, n_clip, sn, j );
                aPairs.push_back( spair );
            }
        }
    }
}


int64_t DIFF_PAIR::CoupledLength( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const
{
    int64_t total = 0;

    for( int i = 0; i < aP.SegmentCount(); i++ )
    {
        for( int j = 0; j < aN.SegmentCount(); j++ )
        {
            SEG sp = aP.CSegment( i );
            SEG sn = aN.CSegment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance(sn) - m_width );

            if( sp.ApproxParallel( sn ) && m_gapConstraint.Matches( dist ) &&
                    commonParallelProjection( sp, sn, p_clip, n_clip ) )
                total += p_clip.Length();
        }
    }

    return total;
}


double DIFF_PAIR::CoupledLength() const
{
    COUPLED_SEGMENTS_VEC pairs;

    CoupledSegmentPairs( pairs );

    double l = 0.0;
    for( unsigned int i = 0; i < pairs.size(); i++ )
        l += pairs[i].coupledP.Length();

    return l;
}


double DIFF_PAIR::CoupledLengthFactor() const
{
    double t = TotalLength();

    if( t == 0.0 )
        return 0.0;

    return CoupledLength() / t;
}


double DIFF_PAIR::TotalLength() const
{
    double lenP = m_p.Length();
    double lenN = m_n.Length();

    return (lenN + lenP ) / 2.0;
}


int DIFF_PAIR::CoupledLength ( const SEG& aP, const SEG& aN ) const
{
    SEG p_clip, n_clip;
    int64_t dist = std::abs( aP.Distance( aN ) - m_width );

    if( aP.ApproxParallel( aN ) && m_gapConstraint.Matches( dist ) &&
            commonParallelProjection ( aP, aN, p_clip, n_clip ) )
        return p_clip.Length();

    return 0;
}

}
