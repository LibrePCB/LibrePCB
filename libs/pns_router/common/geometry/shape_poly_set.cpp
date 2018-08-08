/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 *
 * Point in polygon algorithm adapted from Clipper Library (C) Angus Johnson,
 * subject to Clipper library license.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <vector>
#include <cstdio>
#include <set>
#include <list>
#include <algorithm>
#include <unordered_set>

#include <common.h>
#include <md5_hash.h>
#include <map>

#include <geometry/geometry_utils.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include "poly2tri/poly2tri.h"

using namespace ClipperLib;

SHAPE_POLY_SET::SHAPE_POLY_SET() :
    SHAPE( SH_POLY_SET )
{
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther ) :
    SHAPE( SH_POLY_SET ), m_polys( aOther.m_polys )
{
}


SHAPE_POLY_SET::~SHAPE_POLY_SET()
{
}


SHAPE* SHAPE_POLY_SET::Clone() const
{
    return new SHAPE_POLY_SET( *this );
}


bool SHAPE_POLY_SET::GetRelativeIndices( int aGlobalIdx,
        SHAPE_POLY_SET::VERTEX_INDEX* aRelativeIndices ) const
{
    int polygonIdx = 0;
    unsigned int contourIdx = 0;
    int vertexIdx = 0;

    int currentGlobalIdx = 0;

    for( polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        const POLYGON currentPolygon = CPolygon( polygonIdx );

        for( contourIdx = 0; contourIdx < currentPolygon.size(); contourIdx++ )
        {
            SHAPE_LINE_CHAIN currentContour = currentPolygon[contourIdx];
            int totalPoints = currentContour.PointCount();

            for( vertexIdx = 0; vertexIdx < totalPoints; vertexIdx++ )
            {
                // Check if the current vertex is the globally indexed as aGlobalIdx
                if( currentGlobalIdx == aGlobalIdx )
                {
                    aRelativeIndices->m_polygon = polygonIdx;
                    aRelativeIndices->m_contour = contourIdx;
                    aRelativeIndices->m_vertex  = vertexIdx;

                    return true;
                }

                // Advance
                currentGlobalIdx++;
            }
        }
    }

    return false;
}


bool SHAPE_POLY_SET::GetGlobalIndex( SHAPE_POLY_SET::VERTEX_INDEX aRelativeIndices,
        int& aGlobalIdx )
{
    int selectedVertex = aRelativeIndices.m_vertex;
    unsigned int    selectedContour = aRelativeIndices.m_contour;
    unsigned int    selectedPolygon = aRelativeIndices.m_polygon;

    // Check whether the vertex indices make sense in this poly set
    if( selectedPolygon < m_polys.size() && selectedContour < m_polys[selectedPolygon].size()
        && selectedVertex < m_polys[selectedPolygon][selectedContour].PointCount() )
    {
        POLYGON currentPolygon;

        aGlobalIdx = 0;

        for( unsigned int polygonIdx = 0; polygonIdx < selectedPolygon; polygonIdx++ )
        {
            currentPolygon = Polygon( polygonIdx );

            for( unsigned int contourIdx = 0; contourIdx < currentPolygon.size(); contourIdx++ )
            {
                aGlobalIdx += currentPolygon[contourIdx].PointCount();
            }
        }

        currentPolygon = Polygon( selectedPolygon );

        for( unsigned int contourIdx = 0; contourIdx < selectedContour; contourIdx++ )
        {
            aGlobalIdx += currentPolygon[contourIdx].PointCount();
        }

        aGlobalIdx += selectedVertex;

        return true;
    }
    else
    {
        return false;
    }
}


int SHAPE_POLY_SET::NewOutline()
{
    SHAPE_LINE_CHAIN empty_path;
    POLYGON poly;

    empty_path.SetClosed( true );
    poly.push_back( empty_path );
    m_polys.push_back( poly );
    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::NewHole( int aOutline )
{
    SHAPE_LINE_CHAIN empty_path;

    empty_path.SetClosed( true );

    // Default outline is the last one
    if( aOutline < 0 )
        aOutline += m_polys.size();

    // Add hole to the selected outline
    m_polys[aOutline].push_back( empty_path );

    return m_polys.back().size() - 2;
}


int SHAPE_POLY_SET::Append( int x, int y, int aOutline, int aHole, bool aAllowDuplication )
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    m_polys[aOutline][idx].Append( x, y, aAllowDuplication );

    return m_polys[aOutline][idx].PointCount();
}


void SHAPE_POLY_SET::InsertVertex( int aGlobalIndex, VECTOR2I aNewVertex )
{
    VERTEX_INDEX index;

    if( aGlobalIndex < 0 )
        aGlobalIndex = 0;

    if( aGlobalIndex >= TotalVertices() )
    {
        Append( aNewVertex );
    }
    else
    {
        // Assure the position to be inserted exists; throw an exception otherwise
        if( GetRelativeIndices( aGlobalIndex, &index ) )
            m_polys[index.m_polygon][index.m_contour].Insert( index.m_vertex, aNewVertex );
        else
            throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );
    }
}


int SHAPE_POLY_SET::VertexCount( int aOutline, int aHole  ) const
{
    if( m_polys.size() == 0 ) // Empty poly set
        return 0;

    if( aOutline < 0 ) // Use last outline
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    if( aOutline >= (int) m_polys.size() ) // not existing outline
        return 0;

    if( idx >= (int) m_polys[aOutline].size() ) // not existing hole
        return 0;

    return m_polys[aOutline][idx].PointCount();
}


SHAPE_POLY_SET SHAPE_POLY_SET::Subset( int aFirstPolygon, int aLastPolygon )
{
    assert( aFirstPolygon >= 0 && aLastPolygon <= OutlineCount() );

    SHAPE_POLY_SET newPolySet;

    for( int index = aFirstPolygon; index < aLastPolygon; index++ )
    {
        newPolySet.m_polys.push_back( Polygon( index ) );
    }

    return newPolySet;
}


VECTOR2I& SHAPE_POLY_SET::Vertex( int aIndex, int aOutline, int aHole )
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    return m_polys[aOutline][idx].Point( aIndex );
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int aIndex, int aOutline, int aHole ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    return m_polys[aOutline][idx].CPoint( aIndex );
}


VECTOR2I& SHAPE_POLY_SET::Vertex( int aGlobalIndex )
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // Assure the passed index references a legal position; abort otherwise
    if( !GetRelativeIndices( aGlobalIndex, &index ) )
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );

    return m_polys[index.m_polygon][index.m_contour].Point( index.m_vertex );
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int aGlobalIndex ) const
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // Assure the passed index references a legal position; abort otherwise
    if( !GetRelativeIndices( aGlobalIndex, &index ) )
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );

    return m_polys[index.m_polygon][index.m_contour].CPoint( index.m_vertex );
}


VECTOR2I& SHAPE_POLY_SET::Vertex( SHAPE_POLY_SET::VERTEX_INDEX index )
{
    return Vertex( index.m_vertex, index.m_polygon, index.m_contour - 1 );
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( SHAPE_POLY_SET::VERTEX_INDEX index ) const
{
    return CVertex( index.m_vertex, index.m_polygon, index.m_contour - 1 );
}


bool SHAPE_POLY_SET::GetNeighbourIndexes( int aGlobalIndex, int* aPrevious, int* aNext )
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // If the edge does not exist, throw an exception, it is an illegal access memory error
    if( !GetRelativeIndices( aGlobalIndex, &index ) )
        return false;

    // Calculate the previous and next index of aGlobalIndex, corresponding to
    // the same contour;
    VERTEX_INDEX inext = index;
    int lastpoint = m_polys[index.m_polygon][index.m_contour].SegmentCount();

    if( index.m_vertex == 0 )
    {
        index.m_vertex  = lastpoint;
        inext.m_vertex  = 1;
    }
    else if( index.m_vertex == lastpoint )
    {
        index.m_vertex--;
        inext.m_vertex = 0;
    }
    else
    {
        inext.m_vertex++;
        index.m_vertex--;
    }

    if( aPrevious )
    {
        int previous;
        GetGlobalIndex( index, previous );
        *aPrevious = previous;
    }

    if( aNext )
    {
        int next;
        GetGlobalIndex( inext, next );
        *aNext = next;
    }

    return true;
}


bool SHAPE_POLY_SET::IsPolygonSelfIntersecting( int aPolygonIndex )
{
    SEGMENT_ITERATOR iterator = IterateSegmentsWithHoles( aPolygonIndex );
    SEGMENT_ITERATOR innerIterator;

    for( iterator = IterateSegmentsWithHoles( aPolygonIndex ); iterator; iterator++ )
    {
        SEG firstSegment = *iterator;

        // Iterate through all remaining segments.
        innerIterator = iterator;

        // Start in the next segment, we don't want to check collision between a segment and itself
        for( innerIterator++; innerIterator; innerIterator++ )
        {
            SEG secondSegment = *innerIterator;

            // Check whether the two segments built collide, only when they are not adjacent.
            if( !iterator.IsAdjacent( innerIterator ) && firstSegment.Collide( secondSegment, 0 ) )
                return true;
        }
    }

    return false;
}


bool SHAPE_POLY_SET::IsSelfIntersecting()
{
    for( unsigned int polygon = 0; polygon < m_polys.size(); polygon++ )
    {
        if( IsPolygonSelfIntersecting( polygon ) )
            return true;
    }

    return false;
}


int SHAPE_POLY_SET::AddOutline( const SHAPE_LINE_CHAIN& aOutline )
{
    assert( aOutline.IsClosed() );

    POLYGON poly;

    poly.push_back( aOutline );

    m_polys.push_back( poly );

    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline )
{
    assert( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    POLYGON& poly = m_polys[aOutline];

    assert( poly.size() );

    poly.push_back( aHole );

    return poly.size() - 1;
}


const Path SHAPE_POLY_SET::convertToClipper( const SHAPE_LINE_CHAIN& aPath,
        bool aRequiredOrientation )
{
    Path c_path;

    for( int i = 0; i < aPath.PointCount(); i++ )
    {
        const VECTOR2I& vertex = aPath.CPoint( i );
        c_path.push_back( IntPoint( vertex.x, vertex.y ) );
    }

    if( Orientation( c_path ) != aRequiredOrientation )
        ReversePath( c_path );

    return c_path;
}


const SHAPE_LINE_CHAIN SHAPE_POLY_SET::convertFromClipper( const Path& aPath )
{
    SHAPE_LINE_CHAIN lc;

    for( unsigned int i = 0; i < aPath.size(); i++ )
        lc.Append( aPath[i].X, aPath[i].Y );

    lc.SetClosed( true );

    return lc;
}


void SHAPE_POLY_SET::booleanOp( ClipperLib::ClipType aType, const SHAPE_POLY_SET& aOtherShape,
        POLYGON_MODE aFastMode )
{
    Clipper c;

    if( aFastMode == PM_STRICTLY_SIMPLE )
        c.StrictlySimple( true );

    for( const POLYGON& poly : m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptSubject, true );
    }

    for( const POLYGON& poly : aOtherShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptClip, true );
    }

    PolyTree solution;

    c.Execute( aType, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::booleanOp( ClipperLib::ClipType aType,
        const SHAPE_POLY_SET& aShape,
        const SHAPE_POLY_SET& aOtherShape,
        POLYGON_MODE aFastMode )
{
    Clipper c;

    if( aFastMode == PM_STRICTLY_SIMPLE )
        c.StrictlySimple( true );

    for( const POLYGON& poly : aShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptSubject, true );
    }

    for( const POLYGON& poly : aOtherShape.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptClip, true );
    }

    PolyTree solution;

    c.Execute( aType, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctUnion, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctDifference, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode )
{
    booleanOp( ctIntersection, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& a,
        const SHAPE_POLY_SET& b,
        POLYGON_MODE aFastMode )
{
    booleanOp( ctUnion, a, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& a,
        const SHAPE_POLY_SET& b,
        POLYGON_MODE aFastMode )
{
    booleanOp( ctDifference, a, b, aFastMode );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& a,
        const SHAPE_POLY_SET& b,
        POLYGON_MODE aFastMode )
{
    booleanOp( ctIntersection, a, b, aFastMode );
}


void SHAPE_POLY_SET::Inflate( int aFactor, int aCircleSegmentsCount )
{
    // A static table to avoid repetitive calculations of the coefficient
    // 1.0 - cos( M_PI/aCircleSegmentsCount)
    // aCircleSegmentsCount is most of time <= 64 and usually 8, 12, 16, 32
    #define SEG_CNT_MAX 64
    static double arc_tolerance_factor[SEG_CNT_MAX + 1];

    ClipperOffset c;

    for( const POLYGON& poly : m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), jtRound,
                    etClosedPolygon );
    }

    PolyTree solution;

    // Calculate the arc tolerance (arc error) from the seg count by circle.
    // the seg count is nn = M_PI / acos(1.0 - c.ArcTolerance / abs(aFactor))
    // see:
    // www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/ClipperOffset/Properties/ArcTolerance.htm

    if( aCircleSegmentsCount < 6 ) // avoid incorrect aCircleSegmentsCount values
        aCircleSegmentsCount = 6;

    double coeff;

    if( aCircleSegmentsCount > SEG_CNT_MAX || arc_tolerance_factor[aCircleSegmentsCount] == 0 )
    {
        coeff = 1.0 - cos( M_PI / aCircleSegmentsCount );

        if( aCircleSegmentsCount <= SEG_CNT_MAX )
            arc_tolerance_factor[aCircleSegmentsCount] = coeff;
    }
    else
        coeff = arc_tolerance_factor[aCircleSegmentsCount];

    c.ArcTolerance = std::abs( aFactor ) * coeff;

    c.Execute( solution, aFactor );

    importTree( &solution );
}


void SHAPE_POLY_SET::importTree( PolyTree* tree )
{
    m_polys.clear();

    for( PolyNode* n = tree->GetFirst(); n; n = n->GetNext() )
    {
        if( !n->IsHole() )
        {
            POLYGON paths;
            paths.reserve( n->Childs.size() + 1 );
            paths.push_back( convertFromClipper( n->Contour ) );

            for( unsigned int i = 0; i < n->Childs.size(); i++ )
                paths.push_back( convertFromClipper( n->Childs[i]->Contour ) );

            m_polys.push_back( paths );
        }
    }
}


struct FractureEdge
{
    FractureEdge( bool connected, SHAPE_LINE_CHAIN* owner, int index ) :
        m_connected( connected ),
        m_next( NULL )
    {
        m_p1    = owner->CPoint( index );
        m_p2    = owner->CPoint( index + 1 );
    }

    FractureEdge( int y = 0 ) :
        m_connected( false ),
        m_next( NULL )
    {
        m_p1.x = m_p2.y = y;
    }

    FractureEdge( bool connected, const VECTOR2I& p1, const VECTOR2I& p2 ) :
        m_connected( connected ),
        m_p1( p1 ),
        m_p2( p2 ),
        m_next( NULL )
    {
    }

    bool matches( int y ) const
    {
        int y_min   = std::min( m_p1.y, m_p2.y );
        int y_max   = std::max( m_p1.y, m_p2.y );

        return ( y >= y_min ) && ( y <= y_max );
    }

    bool m_connected;
    VECTOR2I m_p1, m_p2;
    FractureEdge* m_next;
};


typedef std::vector<FractureEdge*> FractureEdgeSet;


static int processEdge( FractureEdgeSet& edges, FractureEdge* edge )
{
    int x   = edge->m_p1.x;
    int y   = edge->m_p1.y;
    int min_dist    = std::numeric_limits<int>::max();
    int x_nearest   = 0;

    FractureEdge* e_nearest = NULL;

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
    {
        if( !(*i)->matches( y ) )
            continue;

        int x_intersect;

        if( (*i)->m_p1.y == (*i)->m_p2.y ) // horizontal edge
            x_intersect = std::max( (*i)->m_p1.x, (*i)->m_p2.x );
        else
            x_intersect = (*i)->m_p1.x + rescale( (*i)->m_p2.x - (*i)->m_p1.x, y - (*i)->m_p1.y,
                    (*i)->m_p2.y - (*i)->m_p1.y );

        int dist = ( x - x_intersect );

        if( dist >= 0 && dist < min_dist && (*i)->m_connected )
        {
            min_dist    = dist;
            x_nearest   = x_intersect;
            e_nearest   = (*i);
        }
    }

    if( e_nearest && e_nearest->m_connected )
    {
        int count = 0;

        FractureEdge* lead1 =
            new FractureEdge( true, VECTOR2I( x_nearest, y ), VECTOR2I( x, y ) );
        FractureEdge* lead2 =
            new FractureEdge( true, VECTOR2I( x, y ), VECTOR2I( x_nearest, y ) );
        FractureEdge* split_2 =
            new FractureEdge( true, VECTOR2I( x_nearest, y ), e_nearest->m_p2 );

        edges.push_back( split_2 );
        edges.push_back( lead1 );
        edges.push_back( lead2 );

        FractureEdge* link = e_nearest->m_next;

        e_nearest->m_p2 = VECTOR2I( x_nearest, y );
        e_nearest->m_next = lead1;
        lead1->m_next = edge;

        FractureEdge* last;

        for( last = edge; last->m_next != edge; last = last->m_next )
        {
            last->m_connected = true;
            count++;
        }

        last->m_connected = true;
        last->m_next    = lead2;
        lead2->m_next   = split_2;
        split_2->m_next = link;

        return count + 1;
    }

    return 0;
}


void SHAPE_POLY_SET::fractureSingle( POLYGON& paths )
{
    FractureEdgeSet edges;
    FractureEdgeSet border_edges;
    FractureEdge*   root = NULL;

    bool first = true;

    if( paths.size() == 1 )
        return;

    int num_unconnected = 0;

    for( SHAPE_LINE_CHAIN& path : paths )
    {
        int index = 0;

        FractureEdge* prev = NULL, * first_edge = NULL;

        int x_min = std::numeric_limits<int>::max();

        for( int i = 0; i < path.PointCount(); i++ )
        {
            const VECTOR2I& p = path.CPoint( i );

            if( p.x < x_min )
                x_min = p.x;
        }

        for( int i = 0; i < path.PointCount(); i++ )
        {
            FractureEdge* fe = new FractureEdge( first, &path, index++ );

            if( !root )
                root = fe;

            if( !first_edge )
                first_edge = fe;

            if( prev )
                prev->m_next = fe;

            if( i == path.PointCount() - 1 )
                fe->m_next = first_edge;

            prev = fe;
            edges.push_back( fe );

            if( !first )
            {
                if( fe->m_p1.x == x_min )
                    border_edges.push_back( fe );
            }

            if( !fe->m_connected )
                num_unconnected++;
        }

        first = false;    // first path is always the outline
    }

    // keep connecting holes to the main outline, until there's no holes left...
    while( num_unconnected > 0 )
    {
        int x_min = std::numeric_limits<int>::max();

        FractureEdge* smallestX = NULL;

        // find the left-most hole edge and merge with the outline
        for( FractureEdgeSet::iterator i = border_edges.begin(); i != border_edges.end(); ++i )
        {
            int xt = (*i)->m_p1.x;

            if( ( xt < x_min ) && !(*i)->m_connected )
            {
                x_min = xt;
                smallestX = *i;
            }
        }

        num_unconnected -= processEdge( edges, smallestX );
    }

    paths.clear();
    SHAPE_LINE_CHAIN newPath;

    newPath.SetClosed( true );

    FractureEdge* e;

    for( e = root; e->m_next != root; e = e->m_next )
        newPath.Append( e->m_p1 );

    newPath.Append( e->m_p1 );

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
        delete *i;

    paths.push_back( newPath );
}


void SHAPE_POLY_SET::Fracture( POLYGON_MODE aFastMode )
{
    Simplify( aFastMode );    // remove overlapping holes/degeneracy

    for( POLYGON& paths : m_polys )
    {
        fractureSingle( paths );
    }
}


void SHAPE_POLY_SET::unfractureSingle( SHAPE_POLY_SET::POLYGON& aPoly )
{
    assert( aPoly.size() == 1 );

    struct EDGE
    {
        int m_index = 0;
        SHAPE_LINE_CHAIN* m_poly = nullptr;
        bool m_duplicate = false;

        EDGE( SHAPE_LINE_CHAIN* aPolygon, int aIndex ) :
            m_index( aIndex ),
            m_poly( aPolygon )
        {}

        bool compareSegs( const SEG& s1, const SEG& s2 ) const
        {
            return (s1.A == s2.B && s1.B == s2.A);
        }

        bool operator==( const EDGE& aOther ) const
        {
            return compareSegs( m_poly->CSegment( m_index ),
                    aOther.m_poly->CSegment( aOther.m_index ) );
        }

        bool operator!=( const EDGE& aOther ) const
        {
            return !compareSegs( m_poly->CSegment( m_index ),
                    aOther.m_poly->CSegment( aOther.m_index ) );
        }

        struct HASH
        {
            std::size_t operator()(  const EDGE& aEdge ) const
            {
                const auto& a = aEdge.m_poly->CSegment( aEdge.m_index );

                return (std::size_t) ( a.A.x + a.B.x + a.A.y + a.B.y );
            }
        };
    };

    struct EDGE_LIST_ENTRY
    {
        int index;
        EDGE_LIST_ENTRY* next;
    };

    std::unordered_set<EDGE, EDGE::HASH> uniqueEdges;

    auto lc = aPoly[0];
    lc.Simplify();

    auto edgeList = std::make_unique<EDGE_LIST_ENTRY []>( lc.SegmentCount() );

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        edgeList[i].index   = i;
        edgeList[i].next    = &edgeList[ (i != lc.SegmentCount() - 1) ? i + 1 : 0 ];
    }

    std::unordered_set<EDGE_LIST_ENTRY*> queue;

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        EDGE e( &lc, i );
        uniqueEdges.insert( e );
    }

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        EDGE    e( &lc, i );
        auto    it = uniqueEdges.find( e );

        if( it != uniqueEdges.end() && it->m_index != i )
        {
            int e1  = it->m_index;
            int e2  = i;

            if( e1 > e2 )
                std::swap( e1, e2 );

            int e1_prev = e1 - 1;

            if( e1_prev < 0 )
                e1_prev = lc.SegmentCount() - 1;

            int e2_prev = e2 - 1;

            if( e2_prev < 0 )
                e2_prev = lc.SegmentCount() - 1;

            int e1_next = e1 + 1;

            if( e1_next == lc.SegmentCount() )
                e1_next = 0;

            int e2_next = e2 + 1;

            if( e2_next == lc.SegmentCount() )
                e2_next = 0;

            edgeList[e1_prev].next  = &edgeList[ e2_next ];
            edgeList[e2_prev].next  = &edgeList[ e1_next ];
            edgeList[i].next = nullptr;
            edgeList[it->m_index].next = nullptr;
        }
    }

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        if( edgeList[i].next )
            queue.insert( &edgeList[i] );
    }

    auto edgeBuf = std::make_unique<EDGE_LIST_ENTRY* []>( lc.SegmentCount() );

    int n = 0;
    int outline = -1;

    POLYGON result;

    while( queue.size() )
    {
        auto    e_first = (*queue.begin() );
        auto    e = e_first;
        int     cnt = 0;

        do {
            edgeBuf[cnt++] = e;
            e = e->next;
        } while( e && e != e_first );

        SHAPE_LINE_CHAIN outl;

        for( int i = 0; i < cnt; i++ )
        {
            auto p = lc.CPoint( edgeBuf[i]->index );
            outl.Append( p );
            queue.erase( edgeBuf[i] );
        }

        outl.SetClosed( true );

        bool cw = outl.Area() > 0.0;

        if( cw )
            outline = n;

        result.push_back( outl );
        n++;
    }

    if( outline > 0 )
        std::swap( result[0], result[outline] );

    aPoly = result;
}


bool SHAPE_POLY_SET::HasHoles() const
{
    // Iterate through all the polygons on the set
    for( const POLYGON& paths : m_polys )
    {
        // If any of them has more than one contour, it is a hole.
        if( paths.size() > 1 )
            return true;
    }

    // Return false if and only if every polygon has just one outline, without holes.
    return false;
}


void SHAPE_POLY_SET::Unfracture( POLYGON_MODE aFastMode )
{
    for( POLYGON& path : m_polys )
    {
        unfractureSingle( path );
    }

    Simplify( aFastMode );    // remove overlapping holes/degeneracy
}


void SHAPE_POLY_SET::Simplify( POLYGON_MODE aFastMode )
{
    SHAPE_POLY_SET empty;

    booleanOp( ctUnion, empty, aFastMode );
}


int SHAPE_POLY_SET::NormalizeAreaOutlines()
{
    // We are expecting only one main outline, but this main outline can have holes
    // if holes: combine holes and remove them from the main outline.
    // Note also we are using SHAPE_POLY_SET::PM_STRICTLY_SIMPLE in polygon
    // calculations, but it is not mandatory. It is used mainly
    // because there is usually only very few vertices in area outlines
    SHAPE_POLY_SET::POLYGON& outline = Polygon( 0 );
    SHAPE_POLY_SET holesBuffer;

    // Move holes stored in outline to holesBuffer:
    // The first SHAPE_LINE_CHAIN is the main outline, others are holes
    while( outline.size() > 1 )
    {
        holesBuffer.AddOutline( outline.back() );
        outline.pop_back();
    }

    Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    // If any hole, substract it to main outline
    if( holesBuffer.OutlineCount() )
    {
        holesBuffer.Simplify( SHAPE_POLY_SET::PM_FAST );
        BooleanSubtract( holesBuffer, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }

    RemoveNullSegments();

    return OutlineCount();
}


const std::string SHAPE_POLY_SET::Format() const
{
    std::stringstream ss;

    ss << "polyset " << m_polys.size() << "\n";

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        ss << "poly " << m_polys[i].size() << "\n";

        for( unsigned j = 0; j < m_polys[i].size(); j++ )
        {
            ss << m_polys[i][j].PointCount() << "\n";

            for( int v = 0; v < m_polys[i][j].PointCount(); v++ )
                ss << m_polys[i][j].CPoint( v ).x << " " << m_polys[i][j].CPoint( v ).y << "\n";
        }

        ss << "\n";
    }

    return ss.str();
}


bool SHAPE_POLY_SET::Parse( std::stringstream& aStream )
{
    std::string tmp;

    aStream >> tmp;

    if( tmp != "polyset" )
        return false;

    aStream >> tmp;

    int n_polys = atoi( tmp.c_str() );

    if( n_polys < 0 )
        return false;

    for( int i = 0; i < n_polys; i++ )
    {
        POLYGON paths;

        aStream >> tmp;

        if( tmp != "poly" )
            return false;

        aStream >> tmp;
        int n_outlines = atoi( tmp.c_str() );

        if( n_outlines < 0 )
            return false;

        for( int j = 0; j < n_outlines; j++ )
        {
            SHAPE_LINE_CHAIN outline;

            outline.SetClosed( true );

            aStream >> tmp;
            int n_vertices = atoi( tmp.c_str() );

            for( int v = 0; v < n_vertices; v++ )
            {
                VECTOR2I p;

                aStream >> tmp; p.x = atoi( tmp.c_str() );
                aStream >> tmp; p.y = atoi( tmp.c_str() );
                outline.Append( p );
            }

            paths.push_back( outline );
        }

        m_polys.push_back( paths );
    }

    return true;
}


const BOX2I SHAPE_POLY_SET::BBox( int aClearance ) const
{
    BOX2I bb;

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        if( i == 0 )
            bb = m_polys[i][0].BBox();
        else
            bb.Merge( m_polys[i][0].BBox() );
    }

    bb.Inflate( aClearance );
    return bb;
}


bool SHAPE_POLY_SET::PointOnEdge( const VECTOR2I& aP ) const
{
    // Iterate through all the polygons in the set
    for( const POLYGON& polygon : m_polys )
    {
        // Iterate through all the line chains in the polygon
        for( const SHAPE_LINE_CHAIN& lineChain : polygon )
        {
            if( lineChain.PointOnEdge( aP ) )
                return true;
        }
    }

    return false;
}


bool SHAPE_POLY_SET::Collide( const SEG& aSeg, int aClearance ) const
{

    SHAPE_POLY_SET polySet = SHAPE_POLY_SET( *this );

    // Inflate the polygon if necessary.
    if( aClearance > 0 )
    {
        // fixme: the number of arc segments should not be hardcoded
        polySet.Inflate( aClearance, 8 );
    }

    // We are going to check to see if the segment crosses an external
    // boundary.  However, if the full segment is inside the polyset, this
    // will not be true.  So we first test to see if one of the points is
    // inside.  If true, then we collide
    if( polySet.Contains( aSeg.A ) )
        return true;

    for( SEGMENT_ITERATOR iterator = polySet.IterateSegmentsWithHoles(); iterator; iterator++ )
    {
        SEG polygonEdge = *iterator;

        if( polygonEdge.Intersect( aSeg, true ) )
            return true;
    }

    return false;
}


bool SHAPE_POLY_SET::Collide( const VECTOR2I& aP, int aClearance ) const
{
    SHAPE_POLY_SET polySet = SHAPE_POLY_SET( *this );

    // Inflate the polygon if necessary.
    if( aClearance > 0 )
    {
        // fixme: the number of arc segments should not be hardcoded
        polySet.Inflate( aClearance, 8 );
    }

    // There is a collision if and only if the point is inside of the polygon.
    return polySet.Contains( aP );
}


void SHAPE_POLY_SET::RemoveAllContours()
{
    m_polys.clear();
}


void SHAPE_POLY_SET::RemoveContour( int aContourIdx, int aPolygonIdx )
{
    // Default polygon is the last one
    if( aPolygonIdx < 0 )
        aPolygonIdx += m_polys.size();

    m_polys[aPolygonIdx].erase( m_polys[aPolygonIdx].begin() + aContourIdx );
}


int SHAPE_POLY_SET::RemoveNullSegments()
{
    int removed = 0;

    ITERATOR iterator = IterateWithHoles();

    VECTOR2I    contourStart = *iterator;
    VECTOR2I    segmentStart, segmentEnd;

    VERTEX_INDEX indexStart;

    while( iterator )
    {
        // Obtain first point and its index
        segmentStart = *iterator;
        indexStart = iterator.GetIndex();

        // Obtain last point
        if( iterator.IsEndContour() )
        {
            segmentEnd = contourStart;

            // Advance
            iterator++;

            if( iterator )
                contourStart = *iterator;
        }
        else
        {
            // Advance
            iterator++;

            if( iterator )
                segmentEnd = *iterator;
        }

        // Remove segment start if both points are equal
        if( segmentStart == segmentEnd )
        {
            RemoveVertex( indexStart );
            removed++;

            // Advance the iterator one position, as there is one vertex less.
            if( iterator )
                iterator++;
        }
    }

    return removed;
}


void SHAPE_POLY_SET::DeletePolygon( int aIdx )
{
    m_polys.erase( m_polys.begin() + aIdx );
}


void SHAPE_POLY_SET::Append( const SHAPE_POLY_SET& aSet )
{
    m_polys.insert( m_polys.end(), aSet.m_polys.begin(), aSet.m_polys.end() );
}


void SHAPE_POLY_SET::Append( const VECTOR2I& aP, int aOutline, int aHole )
{
    Append( aP.x, aP.y, aOutline, aHole );
}


bool SHAPE_POLY_SET::CollideVertex( const VECTOR2I& aPoint,
        SHAPE_POLY_SET::VERTEX_INDEX& aClosestVertex, int aClearance )
{
    // Shows whether there was a collision
    bool collision = false;

    // Difference vector between each vertex and aPoint.
    VECTOR2D    delta;
    double      distance, clearance;

    // Convert clearance to double for precission when comparing distances
    clearance = aClearance;

    for( ITERATOR iterator = IterateWithHoles(); iterator; iterator++ )
    {
        // Get the difference vector between current vertex and aPoint
        delta = *iterator - aPoint;

        // Compute distance
        distance = delta.EuclideanNorm();

        // Check for collisions
        if( distance <= clearance )
        {
            collision = true;

            // Update aClearance to look for closer vertices
            clearance = distance;

            // Store the indices that identify the vertex
            aClosestVertex = iterator.GetIndex();
        }
    }

    return collision;
}


bool SHAPE_POLY_SET::CollideEdge( const VECTOR2I& aPoint,
        SHAPE_POLY_SET::VERTEX_INDEX& aClosestVertex, int aClearance )
{
    // Shows whether there was a collision
    bool collision = false;

    SEGMENT_ITERATOR iterator;

    for( iterator = IterateSegmentsWithHoles(); iterator; iterator++ )
    {
        SEG currentSegment = *iterator;
        int distance = currentSegment.Distance( aPoint );

        // Check for collisions
        if( distance <= aClearance )
        {
            collision = true;

            // Update aClearance to look for closer edges
            aClearance = distance;

            // Store the indices that identify the vertex
            aClosestVertex = iterator.GetIndex();
        }
    }

    return collision;
}


bool SHAPE_POLY_SET::Contains( const VECTOR2I& aP, int aSubpolyIndex, bool aIgnoreHoles ) const
{
    if( m_polys.size() == 0 ) // empty set?
        return false;

    // If there is a polygon specified, check the condition against that polygon
    if( aSubpolyIndex >= 0 )
        return containsSingle( aP, aSubpolyIndex, aIgnoreHoles );

    // In any other case, check it against all polygons in the set
    for( int polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        if( containsSingle( aP, polygonIdx, aIgnoreHoles ) )
            return true;
    }

    return false;
}


void SHAPE_POLY_SET::RemoveVertex( int aGlobalIndex )
{
    VERTEX_INDEX index;

    // Assure the to be removed vertex exists, abort otherwise
    if( GetRelativeIndices( aGlobalIndex, &index ) )
        RemoveVertex( index );
    else
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );
}


void SHAPE_POLY_SET::RemoveVertex( VERTEX_INDEX aIndex )
{
    m_polys[aIndex.m_polygon][aIndex.m_contour].Remove( aIndex.m_vertex );
}


bool SHAPE_POLY_SET::containsSingle( const VECTOR2I& aP, int aSubpolyIndex, bool aIgnoreHoles ) const
{
    // Check that the point is inside the outline
    if( pointInPolygon( aP, m_polys[aSubpolyIndex][0] ) )
    {
        if( !aIgnoreHoles )
        {
            // Check that the point is not in any of the holes
            for( int holeIdx = 0; holeIdx < HoleCount( aSubpolyIndex ); holeIdx++ )
            {
                const SHAPE_LINE_CHAIN hole = CHole( aSubpolyIndex, holeIdx );

                // If the point is inside a hole (and not on its edge),
                // it is outside of the polygon
                if( pointInPolygon( aP, hole ) && !hole.PointOnEdge( aP ) )
                    return false;
            }
        }

        return true;
    }

    return false;
}


bool SHAPE_POLY_SET::pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const
{
    return aPath.PointInside( aP );
}


void SHAPE_POLY_SET::Move( const VECTOR2I& aVector )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
        {
            path.Move( aVector );
        }
    }
}


void SHAPE_POLY_SET::Rotate( double aAngle, const VECTOR2I& aCenter )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
        {
            path.Rotate( aAngle, aCenter );
        }
    }
}


int SHAPE_POLY_SET::TotalVertices() const
{
    int c = 0;

    for( const POLYGON& poly : m_polys )
    {
        for( const SHAPE_LINE_CHAIN& path : poly )
        {
            c += path.PointCount();
        }
    }

    return c;
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::ChamferPolygon( unsigned int aDistance, int aIndex )
{
    return chamferFilletPolygon( CORNER_MODE::CHAMFERED, aDistance, aIndex );
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::FilletPolygon( unsigned int aRadius,
        int aErrorMax,
        int aIndex )
{
    return chamferFilletPolygon( CORNER_MODE::FILLETED, aRadius, aIndex, aErrorMax );
}


int SHAPE_POLY_SET::DistanceToPolygon( VECTOR2I aPoint, int aPolygonIndex )
{
    // We calculate the min dist between the segment and each outline segment
    // However, if the segment to test is inside the outline, and does not cross
    // any edge, it can be seen outside the polygon.
    // Therefore test if a segment end is inside ( testing only one end is enough )
    if( containsSingle( aPoint, aPolygonIndex ) )
        return 0;

    SEGMENT_ITERATOR iterator = IterateSegmentsWithHoles( aPolygonIndex );

    SEG polygonEdge = *iterator;
    int minDistance = polygonEdge.Distance( aPoint );

    for( iterator++; iterator && minDistance > 0; iterator++ )
    {
        polygonEdge = *iterator;

        int currentDistance = polygonEdge.Distance( aPoint );

        if( currentDistance < minDistance )
            minDistance = currentDistance;
    }

    return minDistance;
}


int SHAPE_POLY_SET::DistanceToPolygon( SEG aSegment, int aPolygonIndex, int aSegmentWidth )
{
    // We calculate the min dist between the segment and each outline segment
    // However, if the segment to test is inside the outline, and does not cross
    // any edge, it can be seen outside the polygon.
    // Therefore test if a segment end is inside ( testing only one end is enough )
    if( containsSingle( aSegment.A, aPolygonIndex ) )
        return 0;

    SEGMENT_ITERATOR iterator = IterateSegmentsWithHoles( aPolygonIndex );

    SEG polygonEdge = *iterator;
    int minDistance = polygonEdge.Distance( aSegment );

    for( iterator++; iterator && minDistance > 0; iterator++ )
    {
        polygonEdge = *iterator;

        int currentDistance = polygonEdge.Distance( aSegment );

        if( currentDistance < minDistance )
            minDistance = currentDistance;
    }

    // Take into account the width of the segment
    if( aSegmentWidth > 0 )
        minDistance -= aSegmentWidth / 2;

    // Return the maximum of minDistance and zero
    return minDistance < 0 ? 0 : minDistance;
}


int SHAPE_POLY_SET::Distance( VECTOR2I aPoint )
{
    int currentDistance;
    int minDistance = DistanceToPolygon( aPoint, 0 );

    // Iterate through all the polygons and get the minimum distance.
    for( unsigned int polygonIdx = 1; polygonIdx < m_polys.size(); polygonIdx++ )
    {
        currentDistance = DistanceToPolygon( aPoint, polygonIdx );

        if( currentDistance < minDistance )
            minDistance = currentDistance;
    }

    return minDistance;
}


int SHAPE_POLY_SET::Distance( const SEG& aSegment, int aSegmentWidth )
{
    int currentDistance;
    int minDistance = DistanceToPolygon( aSegment, 0 );

    // Iterate through all the polygons and get the minimum distance.
    for( unsigned int polygonIdx = 1; polygonIdx < m_polys.size(); polygonIdx++ )
    {
        currentDistance = DistanceToPolygon( aSegment, polygonIdx, aSegmentWidth );

        if( currentDistance < minDistance )
            minDistance = currentDistance;
    }

    return minDistance;
}


bool SHAPE_POLY_SET::IsVertexInHole( int aGlobalIdx )
{
    VERTEX_INDEX index;

    // Get the polygon and contour where the vertex is. If the vertex does not exist, return false
    if( !GetRelativeIndices( aGlobalIdx, &index ) )
        return false;

    // The contour is a hole if its index is greater than zero
    return index.m_contour > 0;
}


SHAPE_POLY_SET SHAPE_POLY_SET::Chamfer( int aDistance )
{
    SHAPE_POLY_SET chamfered;

    for( unsigned int polygonIdx = 0; polygonIdx < m_polys.size(); polygonIdx++ )
        chamfered.m_polys.push_back( ChamferPolygon( aDistance, polygonIdx ) );

    return chamfered;
}


SHAPE_POLY_SET SHAPE_POLY_SET::Fillet( int aRadius, int aErrorMax )
{
    SHAPE_POLY_SET filleted;

    for( size_t polygonIdx = 0; polygonIdx < m_polys.size(); polygonIdx++ )
        filleted.m_polys.push_back( FilletPolygon( aRadius, aErrorMax, polygonIdx ) );

    return filleted;
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::chamferFilletPolygon( CORNER_MODE aMode,
        unsigned int aDistance,
        int aIndex,
        int aErrorMax )
{
    // Null segments create serious issues in calculations. Remove them:
    RemoveNullSegments();

    SHAPE_POLY_SET::POLYGON currentPoly = Polygon( aIndex );
    SHAPE_POLY_SET::POLYGON newPoly;

    // If the chamfering distance is zero, then the polygon remain intact.
    if( aDistance == 0 )
    {
        return currentPoly;
    }

    // Iterate through all the contours (outline and holes) of the polygon.
    for( SHAPE_LINE_CHAIN& currContour : currentPoly )
    {
        // Generate a new contour in the new polygon
        SHAPE_LINE_CHAIN newContour;

        // Iterate through the vertices of the contour
        for( int currVertex = 0; currVertex < currContour.PointCount(); currVertex++ )
        {
            // Current vertex
            int x1  = currContour.Point( currVertex ).x;
            int y1  = currContour.Point( currVertex ).y;

            // Indices for previous and next vertices.
            int prevVertex;
            int nextVertex;

            // Previous and next vertices indices computation. Necessary to manage the edge cases.

            // Previous vertex is the last one if the current vertex is the first one
            prevVertex = currVertex == 0 ? currContour.PointCount() - 1 : currVertex - 1;

            // next vertex is the first one if the current vertex is the last one.
            nextVertex = currVertex == currContour.PointCount() - 1 ? 0 : currVertex + 1;

            // Previous vertex computation
            double  xa  = currContour.Point( prevVertex ).x - x1;
            double  ya  = currContour.Point( prevVertex ).y - y1;

            // Next vertex computation
            double  xb  = currContour.Point( nextVertex ).x - x1;
            double  yb  = currContour.Point( nextVertex ).y - y1;

            // Compute the new distances
            double  lena    = hypot( xa, ya );
            double  lenb    = hypot( xb, yb );

            // Make the final computations depending on the mode selected, chamfered or filleted.
            if( aMode == CORNER_MODE::CHAMFERED )
            {
                double distance = aDistance;

                // Chamfer one half of an edge at most
                if( 0.5 * lena < distance )
                    distance = 0.5 * lena;

                if( 0.5 * lenb < distance )
                    distance = 0.5 * lenb;

                int nx1 = KiROUND( distance * xa / lena );
                int ny1 = KiROUND( distance * ya / lena );

                newContour.Append( x1 + nx1, y1 + ny1 );

                int nx2 = KiROUND( distance * xb / lenb );
                int ny2 = KiROUND( distance * yb / lenb );

                newContour.Append( x1 + nx2, y1 + ny2 );
            }
            else    // CORNER_MODE = FILLETED
            {
                double cosine = ( xa * xb + ya * yb ) / ( lena * lenb );

                double  radius  = aDistance;
                double  denom   = sqrt( 2.0 / ( 1 + cosine ) - 1 );

                // Do nothing in case of parallel edges
                if( std::isinf( denom ) )
                    continue;

                // Limit rounding distance to one half of an edge
                if( 0.5 * lena * denom < radius )
                    radius = 0.5 * lena * denom;

                if( 0.5 * lenb * denom < radius )
                    radius = 0.5 * lenb * denom;

                // Calculate fillet arc absolute center point (xc, yx)
                double  k = radius / sqrt( .5 * ( 1 - cosine ) );
                double  lenab = sqrt( ( xa / lena + xb / lenb ) * ( xa / lena + xb / lenb ) +
                        ( ya / lena + yb / lenb ) * ( ya / lena + yb / lenb ) );
                double  xc  = x1 + k * ( xa / lena + xb / lenb ) / lenab;
                double  yc  = y1 + k * ( ya / lena + yb / lenb ) / lenab;

                // Calculate arc start and end vectors
                k = radius / sqrt( 2 / ( 1 + cosine ) - 1 );
                double  xs  = x1 + k * xa / lena - xc;
                double  ys  = y1 + k * ya / lena - yc;
                double  xe  = x1 + k * xb / lenb - xc;
                double  ye  = y1 + k * yb / lenb - yc;

                // Cosine of arc angle
                double argument = ( xs * xe + ys * ye ) / ( radius * radius );

                // Make sure the argument is in [-1,1], interval in which the acos function is
                // defined
                if( argument < -1 )
                    argument = -1;
                else if( argument > 1 )
                    argument = 1;

                double arcAngle = acos( argument );
                double arcAngleDegrees = arcAngle * 180.0 / M_PI;
                int    segments = GetArcToSegmentCount( radius, aErrorMax, arcAngleDegrees );

                double  deltaAngle  = arcAngle / segments;
                double  startAngle  = atan2( -ys, xs );

                // Flip arc for inner corners
                if( xa * yb - ya * xb <= 0 )
                    deltaAngle *= -1;

                double  nx  = xc + xs;
                double  ny  = yc + ys;

                newContour.Append( KiROUND( nx ), KiROUND( ny ) );

                // Store the previous added corner to make a sanity check
                int prevX   = KiROUND( nx );
                int prevY   = KiROUND( ny );

                for( int j = 0; j < segments; j++ )
                {
                    nx = xc + cos( startAngle + ( j + 1 ) * deltaAngle ) * radius;
                    ny = yc - sin( startAngle + ( j + 1 ) * deltaAngle ) * radius;

                    // Sanity check: the rounding can produce repeated corners; do not add them.
                    if( KiROUND( nx ) != prevX || KiROUND( ny ) != prevY )
                    {
                        newContour.Append( KiROUND( nx ), KiROUND( ny ) );
                        prevX   = KiROUND( nx );
                        prevY   = KiROUND( ny );
                    }
                }
            }
        }

        // Close the current contour and add it the new polygon
        newContour.SetClosed( true );
        newPoly.push_back( newContour );
    }

    return newPoly;
}


SHAPE_POLY_SET &SHAPE_POLY_SET::operator=( const SHAPE_POLY_SET& aOther )
{
    static_cast<SHAPE&>(*this) = aOther;
    m_polys = aOther.m_polys;

    // reset poly cache:
    m_hash = MD5_HASH{};
    m_triangulationValid = false;
    m_triangulatedPolys.clear();
    return *this;
}


class SHAPE_POLY_SET::TRIANGULATION_CONTEXT
{
public:

    TRIANGULATION_CONTEXT( TRIANGULATED_POLYGON* aResultPoly ) :
    m_triPoly( aResultPoly )
    {
    }

    void AddOutline( const SHAPE_LINE_CHAIN& outl, bool aIsHole = false )
    {
        m_points.reserve( outl.PointCount() );
        m_points.clear();

        for( int i = 0; i < outl.PointCount(); i++ )
        {
            m_points.push_back( addPoint( outl.CPoint( i ) ) );
        }

        if ( aIsHole )
            m_cdt->AddHole( m_points );
        else
            m_cdt.reset( new p2t::CDT( m_points ) );
    }

    void Triangulate()
    {
        m_cdt->Triangulate();

        m_triPoly->AllocateTriangles( m_cdt->GetTriangles().size() );

        int i = 0;

        for( auto tri : m_cdt->GetTriangles() )
        {
            TRIANGULATED_POLYGON::TRI t;

            t.a = tri->GetPoint( 0 )->id;
            t.b = tri->GetPoint( 1 )->id;
            t.c = tri->GetPoint( 2 )->id;

            m_triPoly->SetTriangle(i, t);
            i++;
        }

        for( auto p : m_uniquePoints )
            delete p;
    }

private:

    class comparePoints
    {
    public:
        bool operator()( p2t::Point* a, p2t::Point* b ) const
        {
        if (a->x < b->x)
            return true;

        if( a->x == b->x )
            return ( a->y > b->y );

        return false;
        }
    };


    p2t::Point* addPoint( const VECTOR2I& aP )
    {
        p2t::Point check( aP.x, aP.y );
        auto it = m_uniquePoints.find( &check );

        if( it != m_uniquePoints.end() )
        {
            return *it;
        }
        else
        {
            auto lastId = m_triPoly->GetVertexCount();
            auto p = new p2t::Point( aP.x, aP.y, lastId );
            m_triPoly->AddVertex( aP );
            m_uniquePoints.insert ( p );
            return p;
        }
    }

    typedef std::set<p2t::Point*, comparePoints>  P2T_SET;
    typedef std::vector<p2t::Point*>    P2T_VEC;

    P2T_VEC m_points;
    P2T_SET m_uniquePoints;
    TRIANGULATED_POLYGON *m_triPoly;
    std::unique_ptr<p2t::CDT> m_cdt;
};

SHAPE_POLY_SET::TRIANGULATED_POLYGON::~TRIANGULATED_POLYGON()
{
    Clear();
}


void SHAPE_POLY_SET::TRIANGULATED_POLYGON::Clear()
{
    if( m_vertices )
        delete[] m_vertices;

    if( m_triangles )
        delete[] m_triangles;
}


void SHAPE_POLY_SET::TRIANGULATED_POLYGON::AllocateVertices( int aSize )
{
    m_vertices = new VECTOR2I[aSize];
}


void SHAPE_POLY_SET::TRIANGULATED_POLYGON::AllocateTriangles( int aSize )
{
    m_triangles = new TRI[aSize];
    m_triangleCount = aSize;
}


static int totalVertexCount( const SHAPE_POLY_SET::POLYGON& aPoly )
{
    int cnt = 0;

    for( const auto& outl : aPoly )
    {
        cnt += outl.PointCount();
    }

    return cnt;
}


void SHAPE_POLY_SET::triangulateSingle( const POLYGON& aPoly,
        SHAPE_POLY_SET::TRIANGULATED_POLYGON& aResult )
{
    if( aPoly.size() == 0 )
        return;

    TRIANGULATION_CONTEXT ctx ( &aResult );

    aResult.AllocateVertices( totalVertexCount( aPoly ) );
    ctx.AddOutline( aPoly[0], false );

    for( unsigned i = 1; i < aPoly.size(); i++ )
    {
        ctx.AddOutline( aPoly[i], true ); // add holes
    }

    ctx.Triangulate();
}


MD5_HASH SHAPE_POLY_SET::GetHash() const
{
    if( !m_hash.IsValid() )
        return checksum();

    return m_hash;
}


bool SHAPE_POLY_SET::IsTriangulationUpToDate() const
{
    if( !m_triangulationValid )
        return false;

    if( !m_hash.IsValid() )
        return false;

    auto hash = checksum();

    return hash == m_hash;
}


void SHAPE_POLY_SET::CacheTriangulation()
{
    bool recalculate = !m_hash.IsValid();
    MD5_HASH hash;

    if( !m_triangulationValid )
        recalculate = true;

    if( !recalculate )
    {
        hash = checksum();

        if( m_hash != hash )
        {
            m_hash = hash;
            recalculate = true;
        }
    }

    if( !recalculate )
        return;

    SHAPE_POLY_SET tmpSet = *this;

    if( !tmpSet.HasHoles() )
        tmpSet.Unfracture( PM_FAST );

    m_triangulatedPolys.clear();

    if( tmpSet.HasTouchingHoles() )
    {
        // temporary workaround for overlapping hole vertices that poly2tri doesn't handle
        m_triangulationValid = false;
        return;
    }

    for( int i = 0; i < tmpSet.OutlineCount(); i++ )
    {
        m_triangulatedPolys.push_back( std::make_unique<TRIANGULATED_POLYGON>() );
        triangulateSingle( tmpSet.Polygon( i ), *m_triangulatedPolys.back() );
    }

    m_triangulationValid = true;
    m_hash = checksum();
}


MD5_HASH SHAPE_POLY_SET::checksum() const
{
    MD5_HASH hash;

    hash.Hash( m_polys.size() );

    for( const auto& outline : m_polys )
    {
        hash.Hash( outline.size() );

        for( const auto& lc : outline )
        {
            hash.Hash( lc.PointCount() );

            for( int i = 0; i < lc.PointCount(); i++ )
            {
                hash.Hash( lc.CPoint( i ).x );
                hash.Hash( lc.CPoint( i ).y );
            }
        }
    }

    hash.Finalize();

    return hash;
}


bool SHAPE_POLY_SET::HasTouchingHoles() const
{
    for( int i = 0; i < OutlineCount(); i++ )
    {
        if( hasTouchingHoles( CPolygon( i ) ) )
        {
            return true;
        }
    }

    return false;
}


bool SHAPE_POLY_SET::hasTouchingHoles( const POLYGON& aPoly ) const
{
    std::vector< VECTOR2I > pts;

    for( const auto& lc : aPoly )
    {
        for( int i = 0; i < lc.PointCount(); i++ )
        {
            const auto p = lc.CPoint( i );

            if( std::find( pts.begin(), pts.end(), p ) != pts.end() )
            {
                return true;
            }

            pts.push_back( p );
        }
    }

    return false;
}
