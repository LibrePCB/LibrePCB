/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __POLY_GRID_PARTITION_H
#define __POLY_GRID_PARTITION_H

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>

/**
 * Class POLY_GRID_PARTITION
 *
 * Provides a fast test for point inside polygon by splitting the edges
 * of the polygon into a rectangular grid.
 */
class POLY_GRID_PARTITION
{
private:
    enum HASH_FLAG
    {
        LEAD_H  = 1,
        LEAD_V  = 2,
        TRAIL_H = 4,
        TRAIL_V = 8
    };

    using EDGE_LIST = std::vector<int>;

    template <class T>
    inline void hash_combine( std::size_t& seed, const T& v )
    {
        std::hash<T> hasher;
        seed ^= hasher( v ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    struct segsEqual
    {
        bool operator()( const SEG& a, const SEG& b ) const
        {
            return (a.A == b.A && a.B == b.B) || (a.A == b.B && a.B == b.A);
        }
    };

    struct segHash
    {
        std::size_t operator()(  const SEG& a ) const
        {
            return a.A.x + a.B.x + a.A.y + a.B.y;
        }
    };

    const VECTOR2I grid2poly( const VECTOR2I& p ) const
    {
        int px  = rescale( p.x, m_bbox.GetWidth(), m_gridSize ) + m_bbox.GetPosition().x;
        int py  = rescale( p.y, m_bbox.GetHeight(), m_gridSize ) + m_bbox.GetPosition().y;     // (int) floor( (double) p.y / m_gridSize * (double) m_bbox.GetHeight() + m_bbox.GetPosition().y );

        return VECTOR2I( px, py );
    }

    void stupid_test() const
    {
        for(int i = 0; i < 16;i++)
        assert( poly2gridX(grid2polyX(i)) == i);
    }

    int grid2polyX( int x ) const
    {
        return rescale( x, m_bbox.GetWidth(), m_gridSize ) + m_bbox.GetPosition().x;
    }

    int grid2polyY( int y ) const
    {
        return rescale( y, m_bbox.GetHeight(), m_gridSize ) + m_bbox.GetPosition().y;
    }

    const VECTOR2I poly2grid( const VECTOR2I& p ) const
    {
        int px  = rescale( p.x - m_bbox.GetPosition().x, m_gridSize, m_bbox.GetWidth() );
        int py  = rescale( p.y - m_bbox.GetPosition().y, m_gridSize, m_bbox.GetHeight() );

        if( px < 0 )
            px = 0;

        if( px >= m_gridSize )
            px = m_gridSize - 1;

        if( py < 0 )
            py = 0;

        if( py >= m_gridSize )
            py = m_gridSize - 1;

        return VECTOR2I( px, py );
    }

    int poly2gridX( int x ) const
    {
        int px = rescale( x - m_bbox.GetPosition().x, m_gridSize, m_bbox.GetWidth() );

        if( px < 0 )
            px = 0;

        if( px >= m_gridSize )
            px = m_gridSize - 1;

        return px;
    }

    int poly2gridY( int y ) const
    {
        int py = rescale( y - m_bbox.GetPosition().y, m_gridSize, m_bbox.GetHeight() );

        if( py < 0 )
            py = 0;

        if( py >= m_gridSize )
            py = m_gridSize - 1;

        return py;
    }

    void build( const SHAPE_LINE_CHAIN& aPolyOutline, int gridSize )
    {
        m_outline = aPolyOutline;

        //if (orientation(m_outline) < 0)
        //    m_outline = m_outline.Reverse();

        m_bbox = m_outline.BBox();
        m_gridSize = gridSize;

        m_outline.SetClosed( true );

        m_grid.reserve( gridSize * gridSize );

        for( int y = 0; y < gridSize; y++ )
        {
            for( int x = 0; x < gridSize; x++ )
            {
                m_grid.push_back( EDGE_LIST() );
            }
        }

        VECTOR2I    ref_v( 0, 1 );
        VECTOR2I    ref_h( 0, 1 );

        m_flags.reserve( m_outline.SegmentCount() );

        std::unordered_map<SEG, int, segHash, segsEqual> edgeSet;

        for( int i = 0; i<m_outline.SegmentCount(); i++ )
        {
            SEG edge = m_outline.CSegment( i );

            if( edgeSet.find( edge ) == edgeSet.end() )
            {
                edgeSet[edge] = 1;
            }
            else
            {
                edgeSet[edge]++;
            }
        }

        for( int i = 0; i<m_outline.SegmentCount(); i++ )
        {
            auto    edge    = m_outline.CSegment( i );
            auto    dir     = edge.B - edge.A;
            int     flags   = 0;

            if( edgeSet[edge] == 1 )
            {
                if( dir.Dot( ref_h ) < 0 )
                {
                    flags |= LEAD_H;
                }
                else if( dir.Dot( ref_h ) > 0 )
                {
                    flags |= TRAIL_H;
                }

            }

            m_flags.push_back( flags );

            std::set<int> indices;

            indices.insert( m_gridSize * poly2gridY( edge.A.y ) + poly2gridX( edge.A.x ) );
            indices.insert( m_gridSize * poly2gridY( edge.B.y ) + poly2gridX( edge.B.x ) );

            if( edge.A.x > edge.B.x )
                std::swap( edge.A, edge.B );

            dir = edge.B - edge.A;

            if( dir.x != 0 )
            {
                int gx0 = poly2gridX( edge.A.x );
                int gx1 = poly2gridX( edge.B.x );

                for( int x = gx0; x <= gx1; x++ )
                {
                    int px  = grid2polyX( x );
                    int py  = ( edge.A.y + rescale( dir.y, px - edge.A.x, dir.x ) );
                    int yy  = poly2gridY( py );

                    indices.insert( m_gridSize * yy + x );
 					if( x > 0 )
                        indices.insert( m_gridSize * yy + x - 1 );

                }
            }

            if( edge.A.y > edge.B.y )
                std::swap( edge.A, edge.B );

            dir = edge.B - edge.A;

            if( dir.y != 0 )
            {
                int gy0 = poly2gridY( edge.A.y );
                int gy1 = poly2gridY( edge.B.y );

                for( int y = gy0; y <= gy1; y++ )
                {
                    int py  = grid2polyY( y );
                    int px  = ( edge.A.x + rescale( dir.x, py - edge.A.y, dir.y ) );
                    int xx  = poly2gridX( px );

                    indices.insert( m_gridSize * y + xx );
       				if( y > 0 )
                        indices.insert( m_gridSize * (y - 1) + xx );
                }
            }

            for( auto idx : indices )
                m_grid[idx].push_back( i );
        }

    }


    bool inRange( int v1, int v2, int x ) const
    {
        if( v1 < v2 )
        {
            return x >= v1 && x <= v2;
        }

        return x >= v2 && x <= v1;
    }

    struct SCAN_STATE
    {
        SCAN_STATE()
        {
            dist_prev   = INT_MAX;
            dist_max    = INT_MAX;
            nearest     = -1;
            nearest_prev = -1;
        };

        int dist_prev;
        int dist_max;
        int nearest_prev;
        int nearest;
    };

    void scanCell( SCAN_STATE& state, const EDGE_LIST& cell, const VECTOR2I& aP, int cx, int cy  ) const
    {
        int cx0 = grid2polyX(cx);
        int cx1 = grid2polyX(cx + 1);

        for( auto index : cell )
        {
            const SEG& edge = m_outline.CSegment( index );


            if( m_flags[index] == 0 )
            {
                if ( aP.y == edge.A.y && inRange( edge.A.x, edge.B.x, aP.x ) ) // we belong to the outline
                {
                    state.nearest = index;
                    state.dist_max = 0;
                    return;
                } else {
                    continue;
                }
            }

            if( inRange( edge.A.y, edge.B.y, aP.y ) )
            {
                int dist = 0;
                int x0;
                if( edge.A.y == aP.y )
                {
                    x0 = edge.A.x;
                }
                else if( edge.B.y == aP.y )
                {
                    x0 = edge.B.x;
                }
                else
                {
                    x0 = edge.A.x + rescale( ( edge.B.x - edge.A.x ), (aP.y - edge.A.y), (edge.B.y - edge.A.y ) );
                }

                if( x0 < cx0 || x0 > cx1 )
                {
                    continue;
                }

                dist = aP.x - x0;

                if( dist == 0 )
                {
                    if( state.nearest_prev < 0 || state.nearest != index )
                    {
                        state.dist_prev = state.dist_max;
                        state.nearest_prev = state.nearest;
                    }

                    state.nearest   = index;
                    state.dist_max  = 0;
                    return;
                }

                if( dist != 0 && std::abs( dist ) <= std::abs( state.dist_max ) )
                {
                    if( state.nearest_prev < 0 || state.nearest != index )
                    {
                        state.dist_prev = state.dist_max;
                        state.nearest_prev = state.nearest;
                    }

                    state.dist_max  = dist;
                    state.nearest   = index;
                }
            }
        }
    }

public:

    POLY_GRID_PARTITION( const SHAPE_LINE_CHAIN& aPolyOutline, int gridSize )
    {
        build( aPolyOutline, gridSize );
    }

    int containsPoint( const VECTOR2I& aP )    // const
    {
        const auto gridPoint = poly2grid( aP );

        if( !m_bbox.Contains( aP ) )
            return false;

        SCAN_STATE state;
        const EDGE_LIST& cell = m_grid[ m_gridSize * gridPoint.y + gridPoint.x ];

        scanCell( state, cell, aP, gridPoint.x, gridPoint.y );

        if( state.nearest < 0 )
        {
            state = SCAN_STATE();

            for( int d = 1; d <= m_gridSize; d++ )
            {
                int xl  = gridPoint.x - d;
                int xh  = gridPoint.x + d;

                if( xl >= 0 )
                {
                    const EDGE_LIST& cell2 = m_grid[ m_gridSize * gridPoint.y + xl ];
                    scanCell( state, cell2, aP, xl, gridPoint.y );

                    if( state.nearest >= 0 )
                        break;
                }

                if( xh < m_gridSize )
                {
                    const EDGE_LIST& cell2 = m_grid[ m_gridSize * gridPoint.y + xh ];
                    scanCell( state, cell2, aP, xh, gridPoint.y );

                    if( state.nearest >= 0 )
                        break;
                }
            }
        }

        if( state.nearest < 0 )
            return 0;

        if( state.dist_max == 0 )
            return 1;

        if( state.nearest_prev >= 0 && state.dist_max == state.dist_prev )
        {
            int d = std::abs( state.nearest_prev - state.nearest );

            if( (d == 1) && ( (m_flags[state.nearest_prev] & m_flags[state.nearest]) == 0 ) )
            {
                return 0;
            }
            else if( d > 1 )
            {
                return 1;
            }
        }

        if( state.dist_max > 0 )
        {
            return m_flags[state.nearest] & LEAD_H ? 1 : 0;
        }
        else
        {
            return m_flags[state.nearest] & TRAIL_H ? 1 : 0;
        }
    }

    bool checkClearance( const VECTOR2I& aP, int aClearance )
    {
        int gx0 = poly2gridX( aP.x - aClearance - 1);
        int gx1 = poly2gridX( aP.x + aClearance + 1);
        int gy0 = poly2gridY( aP.y - aClearance - 1);
        int gy1 = poly2gridY( aP.y + aClearance + 1);

        using ecoord = VECTOR2I::extended_type;

        ecoord dist = (ecoord) aClearance * aClearance;

        for ( int gx = gx0; gx <= gx1; gx++ )
        {
            for ( int gy = gy0; gy <= gy1; gy++ )
            {
                const auto& cell = m_grid [ m_gridSize * gy + gx];
                for ( auto index : cell )
                {
                    const auto& seg = m_outline.CSegment(index);

                    if ( seg.SquaredDistance(aP) <= dist )
                        return true;

                }
            }

        }
        return false;
    }



    int ContainsPoint( const VECTOR2I& aP, int aClearance = 0 )    // const
    {
        if( containsPoint(aP) )
            return 1;

        if( aClearance > 0 )
            return checkClearance ( aP, aClearance );

        return 0;
    }

    const BOX2I& BBox() const
    {
        return m_bbox;
    }

private:
    int m_gridSize;
    SHAPE_LINE_CHAIN m_outline;
    BOX2I m_bbox;
    std::vector<int> m_flags;
    std::vector<EDGE_LIST> m_grid;
};

#endif
