/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <cmath>

#include "pns_line.h"
#include "pns_diff_pair.h"
#include "pns_node.h"
#include "pns_solid.h"
#include "pns_optimizer.h"

#include "geometry/shape_simple.h"
#include "pns_utils.h"
#include "pns_router.h"

namespace PNS {

/**
 *  Cost Estimator Methods
 */
int COST_ESTIMATOR::CornerCost( const SEG& aA, const SEG& aB )
{
    DIRECTION_45 dir_a( aA ), dir_b( aB );

    switch( dir_a.Angle( dir_b ) )
    {
    case DIRECTION_45::ANG_OBTUSE:
        return 1;

    case DIRECTION_45::ANG_STRAIGHT:
        return 0;

    case DIRECTION_45::ANG_ACUTE:
        return 50;

    case DIRECTION_45::ANG_RIGHT:
        return 30;

    case DIRECTION_45::ANG_HALF_FULL:
        return 60;

    default:
        return 100;
    }
}


int COST_ESTIMATOR::CornerCost( const SHAPE_LINE_CHAIN& aLine )
{
    int total = 0;

    for( int i = 0; i < aLine.SegmentCount() - 1; ++i )
        total += CornerCost( aLine.CSegment( i ), aLine.CSegment( i + 1 ) );

    return total;
}


int COST_ESTIMATOR::CornerCost( const LINE& aLine )
{
    return CornerCost( aLine.CLine() );
}


void COST_ESTIMATOR::Add( LINE& aLine )
{
    m_lengthCost += aLine.CLine().Length();
    m_cornerCost += CornerCost( aLine );
}


void COST_ESTIMATOR::Remove( LINE& aLine )
{
    m_lengthCost -= aLine.CLine().Length();
    m_cornerCost -= CornerCost( aLine );
}


void COST_ESTIMATOR::Replace( LINE& aOldLine, LINE& aNewLine )
{
    m_lengthCost -= aOldLine.CLine().Length();
    m_cornerCost -= CornerCost( aOldLine );
    m_lengthCost += aNewLine.CLine().Length();
    m_cornerCost += CornerCost( aNewLine );
}


bool COST_ESTIMATOR::IsBetter( COST_ESTIMATOR& aOther,
        double aLengthTolerance,
        double aCornerTolerance ) const
{
    if( aOther.m_cornerCost < m_cornerCost && aOther.m_lengthCost < m_lengthCost )
        return true;

    else if( aOther.m_cornerCost < m_cornerCost * aCornerTolerance &&
             aOther.m_lengthCost < m_lengthCost * aLengthTolerance )
        return true;

    return false;
}


/**
 *  Optimizer
 **/
OPTIMIZER::OPTIMIZER( NODE* aWorld ) :
    m_world( aWorld ),
    m_collisionKindMask( ITEM::ANY_T ),
    m_effortLevel( MERGE_SEGMENTS ),
    m_keepPostures( false ),
    m_restrictAreaActive( false )
{
}


OPTIMIZER::~OPTIMIZER()
{
}


struct OPTIMIZER::CACHE_VISITOR
{
    CACHE_VISITOR( const ITEM* aOurItem, NODE* aNode, int aMask ) :
        m_ourItem( aOurItem ),
        m_collidingItem( NULL ),
        m_node( aNode ),
        m_mask( aMask )
    {}

    bool operator()( ITEM* aOtherItem )
    {
        if( !( m_mask & aOtherItem->Kind() ) )
            return true;

        int clearance = m_node->GetClearance( aOtherItem, m_ourItem );

        if( !aOtherItem->Collide( m_ourItem, clearance ) )
            return true;

        m_collidingItem = aOtherItem;
        return false;
    }

    const ITEM* m_ourItem;
    ITEM* m_collidingItem;
    NODE* m_node;
    int m_mask;
};


void OPTIMIZER::cacheAdd( ITEM* aItem, bool aIsStatic = false )
{
    if( m_cacheTags.find( aItem ) != m_cacheTags.end() )
        return;

    m_cache.Add( aItem );
    m_cacheTags[aItem].m_hits = 1;
    m_cacheTags[aItem].m_isStatic = aIsStatic;
}


void OPTIMIZER::removeCachedSegments( LINE* aLine, int aStartVertex, int aEndVertex )
{
    if( !aLine->IsLinked() ) return;

    LINE::SEGMENT_REFS& segs = aLine->LinkedSegments();

    if( aEndVertex < 0 )
        aEndVertex += aLine->PointCount();

    for( int i = aStartVertex; i < aEndVertex - 1; i++ )
    {
        SEGMENT* s = segs[i];
        m_cacheTags.erase( s );
        m_cache.Remove( s );
    }
}


void OPTIMIZER::CacheRemove( ITEM* aItem )
{
    if( aItem->Kind() == ITEM::LINE_T )
        removeCachedSegments( static_cast<LINE*>( aItem ) );
}


void OPTIMIZER::CacheStaticItem( ITEM* aItem )
{
    cacheAdd( aItem, true );
}


void OPTIMIZER::ClearCache( bool aStaticOnly  )
{
    if( !aStaticOnly )
    {
        m_cacheTags.clear();
        m_cache.Clear();
        return;
    }

    for( CachedItemTags::iterator i = m_cacheTags.begin(); i!= m_cacheTags.end(); ++i )
    {
        if( i->second.m_isStatic )
        {
            m_cache.Remove( i->first );
            m_cacheTags.erase( i->first );
        }
    }
}


class LINE_RESTRICTIONS
{
    public:
        LINE_RESTRICTIONS() {};
        ~LINE_RESTRICTIONS() {};

        void Build( NODE* aWorld, LINE* aOriginLine, const SHAPE_LINE_CHAIN& aLine, const BOX2I& aRestrictedArea, bool aRestrictedAreaEnable );
        bool Check ( int aVertex1, int aVertex2, const SHAPE_LINE_CHAIN& aReplacement );
        void Dump();

    private:
        int allowedAngles( NODE* aWorld, const LINE* aLine, const VECTOR2I& aP, bool aFirst );

        struct RVERTEX
        {
            RVERTEX ( bool aRestricted, int aAllowedAngles ) :
                restricted( aRestricted ),
                allowedAngles( aAllowedAngles )
            {
            }

            bool restricted;
            int allowedAngles;
        };

        std::vector<RVERTEX> m_rs;
};


// fixme: use later
int LINE_RESTRICTIONS::allowedAngles( NODE* aWorld, const LINE* aLine, const VECTOR2I& aP, bool aFirst )
{
    JOINT* jt = aWorld->FindJoint( aP , aLine );

    if( !jt )
        return 0xff;

    DIRECTION_45 dirs [8];

    int n_dirs = 0;

    for( const ITEM* item : jt->Links().CItems() )
    {
        if( item->OfKind( ITEM::VIA_T ) || item->OfKind( ITEM::SOLID_T ) )
            return 0xff;
        else if( const SEGMENT* seg = dynamic_cast<const SEGMENT*>( item ) )
        {
            SEG s = seg->Seg();
            if( s.A != aP )
                s.Reverse();

            if( n_dirs < 8 )
                dirs[n_dirs++] = aFirst ? DIRECTION_45( s ) : DIRECTION_45( s ).Opposite();
        }
    }

    const int angleMask = DIRECTION_45::ANG_OBTUSE | DIRECTION_45::ANG_HALF_FULL | DIRECTION_45::ANG_STRAIGHT;
    int outputMask = 0xff;

    for( int d = 0; d < 8; d++ )
    {
        DIRECTION_45 refDir( ( DIRECTION_45::Directions ) d );

        for( int i = 0; i < n_dirs; i++ )
        {
            if( !( refDir.Angle( dirs[i] ) & angleMask ) )
                outputMask &= ~refDir.Mask();
        }
    }

    //DrawDebugDirs( aP, outputMask, 3 );
    return 0xff;
}


void LINE_RESTRICTIONS::Build( NODE* aWorld, LINE* aOriginLine, const SHAPE_LINE_CHAIN& aLine, const BOX2I& aRestrictedArea, bool aRestrictedAreaEnable )
{
    const SHAPE_LINE_CHAIN& l = aLine;
    VECTOR2I v_prev;
    int n = l.PointCount( );

    m_rs.reserve( n );

    for( int i = 0; i < n; i++ )
    {
        const VECTOR2I &v = l.CPoint( i );
        RVERTEX r( false, 0xff );

        if( aRestrictedAreaEnable )
        {
            bool exiting = ( i > 0 && aRestrictedArea.Contains( v_prev ) && !aRestrictedArea.Contains( v ) );
            bool entering = false;

            if( i != l.PointCount() - 1 )
            {
                const VECTOR2I& v_next = l.CPoint( i + 1 );
                entering = ( !aRestrictedArea.Contains( v ) && aRestrictedArea.Contains( v_next ) );
            }

            if( entering )
            {
                const SEG& sp = l.CSegment( i );
                r.allowedAngles = DIRECTION_45( sp ).Mask();
            }
            else if( exiting )
            {
                const SEG& sp = l.CSegment( i - 1 );
                r.allowedAngles = DIRECTION_45( sp ).Mask();
            }
            else
            {
                r.allowedAngles = ( !aRestrictedArea.Contains( v ) ) ? 0 : 0xff;
                r.restricted = r.allowedAngles ? false : true;
            }
        }

        v_prev = v;
        m_rs.push_back( r );
    }
}


void LINE_RESTRICTIONS::Dump()
{
}


bool LINE_RESTRICTIONS::Check( int aVertex1, int aVertex2, const SHAPE_LINE_CHAIN& aReplacement )
{
    if( m_rs.empty( ) )
        return true;

    for( int i = aVertex1; i <= aVertex2; i++ )
        if ( m_rs[i].restricted )
            return false;

    const RVERTEX& v1 = m_rs[ aVertex1 ];
    const RVERTEX& v2 = m_rs[ aVertex2 ];

    int m1 = DIRECTION_45( aReplacement.CSegment( 0 ) ).Mask();
    int m2;

    if( aReplacement.SegmentCount() == 1 )
        m2 = m1;
    else
        m2 = DIRECTION_45( aReplacement.CSegment( 1 ) ).Mask();

    return ( ( v1.allowedAngles & m1 ) != 0 ) &&
           ( ( v2.allowedAngles & m2 ) != 0 );
}


bool OPTIMIZER::checkColliding( ITEM* aItem, bool aUpdateCache )
{
    CACHE_VISITOR v( aItem, m_world, m_collisionKindMask );

    return static_cast<bool>( m_world->CheckColliding( aItem ) );

#if 0
    // something is wrong with the cache, need to investigate.
    m_cache.Query( aItem->Shape(), m_world->GetMaxClearance(), v, false );

    if( !v.m_collidingItem )
    {
        NODE::OPT_OBSTACLE obs = m_world->CheckColliding( aItem );

        if( obs )
        {
            if( aUpdateCache )
                cacheAdd( obs->m_item );

            return true;
        }
    }
    else
    {
        m_cacheTags[v.m_collidingItem].m_hits++;
        return true;
    }

    return false;
#endif
}


bool OPTIMIZER::checkColliding( LINE* aLine, const SHAPE_LINE_CHAIN& aOptPath )
{
    LINE tmp( *aLine, aOptPath );

    return checkColliding( &tmp );
}


bool OPTIMIZER::mergeObtuse( LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();

    int step = line.PointCount() - 3;
    int iter = 0;
    int segs_pre = line.SegmentCount();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( line );

    while( 1 )
    {
        iter++;
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 2 )
        {
            line = current_path;
            return current_path.SegmentCount() < segs_pre;
        }

        bool found_anything = false;
        int n = 0;

        while( n < n_segs - step )
        {
            const SEG s1 = current_path.CSegment( n );
            const SEG s2 = current_path.CSegment( n + step );
            SEG s1opt, s2opt;

            if( DIRECTION_45( s1 ).IsObtuse( DIRECTION_45( s2 ) ) )
            {
                VECTOR2I ip = *s1.IntersectLines( s2 );

                if( s1.Distance( ip ) <= 1 || s2.Distance( ip ) <= 1 )
                {
                    s1opt = SEG( s1.A, ip );
                    s2opt = SEG( ip, s2.B );
                }
                else
                {
                    s1opt = SEG( s1.A, ip );
                    s2opt = SEG( ip, s2.B );
                }

                if( DIRECTION_45( s1opt ).IsObtuse( DIRECTION_45( s2opt ) ) )
                {
                    SHAPE_LINE_CHAIN opt_path;
                    opt_path.Append( s1opt.A );
                    opt_path.Append( s1opt.B );
                    opt_path.Append( s2opt.B );

                    LINE opt_track( *aLine, opt_path );

                    if( !checkColliding( &opt_track ) )
                    {
                        current_path.Replace( s1.Index() + 1, s2.Index(), ip );
                        // removeCachedSegments(aLine, s1.Index(), s2.Index());
                        n_segs = current_path.SegmentCount();
                        found_anything = true;
                        break;
                    }
                }
            }

            n++;
        }

        if( !found_anything )
        {
            if( step <= 2 )
            {
                line = current_path;
                return line.SegmentCount() < segs_pre;
            }

            step--;
        }
    }

    return line.SegmentCount() < segs_pre;
}


bool OPTIMIZER::mergeFull( LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();
    int step = line.SegmentCount() - 1;

    int segs_pre = line.SegmentCount();

    line.Simplify();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( line );

    while( 1 )
    {
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 1 )
            break;

        bool found_anything = mergeStep( aLine, current_path, step );

        if( !found_anything )
            step--;
    }

    aLine->SetShape( current_path );

    return current_path.SegmentCount() < segs_pre;
}


bool OPTIMIZER::Optimize( LINE* aLine, LINE* aResult )
{
    if( !aResult )
        aResult = aLine;
    else
        *aResult = *aLine;

    m_keepPostures = false;

    bool rv = false;

    if( m_effortLevel & MERGE_SEGMENTS )
        rv |= mergeFull( aResult );

    if( m_effortLevel & MERGE_OBTUSE )
        rv |= mergeObtuse( aResult );

    if( m_effortLevel & SMART_PADS )
        rv |= runSmartPads( aResult );

    if( m_effortLevel & FANOUT_CLEANUP )
        rv |= fanoutCleanup( aResult );

    return rv;
}


bool OPTIMIZER::mergeStep( LINE* aLine, SHAPE_LINE_CHAIN& aCurrentPath, int step )
{
    int n = 0;
    int n_segs = aCurrentPath.SegmentCount();

    int cost_orig = COST_ESTIMATOR::CornerCost( aCurrentPath );

    LINE_RESTRICTIONS restr;

    if( aLine->SegmentCount() < 4 )
        return false;

    DIRECTION_45 orig_start( aLine->CSegment( 0 ) );
    DIRECTION_45 orig_end( aLine->CSegment( -1 ) );

    restr.Build( m_world, aLine, aCurrentPath, m_restrictArea, m_restrictAreaActive );

    while( n < n_segs - step )
    {
        const SEG s1    = aCurrentPath.CSegment( n );
        const SEG s2    = aCurrentPath.CSegment( n + step );

        SHAPE_LINE_CHAIN path[2];
        SHAPE_LINE_CHAIN* picked = NULL;
        int cost[2];

        for( int i = 0; i < 2; i++ )
        {
            bool postureMatch = true;
            SHAPE_LINE_CHAIN bypass = DIRECTION_45().BuildInitialTrace( s1.A, s2.B, i );
            cost[i] = INT_MAX;

            bool restrictionsOK = restr.Check ( n, n + step + 1, bypass );

            if( n == 0 && orig_start != DIRECTION_45( bypass.CSegment( 0 ) ) )
                postureMatch = false;
            else if( n == n_segs - step && orig_end != DIRECTION_45( bypass.CSegment( -1 ) ) )
                postureMatch = false;

            if( restrictionsOK && (postureMatch || !m_keepPostures) && !checkColliding( aLine, bypass ) )
            {
                path[i] = aCurrentPath;
                path[i].Replace( s1.Index(), s2.Index(), bypass );
                path[i].Simplify();
                cost[i] = COST_ESTIMATOR::CornerCost( path[i] );
            }
        }

        if( cost[0] < cost_orig && cost[0] < cost[1] )
            picked = &path[0];
        else if( cost[1] < cost_orig )
            picked = &path[1];

        if( picked )
        {
            n_segs = aCurrentPath.SegmentCount();
            aCurrentPath = *picked;
            return true;
        }

        n++;
    }

    return false;
}


OPTIMIZER::BREAKOUT_LIST OPTIMIZER::circleBreakouts( int aWidth,
        const SHAPE* aShape, bool aPermitDiagonal ) const
{
    BREAKOUT_LIST breakouts;

    for( int angle = 0; angle < 360; angle += 45 )
    {
        const SHAPE_CIRCLE* cir = static_cast<const SHAPE_CIRCLE*>( aShape );
        SHAPE_LINE_CHAIN l;
        VECTOR2I p0 = cir->GetCenter();
        VECTOR2I v0( cir->GetRadius() * M_SQRT2, 0 );
        l.Append( p0 );
        l.Append( p0 + v0.Rotate( angle * M_PI / 180.0 ) );
        breakouts.push_back( l );
    }

    return breakouts;
}


OPTIMIZER::BREAKOUT_LIST OPTIMIZER::customBreakouts( int aWidth,
        const ITEM* aItem, bool aPermitDiagonal ) const
{
    BREAKOUT_LIST breakouts;
    const SHAPE_SIMPLE* convex = static_cast<const SHAPE_SIMPLE*>( aItem->Shape() );

    BOX2I bbox = convex->BBox( 0 );
    VECTOR2I p0 = static_cast<const SOLID*>( aItem )->Pos();
    // must be large enough to guarantee intersecting the convex polygon
    int length = bbox.GetSize().EuclideanNorm() / 2 + 5;

    for( int angle = 0; angle < 360; angle += ( aPermitDiagonal ? 45 : 90 ) )
    {
        SHAPE_LINE_CHAIN l;
        VECTOR2I v0( p0 + VECTOR2I( length, 0 ).Rotate( angle * M_PI / 180.0 ) );
        SHAPE_LINE_CHAIN::INTERSECTIONS intersections;
        int n = convex->Vertices().Intersect( SEG( p0, v0 ), intersections );
        // if n == 1 intersected a segment
        // if n == 2 intersected the common point of 2 segments
        // n == 0 can not happen I think, but...
        if( n > 0 )
        {
            l.Append( p0 );

            // for a breakout distance relative to the distance between
            // center and polygon edge
            //l.Append( intersections[0].p + (v0 - p0).Resize( (intersections[0].p - p0).EuclideanNorm() * 0.4 ) );

            // for an absolute breakout distance, e.g. 0.1 mm
            l.Append( intersections[0].p + (v0 - p0).Resize( 100000 ) );

            // for the breakout right on the polygon edge
            //l.Append( intersections[0].p );

            breakouts.push_back( l );
        }
    }

    return breakouts;
}


OPTIMIZER::BREAKOUT_LIST OPTIMIZER::rectBreakouts( int aWidth,
        const SHAPE* aShape, bool aPermitDiagonal ) const
{
    const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>(aShape);
    VECTOR2I s = rect->GetSize(), c = rect->GetPosition() + VECTOR2I( s.x / 2, s.y / 2 );
    BREAKOUT_LIST breakouts;

    VECTOR2I d_offset;

    d_offset.x = ( s.x > s.y ) ? ( s.x - s.y ) / 2 : 0;
    d_offset.y = ( s.x < s.y ) ? ( s.y - s.x ) / 2 : 0;

    VECTOR2I d_vert  = VECTOR2I( 0, s.y / 2 + aWidth );
    VECTOR2I d_horiz = VECTOR2I( s.x / 2 + aWidth, 0 );

    breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_horiz ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_horiz ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_vert ) );
    breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_vert ) );

    if( aPermitDiagonal )
    {
        int l = aWidth + std::min( s.x, s.y ) / 2;
        VECTOR2I d_diag;

        if( s.x >= s.y )
        {
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset - VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset + VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( l, l ) ) );
        }
        else
        {
            // fixme: this could be done more efficiently
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c + d_offset,
                                                   c + d_offset + VECTOR2I( -l, l ) ) );
            breakouts.push_back( SHAPE_LINE_CHAIN( c, c - d_offset,
                                                   c - d_offset - VECTOR2I( l, l ) ) );
        }
    }

    return breakouts;
}


OPTIMIZER::BREAKOUT_LIST OPTIMIZER::computeBreakouts( int aWidth,
        const ITEM* aItem, bool aPermitDiagonal ) const
{
    switch( aItem->Kind() )
    {
    case ITEM::VIA_T:
    {
        const VIA* via = static_cast<const VIA*>( aItem );
        return circleBreakouts( aWidth, via->Shape(), aPermitDiagonal );
    }

    case ITEM::SOLID_T:
    {
        const SHAPE* shape = aItem->Shape();

        switch( shape->Type() )
        {
        case SH_RECT:
            return rectBreakouts( aWidth, shape, aPermitDiagonal );

        case SH_SEGMENT:
        {
            const SHAPE_SEGMENT* seg = static_cast<const SHAPE_SEGMENT*> (shape);
            const SHAPE_RECT rect = ApproximateSegmentAsRect ( *seg );
            return rectBreakouts( aWidth, &rect, aPermitDiagonal );
        }

        case SH_CIRCLE:
            return circleBreakouts( aWidth, shape, aPermitDiagonal );

        case SH_SIMPLE:
            return customBreakouts( aWidth, aItem, aPermitDiagonal );

        default:
            break;
        }

        break;
    }

    default:
        break;
    }

    return BREAKOUT_LIST();
}


ITEM* OPTIMIZER::findPadOrVia( int aLayer, int aNet, const VECTOR2I& aP ) const
{
    JOINT* jt = m_world->FindJoint( aP, aLayer, aNet );

    if( !jt )
        return NULL;

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::VIA_T | ITEM::SOLID_T ) )
            return item;
    }

    return NULL;
}


int OPTIMIZER::smartPadsSingle( LINE* aLine, ITEM* aPad, bool aEnd, int aEndVertex )
{
    int min_cost = INT_MAX; // COST_ESTIMATOR::CornerCost( line );
    int min_len = INT_MAX;
    DIRECTION_45 dir;

    const int ForbiddenAngles = DIRECTION_45::ANG_ACUTE | DIRECTION_45::ANG_RIGHT |
                                DIRECTION_45::ANG_HALF_FULL | DIRECTION_45::ANG_UNDEFINED;

    typedef std::pair<int, SHAPE_LINE_CHAIN> RtVariant;
    std::vector<RtVariant> variants;

    SOLID* solid = dynamic_cast<SOLID*>( aPad );

    // don't do optimized connections for offset pads
    if( solid && solid->Offset() != VECTOR2I( 0, 0 ) )
        return -1;


    BREAKOUT_LIST breakouts = computeBreakouts( aLine->Width(), aPad, true );

    SHAPE_LINE_CHAIN line = ( aEnd ? aLine->CLine().Reverse() : aLine->CLine() );


    int p_end = std::min( aEndVertex, std::min( 3, line.PointCount() - 1 ) );

    for( int p = 1; p <= p_end; p++ )
    {
        for( SHAPE_LINE_CHAIN & l : breakouts ) {

            for( int diag = 0; diag < 2; diag++ )
            {
                SHAPE_LINE_CHAIN v;
                SHAPE_LINE_CHAIN connect = dir.BuildInitialTrace( l.CPoint( -1 ),
                                                                  line.CPoint( p ), diag == 0 );

                DIRECTION_45 dir_bkout( l.CSegment( -1 ) );

                if(!connect.SegmentCount())
                    continue;

                int ang1 = dir_bkout.Angle( DIRECTION_45( connect.CSegment( 0 ) ) );
                int ang2 = 0;

                if( (ang1 | ang2) & ForbiddenAngles )
                    continue;

                if( l.Length() > line.Length() )
                    continue;

                v = l;

                v.Append( connect );

                for( int i = p + 1; i < line.PointCount(); i++ )
                    v.Append( line.CPoint( i ) );

                LINE tmp( *aLine, v );
                int cc = tmp.CountCorners( ForbiddenAngles );

                if( cc == 0 )
                {
                    RtVariant vp;
                    vp.first = p;
                    vp.second = aEnd ? v.Reverse() : v;
                    vp.second.Simplify();
                    variants.push_back( vp );
                }
            }
        }
    }

    SHAPE_LINE_CHAIN l_best;
    bool found = false;
    int p_best = -1;

    for( RtVariant& vp : variants )
    {
        LINE tmp( *aLine, vp.second );
        int cost = COST_ESTIMATOR::CornerCost( vp.second );
        int len = vp.second.Length();

        if( !checkColliding( &tmp ) )
        {
            if( cost < min_cost || ( cost == min_cost && len < min_len ) )
            {
                l_best = vp.second;
                p_best = vp.first;
                found  = true;

                if( cost == min_cost )
                    min_len = std::min( len, min_len );

                min_cost = std::min( cost, min_cost );
            }
        }
    }

    if( found )
    {
        aLine->SetShape( l_best );
        return p_best;
    }

    return -1;
}

bool OPTIMIZER::runSmartPads( LINE* aLine )
{
    SHAPE_LINE_CHAIN& line = aLine->Line();

    if( line.PointCount() < 3 )
        return false;

    VECTOR2I p_start = line.CPoint( 0 ), p_end = line.CPoint( -1 );

    ITEM* startPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_start );
    ITEM* endPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_end );

    int vtx = -1;

    if( startPad )
        vtx = smartPadsSingle( aLine, startPad, false, 3 );

    if( endPad )
        smartPadsSingle( aLine, endPad, true,
                         vtx < 0 ? line.PointCount() - 1 : line.PointCount() - 1 - vtx );

    aLine->Line().Simplify();

    return true;
}


bool OPTIMIZER::Optimize( LINE* aLine, int aEffortLevel, NODE* aWorld )
{
    OPTIMIZER opt( aWorld );

    opt.SetEffortLevel( aEffortLevel );
    opt.SetCollisionMask( -1 );
    return opt.Optimize( aLine );
}


bool OPTIMIZER::fanoutCleanup( LINE* aLine )
{
    if( aLine->PointCount() < 3 )
        return false;

    VECTOR2I p_start = aLine->CPoint( 0 ), p_end = aLine->CPoint( -1 );

    ITEM* startPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_start );
    ITEM* endPad = findPadOrVia( aLine->Layer(), aLine->Net(), p_end );

    int thr = aLine->Width() * 10;
    int len = aLine->CLine().Length();

    if( !startPad )
        return false;

    bool startMatch = startPad->OfKind( ITEM::VIA_T | ITEM::SOLID_T );
    bool endMatch = false;

    if(endPad)
    {
        endMatch = endPad->OfKind( ITEM::VIA_T | ITEM::SOLID_T );
    }
    else
    {
        endMatch = aLine->EndsWithVia();
    }

    if( startMatch && endMatch && len < thr )
    {
        for( int i = 0; i < 2; i++ )
        {
            SHAPE_LINE_CHAIN l2 = DIRECTION_45().BuildInitialTrace( p_start, p_end, i );
            LINE repl;
            repl = LINE( *aLine, l2 );

            if( !m_world->CheckColliding( &repl ) )
            {
                aLine->SetShape( repl.CLine() );
                return true;
            }
        }
    }

    return false;
}


int findCoupledVertices( const VECTOR2I& aVertex, const SEG& aOrigSeg, const SHAPE_LINE_CHAIN& aCoupled, DIFF_PAIR* aPair, int* aIndices )
{
    int count = 0;
    for ( int i = 0; i < aCoupled.SegmentCount(); i++ )
    {
        SEG s = aCoupled.CSegment( i );
        VECTOR2I projOverCoupled = s.LineProject ( aVertex );

        if( s.ApproxParallel ( aOrigSeg ) )
        {
            int64_t dist = ( projOverCoupled - aVertex ).EuclideanNorm() - aPair->Width();

            if( aPair->GapConstraint().Matches( dist ) )
            {
               *aIndices++ = i;
               count++;
            }
        }
    }

    return count;
}


bool verifyDpBypass( NODE* aNode, DIFF_PAIR* aPair, bool aRefIsP, const SHAPE_LINE_CHAIN& aNewRef, const SHAPE_LINE_CHAIN& aNewCoupled )
{
    LINE refLine ( aRefIsP ? aPair->PLine() : aPair->NLine(), aNewRef );
    LINE coupledLine ( aRefIsP ? aPair->NLine() : aPair->PLine(), aNewCoupled );

    if( aNode->CheckColliding( &refLine, &coupledLine, ITEM::ANY_T, aPair->Gap() - 10 ) )
        return false;

    if( aNode->CheckColliding ( &refLine ) )
        return false;

    if( aNode->CheckColliding ( &coupledLine ) )
        return false;

    return true;
}


bool coupledBypass( NODE* aNode, DIFF_PAIR* aPair, bool aRefIsP, const SHAPE_LINE_CHAIN& aRef, const SHAPE_LINE_CHAIN& aRefBypass, const SHAPE_LINE_CHAIN& aCoupled, SHAPE_LINE_CHAIN& aNewCoupled )
{
    int vStartIdx[1024]; // fixme: possible overflow

    int nStarts = findCoupledVertices( aRefBypass.CPoint( 0 ), aRefBypass.CSegment( 0 ), aCoupled, aPair, vStartIdx );
    DIRECTION_45 dir( aRefBypass.CSegment( 0 ) );

    int64_t bestLength = -1;
    bool found = false;
    SHAPE_LINE_CHAIN bestBypass;
    int si, ei;

    for( int i=0; i< nStarts; i++ )
    {
        for( int j = 1; j < aCoupled.PointCount() - 1; j++ )
        {
            int delta = std::abs ( vStartIdx[i] - j );

            if( delta > 1 )
            {
                const VECTOR2I& vs = aCoupled.CPoint( vStartIdx[i] );
                SHAPE_LINE_CHAIN bypass = dir.BuildInitialTrace( vs, aCoupled.CPoint(j), dir.IsDiagonal() );

                int64_t coupledLength = aPair->CoupledLength( aRef, bypass );

                SHAPE_LINE_CHAIN newCoupled = aCoupled;

                si = vStartIdx[i];
                ei = j;

                if(si < ei)
                    newCoupled.Replace( si, ei, bypass );
                else
                    newCoupled.Replace( ei, si, bypass.Reverse() );

                if(coupledLength > bestLength && verifyDpBypass( aNode, aPair, aRefIsP, aRef, newCoupled) )
                {
                    bestBypass = newCoupled;
                    bestLength = coupledLength;
                    found = true;
                }
            }
        }
    }


    if( found )
        aNewCoupled = bestBypass;

    return found;
}


bool checkDpColliding( NODE* aNode, DIFF_PAIR* aPair, bool aIsP, const SHAPE_LINE_CHAIN& aPath )
{
    LINE tmp ( aIsP ? aPair->PLine() : aPair->NLine(), aPath );

    return static_cast<bool>( aNode->CheckColliding( &tmp ) );
}


bool OPTIMIZER::mergeDpStep( DIFF_PAIR* aPair, bool aTryP, int step )
{
    int n = 1;

    SHAPE_LINE_CHAIN currentPath = aTryP ? aPair->CP() : aPair->CN();
    SHAPE_LINE_CHAIN coupledPath = aTryP ? aPair->CN() : aPair->CP();

    int n_segs = currentPath.SegmentCount() - 1;

    int64_t clenPre = aPair->CoupledLength( currentPath, coupledPath );
    int64_t budget = clenPre / 10; // fixme: come up with somethig more intelligent here...

    while( n < n_segs - step )
    {
        const SEG s1    = currentPath.CSegment( n );
        const SEG s2    = currentPath.CSegment( n + step );

        DIRECTION_45 dir1( s1 );
        DIRECTION_45 dir2( s2 );

        if( dir1.IsObtuse( dir2 ) )
        {
            SHAPE_LINE_CHAIN bypass = DIRECTION_45().BuildInitialTrace( s1.A, s2.B, dir1.IsDiagonal() );
            SHAPE_LINE_CHAIN newRef;
            SHAPE_LINE_CHAIN newCoup;
            int64_t deltaCoupled = -1, deltaUni = -1;

            newRef = currentPath;
            newRef.Replace( s1.Index(), s2.Index(), bypass );

            deltaUni = aPair->CoupledLength ( newRef, coupledPath ) - clenPre + budget;

            if ( coupledBypass( m_world, aPair, aTryP, newRef, bypass, coupledPath, newCoup ) )
            {
                deltaCoupled = aPair->CoupledLength( newRef, newCoup ) - clenPre + budget;

                if( deltaCoupled >= 0 )
                {
                    newRef.Simplify();
                    newCoup.Simplify();

                    aPair->SetShape( newRef, newCoup, !aTryP );
                    return true;
                }
            }
            else if( deltaUni >= 0 &&  verifyDpBypass ( m_world, aPair, aTryP, newRef, coupledPath ) )
            {
                newRef.Simplify();
                coupledPath.Simplify();

                aPair->SetShape( newRef, coupledPath, !aTryP );
                return true;
            }
        }

        n++;
    }

    return false;
}


bool OPTIMIZER::mergeDpSegments( DIFF_PAIR* aPair )
{
    int step_p = aPair->CP().SegmentCount() - 2;
    int step_n = aPair->CN().SegmentCount() - 2;

    while( 1 )
    {
        int n_segs_p = aPair->CP().SegmentCount();
        int n_segs_n = aPair->CN().SegmentCount();

        int max_step_p = n_segs_p - 2;
        int max_step_n = n_segs_n - 2;

        if( step_p > max_step_p )
            step_p = max_step_p;

        if( step_n > max_step_n )
            step_n = max_step_n;

        if( step_p < 1 && step_n < 1)
            break;

        bool found_anything_p = false;
        bool found_anything_n = false;

        if( step_p > 1 )
            found_anything_p = mergeDpStep( aPair, true, step_p );

        if( step_n > 1 )
            found_anything_n = mergeDpStep( aPair, false, step_n );

        if( !found_anything_n && !found_anything_p )
        {
            step_n--;
            step_p--;
        }
    }
    return true;
}


bool OPTIMIZER::Optimize( DIFF_PAIR* aPair )
{
    return mergeDpSegments( aPair );
}

}
