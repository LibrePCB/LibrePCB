/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#ifndef __SHAPE_POLY_SET_H
#define __SHAPE_POLY_SET_H

#include <vector>
#include <cstdio>
#include <memory>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "clipper.hpp"

#include <md5_hash.h>


/**
 * Class SHAPE_POLY_SET
 *
 * Represents a set of closed polygons. Polygons may be nonconvex, self-intersecting
 * and have holes. Provides boolean operations (using Clipper library as the backend).
 *
 * Let us define the terms used on this class to clarify methods names and comments:
 *      - Polygon: each polygon in the set.
 *      - Outline: first polyline in each polygon; represents its outer contour.
 *      - Hole: second and following polylines in the polygon.
 *      - Contour: each polyline of each polygon in the set, whether or not it is an
 *      outline or a hole.
 *      - Vertex (or corner): each one of the points that define a contour.
 *
 * TODO: add convex partitioning & spatial index
 */
class SHAPE_POLY_SET : public SHAPE
{
    public:
        ///> represents a single polygon outline with holes. The first entry is the outline,
        ///> the remaining (if any), are the holes
        typedef std::vector<SHAPE_LINE_CHAIN> POLYGON;

        class TRIANGULATION_CONTEXT;

        class TRIANGULATED_POLYGON
        {
        public:
            struct TRI
            {
                TRI() : a(0), b(0), c(0)
                {
                }

                int a, b, c;
            };

            ~TRIANGULATED_POLYGON();

            void Clear();

            void AllocateVertices( int aSize );
            void AllocateTriangles ( int aSize );

            void GetTriangle( int index, VECTOR2I& a, VECTOR2I& b, VECTOR2I& c ) const
            {
                auto tri = &m_triangles[ index ];
                a = m_vertices[ tri->a ];
                b = m_vertices[ tri->b ];
                c = m_vertices[ tri->c ];
            }

            void SetTriangle( int aIndex, const TRI& aTri )
            {
                m_triangles[aIndex] = aTri;
            }

            int AddVertex( const VECTOR2I& aP )
            {
                m_vertices[ m_vertexCount ] = aP;
                return (m_vertexCount++);
            }

            int GetTriangleCount() const
            {
                return m_triangleCount;
            }

            int GetVertexCount() const
            {
                return m_vertexCount;
            }

        private:

            TRI* m_triangles = nullptr;
            VECTOR2I* m_vertices = nullptr;
            int m_vertexCount = 0;
            int m_triangleCount = 0;
        };

        /**
         * Struct VERTEX_INDEX
         *
         * Structure to hold the necessary information in order to index a vertex on a
         * SHAPE_POLY_SET object: the polygon index, the contour index relative to the polygon and
         * the vertex index relative the contour.
         */
        typedef struct VERTEX_INDEX
        {
            int m_polygon;   /*!< m_polygon is the index of the polygon. */
            int m_contour;   /*!< m_contour is the index of the contour relative to the polygon. */
            int m_vertex;    /*!< m_vertex is the index of the vertex relative to the contour. */

            VERTEX_INDEX() : m_polygon(-1), m_contour(-1), m_vertex(-1)
            {
            }
        } VERTEX_INDEX;

        /**
         * Class ITERATOR_TEMPLATE
         *
         * Base class for iterating over all vertices in a given SHAPE_POLY_SET.
         */
        template <class T>
        class ITERATOR_TEMPLATE
        {
        public:

            /**
             * Function IsEndContour.
             * @return bool - true if the current vertex is the last one of the current contour
             *              (outline or hole); false otherwise.
             */
            bool IsEndContour() const
            {
                return m_currentVertex + 1 == m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount();
            }

            /**
             * Function IsLastOutline.
             * @return bool - true if the current outline is the last one; false otherwise.
             */
            bool IsLastPolygon() const
            {
                return m_currentPolygon == m_lastPolygon;
            }

            operator bool() const
            {
                return m_currentPolygon <= m_lastPolygon;
            }

            /**
             * Function Advance
             * advances the indices of the current vertex/outline/contour, checking whether the
             * vertices in the holes have to be iterated through
             */
            void Advance()
            {
                // Advance vertex index
                m_currentVertex ++;

                // Check whether the user wants to iterate through the vertices of the holes
                // and behave accordingly
                if( m_iterateHoles )
                {
                    // If the last vertex of the contour was reached, advance the contour index
                    if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount() )
                    {
                        m_currentVertex = 0;
                        m_currentContour++;

                        // If the last contour of the current polygon was reached, advance the
                        // outline index
                        int totalContours = m_poly->CPolygon( m_currentPolygon ).size();

                        if( m_currentContour >= totalContours )
                        {
                            m_currentContour = 0;
                            m_currentPolygon++;
                        }
                    }
                }
                else
                {
                    // If the last vertex of the outline was reached, advance to the following polygon
                    if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[0].PointCount() )
                    {
                        m_currentVertex = 0;
                        m_currentPolygon++;
                    }
                }
            }

            void operator++( int dummy )
            {
                Advance();
            }

            void operator++()
            {
                Advance();
            }

            T& Get()
            {
                return m_poly->Polygon( m_currentPolygon )[m_currentContour].Point( m_currentVertex );
            }

            T& operator*()
            {
                return Get();
            }

            T* operator->()
            {
                return &Get();
            }

            /**
             * Function GetIndex
             * @return VERTEX_INDEX - indices of the current polygon, contour and vertex.
             */
            VERTEX_INDEX GetIndex()
            {
                VERTEX_INDEX index;

                index.m_polygon = m_currentPolygon;
                index.m_contour = m_currentContour;
                index.m_vertex = m_currentVertex;

                return index;
            }


        private:
            friend class SHAPE_POLY_SET;

            SHAPE_POLY_SET* m_poly;
            int m_currentPolygon;
            int m_currentContour;
            int m_currentVertex;
            int m_lastPolygon;
            bool m_iterateHoles;
        };

        /**
         * Class SEGMENT_ITERATOR_TEMPLATE
         *
         * Base class for iterating over all segments in a given SHAPE_POLY_SET.
         */
        template <class T>
        class SEGMENT_ITERATOR_TEMPLATE
        {
        public:
            /**
             * Function IsLastOutline.
             * @return bool - true if the current outline is the last one.
             */
            bool IsLastPolygon() const
            {
                return m_currentPolygon == m_lastPolygon;
            }

            operator bool() const
            {
                return m_currentPolygon <= m_lastPolygon;
            }

            /**
             * Function Advance
             * advances the indices of the current vertex/outline/contour, checking whether the
             * vertices in the holes have to be iterated through
             */
            void Advance()
            {
                // Advance vertex index
                m_currentSegment++;
                int last;

                // Check whether the user wants to iterate through the vertices of the holes
                // and behave accordingly
                if( m_iterateHoles )
                {
                    last = m_poly->CPolygon( m_currentPolygon )[m_currentContour].SegmentCount();

                    // If the last vertex of the contour was reached, advance the contour index
                    if( m_currentSegment >= last )
                    {
                        m_currentSegment = 0;
                        m_currentContour++;

                        // If the last contour of the current polygon was reached, advance the
                        // outline index
                        int totalContours = m_poly->CPolygon( m_currentPolygon ).size();

                        if( m_currentContour >= totalContours )
                        {
                            m_currentContour = 0;
                            m_currentPolygon++;
                        }
                    }
                }
                else
                {
                    last = m_poly->CPolygon( m_currentPolygon )[0].SegmentCount();
                    // If the last vertex of the outline was reached, advance to the following
                    // polygon
                    if( m_currentSegment >= last )
                    {
                        m_currentSegment = 0;
                        m_currentPolygon++;
                    }
                }
            }

            void operator++( int dummy )
            {
                Advance();
            }

            void operator++()
            {
                Advance();
            }

            T Get()
            {
                return m_poly->Polygon( m_currentPolygon )[m_currentContour].Segment( m_currentSegment );
            }

            T operator*()
            {
                return Get();
            }

            /**
             * Function GetIndex
             * @return VERTEX_INDEX - indices of the current polygon, contour and vertex.
             */
            VERTEX_INDEX GetIndex()
            {
                VERTEX_INDEX index;

                index.m_polygon = m_currentPolygon;
                index.m_contour = m_currentContour;
                index.m_vertex = m_currentSegment;

                return index;
            }

            /**
             * Function IsAdjacent
             * @param  aOther is an iterator pointing to another segment.
             * @return        bool - true if both iterators point to the same segment of the same
             *                     contour of the same polygon of the same polygon set; false
             *                     otherwise.
             */
            bool IsAdjacent( SEGMENT_ITERATOR_TEMPLATE<T> aOther )
            {
                // Check that both iterators point to the same contour of the same polygon of the
                // same polygon set
                if( m_poly == aOther.m_poly && m_currentPolygon == aOther.m_currentPolygon &&
                    m_currentContour == aOther.m_currentContour )
                {
                    // Compute the total number of segments
                    int numSeg;
                    numSeg = m_poly->CPolygon( m_currentPolygon )[m_currentContour].SegmentCount();

                    // Compute the difference of the segment indices. If it is exactly one, they
                    // are adjacent. The only missing case where they also are adjacent is when
                    // the segments are the first and last one, in which case the difference
                    // always equals the total number of segments minus one.
                    int indexDiff = abs( m_currentSegment - aOther.m_currentSegment );

                    return ( indexDiff == 1 ) || ( indexDiff == (numSeg - 1) );
                }

                return false;
            }

        private:
            friend class SHAPE_POLY_SET;

            SHAPE_POLY_SET* m_poly;
            int m_currentPolygon;
            int m_currentContour;
            int m_currentSegment;
            int m_lastPolygon;
            bool m_iterateHoles;
        };

        // Iterator and const iterator types to visit polygon's points.
        typedef ITERATOR_TEMPLATE<VECTOR2I> ITERATOR;
        typedef ITERATOR_TEMPLATE<const VECTOR2I> CONST_ITERATOR;

        // Iterator and const iterator types to visit polygon's edges.
        typedef SEGMENT_ITERATOR_TEMPLATE<SEG> SEGMENT_ITERATOR;
        typedef SEGMENT_ITERATOR_TEMPLATE<const SEG> CONST_SEGMENT_ITERATOR;

        SHAPE_POLY_SET();

        /**
         * Copy constructor SHAPE_POLY_SET
         * Performs a deep copy of \p aOther into \p this.
         * @param aOther is the SHAPE_POLY_SET object that will be copied.
         */
        SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther );

        ~SHAPE_POLY_SET();

        /**
         * Function GetRelativeIndices
         *
         * Converts a global vertex index ---i.e., a number that globally identifies a vertex in a
         * concatenated list of all vertices in all contours--- and get the index of the vertex
         * relative to the contour relative to the polygon in which it is.
         * @param  aGlobalIdx  is the global index of the corner whose structured index wants to
         *                     be found
         * @param  aRelativeIndices is a pointer to the set of relative indices to store.
         * @return bool - true if the global index is correct and the information in
         *              aRelativeIndices is valid; false otherwise.
         */
        bool GetRelativeIndices( int aGlobalIdx, VERTEX_INDEX* aRelativeIndices) const;

        /**
         * Function GetGlobalIndex
         * computes the global index of a vertex from the relative indices of polygon, contour and
         * vertex.
         * @param  aRelativeIndices is the set of relative indices.
         * @param  aGlobalIdx       [out] is the computed global index.
         * @return bool - true if the relative indices are correct; false otherwise. The computed
         *              global index is returned in the \p aGlobalIdx reference.
         */
        bool GetGlobalIndex( VERTEX_INDEX aRelativeIndices, int& aGlobalIdx );

        /// @copydoc SHAPE::Clone()
        SHAPE* Clone() const override;

        ///> Creates a new empty polygon in the set and returns its index
        int NewOutline();

        ///> Creates a new hole in a given outline
        int NewHole( int aOutline = -1 );

        ///> Adds a new outline to the set and returns its index
        int AddOutline( const SHAPE_LINE_CHAIN& aOutline );

        ///> Adds a new hole to the given outline (default: last) and returns its index
        int AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline = -1 );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        /**
         * Function Append
         * adds a new vertex to the contour indexed by \p aOutline and \p aHole (defaults to the
         * outline of the last polygon).
         * @param  x                 is the x coordinate of the new vertex.
         * @param  y                 is the y coordinate of the new vertex.
         * @param  aOutline          is the index of the polygon.
         * @param  aHole             is the index of the hole (-1 for the main outline),
         * @param  aAllowDuplication is a flag to indicate whether it is allowed to add this
         *                           corner even if it is duplicated.
         * @return int - the number of corners of the selected contour after the addition.
         */
        int Append( int x, int y, int aOutline = -1, int aHole = -1,
                    bool aAllowDuplication = false );

        ///> Merges polygons from two sets.
        void Append( const SHAPE_POLY_SET& aSet );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        void Append( const VECTOR2I& aP, int aOutline = -1, int aHole = -1 );

        /**
         * Function InsertVertex
         * Adds a vertex in the globally indexed position aGlobalIndex.
         * @param aGlobalIndex is the global index of the position in which teh new vertex will be
         *                     inserted.
         * @param aNewVertex   is the new inserted vertex.
         */
        void InsertVertex( int aGlobalIndex, VECTOR2I aNewVertex );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        VECTOR2I& Vertex( int aIndex, int aOutline, int aHole );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I& CVertex( int aIndex, int aOutline, int aHole ) const;

        ///> Returns the aGlobalIndex-th vertex in the poly set
        VECTOR2I& Vertex( int aGlobalIndex );

        ///> Returns the aGlobalIndex-th vertex in the poly set
        const VECTOR2I& CVertex( int aGlobalIndex ) const;

        ///> Returns the index-th vertex in a given hole outline within a given outline
        VECTOR2I& Vertex( VERTEX_INDEX aIndex );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I& CVertex( VERTEX_INDEX aIndex ) const;

        /**
         * Returns the global indexes of the previous and the next corner
         * of the aGlobalIndex-th corner of a contour in the polygon set.
         * they are often aGlobalIndex-1 and aGlobalIndex+1, but not for the first and last
         * corner of the contour.
         * @param  aGlobalIndex is index of the corner, globally indexed between all edges in all
         *                      contours
         * @param aPrevious - the globalIndex of the previous corner of the same contour.
         * @param aNext - the globalIndex of the next corner of the same contour.
         * @return true if OK, false if aGlobalIndex is out of range
         */
        bool GetNeighbourIndexes( int aGlobalIndex, int* aPrevious, int* aNext );


        /**
         * Function IsPolygonSelfIntersecting.
         * Checks whether the aPolygonIndex-th polygon in the set is self intersecting.
         * @param  aPolygonIndex index of the polygon that wants to be checked.
         * @return bool - true if the aPolygonIndex-th polygon is self intersecting, false
         *              otherwise.
         */
        bool IsPolygonSelfIntersecting( int aPolygonIndex );

        /**
         * Function IsSelfIntersecting
         * Checks whether any of the polygons in the set is self intersecting.
         * @return bool - true if any of the polygons is self intersecting, false otherwise.
         */
        bool IsSelfIntersecting();

        ///> Returns the number of triangulated polygons
        unsigned int TriangulatedPolyCount() const { return m_triangulatedPolys.size(); }

        ///> Returns the number of outlines in the set
        int OutlineCount() const { return m_polys.size(); }

        ///> Returns the number of vertices in a given outline/hole
        int VertexCount( int aOutline = -1, int aHole = -1 ) const;

        ///> Returns the number of holes in a given outline
        int HoleCount( int aOutline ) const
        {
            if( ( aOutline < 0 ) || (aOutline >= (int)m_polys.size()) || (m_polys[aOutline].size() < 2) )
                return 0;

            // the first polygon in m_polys[aOutline] is the main contour,
            // only others are holes:
            return m_polys[aOutline].size() - 1;
        }

        ///> Returns the reference to aIndex-th outline in the set
        SHAPE_LINE_CHAIN& Outline( int aIndex )
        {
            return m_polys[aIndex][0];
        }

        /**
         * Function Subset
         * returns a subset of the polygons in this set, the ones between aFirstPolygon and
         * aLastPolygon.
         * @param  aFirstPolygon is the first polygon to be included in the returned set.
         * @param  aLastPolygon  is the first polygon to be excluded of the returned set.
         * @return SHAPE_POLY_SET - a set containing the polygons between aFirstPolygon (included)
         *                        and aLastPolygon (excluded).
         */
        SHAPE_POLY_SET Subset( int aFirstPolygon, int aLastPolygon );

        SHAPE_POLY_SET UnitSet( int aPolygonIndex )
        {
            return Subset( aPolygonIndex, aPolygonIndex + 1 );
        }

        ///> Returns the reference to aHole-th hole in the aIndex-th outline
        SHAPE_LINE_CHAIN& Hole( int aOutline, int aHole )
        {
            return m_polys[aOutline][aHole + 1];
        }

        ///> Returns the aIndex-th subpolygon in the set
        POLYGON& Polygon( int aIndex )
        {
            return m_polys[aIndex];
        }

        const POLYGON& Polygon( int aIndex ) const
        {
            return m_polys[aIndex];
        }

        const TRIANGULATED_POLYGON* TriangulatedPolygon( int aIndex ) const
        {
            return m_triangulatedPolys[aIndex].get();
        }

        const SHAPE_LINE_CHAIN& COutline( int aIndex ) const
        {
            return m_polys[aIndex][0];
        }

        const SHAPE_LINE_CHAIN& CHole( int aOutline, int aHole ) const
        {
            return m_polys[aOutline][aHole + 1];
        }

        const POLYGON& CPolygon( int aIndex ) const
        {
            return m_polys[aIndex];
        }

        /**
         * Function Iterate
         * returns an object to iterate through the points of the polygons between \p aFirst and
         * \p aLast.
         * @param  aFirst        is the first polygon whose points will be iterated.
         * @param  aLast         is the last polygon whose points will be iterated.
         * @param  aIterateHoles is a flag to indicate whether the points of the holes should be
         *                       iterated.
         * @return ITERATOR - the iterator object.
         */
        ITERATOR Iterate( int aFirst, int aLast, bool aIterateHoles = false )
        {
            ITERATOR iter;

            iter.m_poly = this;
            iter.m_currentPolygon = aFirst;
            iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentContour = 0;
            iter.m_currentVertex = 0;
            iter.m_iterateHoles = aIterateHoles;

            return iter;
        }

        /**
         * Function Iterate
         * @param  aOutline the index of the polygon to be iterated.
         * @return ITERATOR - an iterator object to visit all points in the main outline of the
         *                  aOutline-th polygon, without visiting the points in the holes.
         */
        ITERATOR Iterate( int aOutline )
        {
            return Iterate( aOutline, aOutline );
        }

        /**
         * Function IterateWithHoles
         * @param  aOutline the index of the polygon to be iterated.
         * @return ITERATOR - an iterator object to visit all points in the main outline of the
         *                  aOutline-th polygon, visiting also the points in the holes.
         */
        ITERATOR IterateWithHoles( int aOutline )
        {
            return Iterate( aOutline, aOutline, true );
        }

        /**
         * Function Iterate
         * @return ITERATOR - an iterator object to visit all points in all outlines of the set,
         *                  without visiting the points in the holes.
         */
        ITERATOR Iterate()
        {
            return Iterate( 0, OutlineCount() - 1 );
        }

        /**
         * Function IterateWithHoles
         * @return ITERATOR - an iterator object to visit all points in all outlines of the set,
         *                  visiting also the points in the holes.
         */
        ITERATOR IterateWithHoles()
        {
            return Iterate( 0, OutlineCount() - 1, true );
        }


        CONST_ITERATOR CIterate( int aFirst, int aLast, bool aIterateHoles = false ) const
        {
            CONST_ITERATOR iter;

            iter.m_poly = const_cast<SHAPE_POLY_SET*>( this );
            iter.m_currentPolygon = aFirst;
            iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentContour = 0;
            iter.m_currentVertex = 0;
            iter.m_iterateHoles = aIterateHoles;

            return iter;
        }

        CONST_ITERATOR CIterate( int aOutline ) const
        {
            return CIterate( aOutline, aOutline );
        }

        CONST_ITERATOR CIterateWithHoles( int aOutline ) const
        {
            return CIterate( aOutline, aOutline, true );
        }

        CONST_ITERATOR CIterate() const
        {
            return CIterate( 0, OutlineCount() - 1 );
        }

        CONST_ITERATOR CIterateWithHoles() const
        {
            return CIterate( 0, OutlineCount() - 1, true );
        }

        ITERATOR IterateFromVertexWithHoles( int aGlobalIdx )
        {
            // Build iterator
            ITERATOR iter = IterateWithHoles();

            // Get the relative indices of the globally indexed vertex
            VERTEX_INDEX indices;

            if( !GetRelativeIndices( aGlobalIdx, &indices ) )
                throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );

            // Adjust where the iterator is pointing
            iter.m_currentPolygon = indices.m_polygon;
            iter.m_currentContour = indices.m_contour;
            iter.m_currentVertex = indices.m_vertex;

            return iter;
        }

        ///> Returns an iterator object, for iterating between aFirst and aLast outline, with or
        /// without holes (default: without)
        SEGMENT_ITERATOR IterateSegments( int aFirst, int aLast, bool aIterateHoles = false )
        {
            SEGMENT_ITERATOR iter;

            iter.m_poly = this;
            iter.m_currentPolygon = aFirst;
            iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentContour = 0;
            iter.m_currentSegment = 0;
            iter.m_iterateHoles = aIterateHoles;

            return iter;
        }

        ///> Returns an iterator object, for iterating aPolygonIdx-th polygon edges
        SEGMENT_ITERATOR IterateSegments( int aPolygonIdx )
        {
            return IterateSegments( aPolygonIdx, aPolygonIdx );
        }

        ///> Returns an iterator object, for all outlines in the set (no holes)
        SEGMENT_ITERATOR IterateSegments()
        {
            return IterateSegments( 0, OutlineCount() - 1 );
        }

        ///> Returns an iterator object, for all outlines in the set (with holes)
        SEGMENT_ITERATOR IterateSegmentsWithHoles()
        {
            return IterateSegments( 0, OutlineCount() - 1, true );
        }

        ///> Returns an iterator object, for the aOutline-th outline in the set (with holes)
        SEGMENT_ITERATOR IterateSegmentsWithHoles( int aOutline )
        {
            return IterateSegments( aOutline, aOutline, true );
        }

        /** operations on polygons use a aFastMode param
         * if aFastMode is PM_FAST (true) the result can be a weak polygon
         * if aFastMode is PM_STRICTLY_SIMPLE (false) (default) the result is (theorically) a strictly
         * simple polygon, but calculations can be really significantly time consuming
         * Most of time PM_FAST is preferable.
         * PM_STRICTLY_SIMPLE can be used in critical cases (Gerber output for instance)
         */
        enum POLYGON_MODE
        {
            PM_FAST = true,
            PM_STRICTLY_SIMPLE = false
        };

        ///> Performs boolean polyset union
        ///> For aFastMode meaning, see function booleanOp
        void BooleanAdd( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLY_SET& b, POLYGON_MODE aFastMode );

        ///> Performs boolean polyset union between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanAdd( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                         POLYGON_MODE aFastMode );

        ///> Performs boolean polyset difference between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanSubtract( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                              POLYGON_MODE aFastMode );

        ///> Performs boolean polyset intersection between a and b, store the result in it self
        ///> For aFastMode meaning, see function booleanOp
        void BooleanIntersection( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b,
                                  POLYGON_MODE aFastMode );

        ///> Performs outline inflation/deflation, using round corners.
        void Inflate( int aFactor, int aCircleSegmentsCount );

        ///> Converts a set of polygons with holes to a singe outline with "slits"/"fractures" connecting the outer ring
        ///> to the inner holes
        ///> For aFastMode meaning, see function booleanOp
        void Fracture( POLYGON_MODE aFastMode );

        ///> Converts a single outline slitted ("fractured") polygon into a set ouf outlines
        ///> with holes.
        void Unfracture( POLYGON_MODE aFastMode );

        ///> Returns true if the polygon set has any holes.
        bool HasHoles() const;

        ///> Returns true if the polygon set has any holes tha share a vertex.
        bool HasTouchingHoles() const;


        ///> Simplifies the polyset (merges overlapping polys, eliminates degeneracy/self-intersections)
        ///> For aFastMode meaning, see function booleanOp
        void Simplify( POLYGON_MODE aFastMode );

        /**
         * Function NormalizeAreaOutlines
         * Convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s)
         * Removes null segments.
         * @return int - the polygon count (always >= 1, because there is at least one polygon)
         *             There are new polygons only if the polygon count is > 1.
         */
        int NormalizeAreaOutlines();

        /// @copydoc SHAPE::Format()
        const std::string Format() const override;

        /// @copydoc SHAPE::Parse()
        bool Parse( std::stringstream& aStream ) override;

        /// @copydoc SHAPE::Move()
        void Move( const VECTOR2I& aVector ) override;

        /**
         * Function Rotate
         * rotates all vertices by a given angle
         * @param aCenter is the rotation center
         * @param aAngle rotation angle in radians
         */
        void Rotate( double aAngle, const VECTOR2I& aCenter );

        /// @copydoc SHAPE::IsSolid()
        bool IsSolid() const override
        {
            return true;
        }

        const BOX2I BBox( int aClearance = 0 ) const override;

        /**
         * Function PointOnEdge()
         *
         * Checks if point aP lies on an edge or vertex of some of the outlines or holes.
         * @param aP is the point to check.
         * @return bool - true if the point lies on the edge of any polygon.
         */
        bool PointOnEdge( const VECTOR2I& aP ) const;

        /**
         * Function Collide
         * Checks whether the point aP collides with the inside of the polygon set; if the point
         * lies on an edge or on a corner of any of the polygons, there is no collision: the edges
         * does not belong to the polygon itself.
         * @param  aP         is the VECTOR2I point whose collision with respect to the poly set
         *                    will be tested.
         * @param  aClearance is the security distance; if the point lies closer to the polygon
         *                    than aClearance distance, then there is a collision.
         * @return bool - true if the point aP collides with the polygon; false in any other case.
         */
        bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const override;

        /**
         * Function Collide
         * Checks whether the segment aSeg collides with the inside of the polygon set;  if the
         * segment touches an edge or a corner of any of the polygons, there is no collision:
         * the edges do not belong to the polygon itself.
         * @param  aSeg       is the SEG segment whose collision with respect to the poly set
         *                    will be tested.
         * @param  aClearance is the security distance; if the segment passes closer to the polygon
         *                    than aClearance distance, then there is a collision.
         * @return bool - true if the segment aSeg collides with the polygon;
         *                    false in any other case.
         */
        bool Collide( const SEG& aSeg, int aClearance = 0 ) const override;

        /**
         * Function CollideVertex
         * Checks whether aPoint collides with any vertex of any of the contours of the polygon.
         * @param  aPoint     is the VECTOR2I point whose collision with respect to the polygon
         *                    will be tested.
         * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
         *                    aClearance distance, then there is a collision.
         * @param aClosestVertex is the index of the closes vertex to \p aPoint.
         * @return bool - true if there is a collision, false in any other case.
         */
        bool CollideVertex( const VECTOR2I& aPoint, VERTEX_INDEX& aClosestVertex,
                int aClearance = 0 );

        /**
         * Function CollideEdge
         * Checks whether aPoint collides with any edge of any of the contours of the polygon.
         * @param  aPoint     is the VECTOR2I point whose collision with respect to the polygon
         *                    will be tested.
         * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
         *                    aClearance distance, then there is a collision.
         * @param aClosestVertex is the index of the closes vertex to \p aPoint.
         * @return bool - true if there is a collision, false in any other case.
         */
        bool CollideEdge( const VECTOR2I& aPoint, VERTEX_INDEX& aClosestVertex,
                int aClearance = 0 );

        /**
         * Returns true if a given subpolygon contains the point aP
         *
         * @param aP is the point to check
         * @param aSubpolyIndex is the subpolygon to check, or -1 to check all
         * @param aIgnoreHoles controls whether or not internal holes are considered
         * @return true if the polygon contains the point
         */
        bool Contains( const VECTOR2I& aP, int aSubpolyIndex = -1, bool aIgnoreHoles = false ) const;

        ///> Returns true if the set is empty (no polygons at all)
        bool IsEmpty() const
        {
            return m_polys.size() == 0;
        }

        /**
         * Function RemoveVertex
         * deletes the aGlobalIndex-th vertex.
         * @param aGlobalIndex is the global index of the to-be-removed vertex.
         */
        void RemoveVertex( int aGlobalIndex );

        /**
         * Function RemoveVertex
         * deletes the vertex indexed by aIndex (index of polygon, contour and vertex).
         * @param aRelativeIndices is the set of relative indices of the to-be-removed vertex.
         */
        void RemoveVertex( VERTEX_INDEX aRelativeIndices );

        ///> Removes all outlines & holes (clears) the polygon set.
        void RemoveAllContours();

        /**
         * Function RemoveContour
         * deletes the aContourIdx-th contour of the aPolygonIdx-th polygon in the set.
         * @param aContourIdx is the index of the contour in the aPolygonIdx-th polygon to be
         *                    removed.
         * @param aPolygonIdx is the index of the polygon in which the to-be-removed contour is.
         *                    Defaults to the last polygon in the set.
         */
        void RemoveContour( int aContourIdx, int aPolygonIdx = -1 );

        /**
         * Function RemoveNullSegments
         * looks for null segments; ie, segments whose ends are exactly the same and deletes them.
         * @return int - the number of deleted segments.
         */
        int RemoveNullSegments();

        ///> Returns total number of vertices stored in the set.
        int TotalVertices() const;

        ///> Deletes aIdx-th polygon from the set
        void DeletePolygon( int aIdx );

        /**
         * Function Chamfer
         * returns a chamfered version of the aIndex-th polygon.
         * @param aDistance is the chamfering distance.
         * @param aIndex is the index of the polygon to be chamfered.
         * @return POLYGON - A polygon containing the chamfered version of the aIndex-th polygon.
         */
        POLYGON ChamferPolygon( unsigned int aDistance, int aIndex = 0 );

        /**
         * Function Fillet
         * returns a filleted version of the aIndex-th polygon.
         * @param aRadius is the fillet radius.
         * @param aErrorMax is the maximum allowable deviation of the polygon from the circle
         * @param aIndex is the index of the polygon to be filleted
         * @return POLYGON - A polygon containing the filleted version of the aIndex-th polygon.
         */
        POLYGON FilletPolygon( unsigned int aRadius, int aErrorMax, int aIndex = 0 );

        /**
         * Function Chamfer
         * returns a chamfered version of the polygon set.
         * @param aDistance is the chamfering distance.
         * @return SHAPE_POLY_SET - A set containing the chamfered version of this set.
         */
        SHAPE_POLY_SET Chamfer(  int aDistance );

        /**
         * Function Fillet
         * returns a filleted version of the polygon set.
         * @param aRadius is the fillet radius.
         * @param aErrorMax is the maximum allowable deviation of the polygon from the circle
         * @return SHAPE_POLY_SET - A set containing the filleted version of this set.
         */
        SHAPE_POLY_SET Fillet(  int aRadius, int aErrorMax );

        /**
         * Function DistanceToPolygon
         * computes the minimum distance between the aIndex-th polygon and aPoint.
         * @param  aPoint is the point whose distance to the aIndex-th polygon has to be measured.
         * @param  aIndex is the index of the polygon whose distace to aPoint has to be measured.
         * @return int -  The minimum distance between aPoint and all the segments of the aIndex-th
         *                polygon. If the point is contained in the polygon, the distance is zero.
         */
        int DistanceToPolygon( VECTOR2I aPoint, int aIndex );

        /**
         * Function DistanceToPolygon
         * computes the minimum distance between the aIndex-th polygon and aSegment with a
         * possible width.
         * @param  aSegment is the segment whose distance to the aIndex-th polygon has to be
         *                  measured.
         * @param  aIndex   is the index of the polygon whose distace to aPoint has to be measured.
         * @param  aSegmentWidth is the width of the segment; defaults to zero.
         * @return int -    The minimum distance between aSegment and all the segments of the
         *                  aIndex-th polygon. If the point is contained in the polygon, the
         *                  distance is zero.
         */
        int DistanceToPolygon( SEG aSegment, int aIndex, int aSegmentWidth = 0 );

        /**
         * Function DistanceToPolygon
         * computes the minimum distance between aPoint and all the polygons in the set
         * @param  aPoint is the point whose distance to the set has to be measured.
         * @return int -  The minimum distance between aPoint and all the polygons in the set. If
         *                the point is contained in any of the polygons, the distance is zero.
         */
        int Distance( VECTOR2I aPoint );

        /**
         * Function DistanceToPolygon
         * computes the minimum distance between aSegment and all the polygons in the set.
         * @param  aSegment is the segment whose distance to the polygon set has to be measured.
         * @param  aSegmentWidth is the width of the segment; defaults to zero.
         * @return int -    The minimum distance between aSegment and all the polygons in the set.
         *                  If the point is contained in the polygon, the distance is zero.
         */
        int Distance( const SEG& aSegment, int aSegmentWidth = 0 );

        /**
         * Function IsVertexInHole.
         * checks whether the aGlobalIndex-th vertex belongs to a hole.
         * @param  aGlobalIdx is the index of the vertex.
         * @return bool - true if the globally indexed aGlobalIdx-th vertex belongs to a hole.
         */
        bool IsVertexInHole( int aGlobalIdx );

    private:

        SHAPE_LINE_CHAIN& getContourForCorner( int aCornerId, int& aIndexWithinContour );
        VECTOR2I& vertex( int aCornerId );
        const VECTOR2I& cvertex( int aCornerId ) const;


        void fractureSingle( POLYGON& paths );
        void unfractureSingle ( POLYGON& path );
        void importTree( ClipperLib::PolyTree* tree );

        /** Function booleanOp
         * this is the engine to execute all polygon boolean transforms
         * (AND, OR, ... and polygon simplification (merging overlaping  polygons)
         * @param aType is the transform type ( see ClipperLib::ClipType )
         * @param aOtherShape is the SHAPE_LINE_CHAIN to combine with me.
         * @param aFastMode is an option to choose if the result can be a weak polygon
         * or a stricty simple polygon.
         * if aFastMode is PM_FAST the result can be a weak polygon
         * if aFastMode is PM_STRICTLY_SIMPLE (default) the result is (theorically) a strictly
         * simple polygon, but calculations can be really significantly time consuming
         */
        void booleanOp( ClipperLib::ClipType aType,
                        const SHAPE_POLY_SET& aOtherShape, POLYGON_MODE aFastMode );

        void booleanOp( ClipperLib::ClipType aType,
                        const SHAPE_POLY_SET& aShape,
                        const SHAPE_POLY_SET& aOtherShape, POLYGON_MODE aFastMode );

        bool pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const;

        const ClipperLib::Path convertToClipper( const SHAPE_LINE_CHAIN& aPath, bool aRequiredOrientation );
        const SHAPE_LINE_CHAIN convertFromClipper( const ClipperLib::Path& aPath );

        /**
         * containsSingle function
         * Checks whether the point aP is inside the aSubpolyIndex-th polygon of the polyset. If
         * the points lies on an edge, the polygon is considered to contain it.
         * @param  aP            is the VECTOR2I point whose position with respect to the inside of
         *                       the aSubpolyIndex-th polygon will be tested.
         * @param  aSubpolyIndex is an integer specifying which polygon in the set has to be
         *                       checked.
         * @param  aIgnoreHoles  can be set to true to ignore internal holes in the polygon
         * @return bool - true if aP is inside aSubpolyIndex-th polygon; false in any other
         *         case.
         */
        bool containsSingle( const VECTOR2I& aP, int aSubpolyIndex, bool aIgnoreHoles = false ) const;

        /**
         * Operations ChamferPolygon and FilletPolygon are computed under the private chamferFillet
         * method; this enum is defined to make the necessary distinction when calling this method
         * from the public ChamferPolygon and FilletPolygon methods.
         */
        enum CORNER_MODE
        {
            CHAMFERED,
            FILLETED
        };



        /**
         * Function chamferFilletPolygon
         * Returns the camfered or filleted version of the aIndex-th polygon in the set, depending
         * on the aMode selected
         * @param  aMode     represent which action will be taken: CORNER_MODE::CHAMFERED will
         *                   return a chamfered version of the polygon, CORNER_MODE::FILLETED will
         *                   return a filleted version of the polygon.
         * @param  aDistance is the chamfering distance if aMode = CHAMFERED; if aMode = FILLETED,
         *                   is the filleting radius.
         * @param  aIndex    is the index of the polygon that will be chamfered/filleted.
         * @param  aErrorMax is the maximum allowable deviation of the polygon from the circle
         *                   if aMode = FILLETED. If aMode = CHAMFERED, it is unused.
         * @return POLYGON - the chamfered/filleted version of the polygon.
         */
        POLYGON chamferFilletPolygon( CORNER_MODE aMode, unsigned int aDistance,
                                      int aIndex, int aErrorMax = -1 );

        ///> Returns true if the polygon set has any holes that touch share a vertex.
        bool hasTouchingHoles( const POLYGON& aPoly ) const;

        typedef std::vector<POLYGON> POLYSET;

        POLYSET m_polys;

    public:

        SHAPE_POLY_SET& operator=( const SHAPE_POLY_SET& );

        void CacheTriangulation();
        bool IsTriangulationUpToDate() const;

        MD5_HASH GetHash() const;

    private:
        void triangulateSingle( const POLYGON& aPoly, SHAPE_POLY_SET::TRIANGULATED_POLYGON& aResult );

        MD5_HASH checksum() const;

        std::vector<std::unique_ptr<TRIANGULATED_POLYGON>> m_triangulatedPolys;
        bool m_triangulationValid = false;
        MD5_HASH m_hash;

};

#endif
