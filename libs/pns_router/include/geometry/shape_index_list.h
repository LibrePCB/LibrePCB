/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __SHAPE_INDEX_LIST_H
#define __SHAPE_INDEX_LIST_H

template <class T>
const SHAPE* defaultShapeFunctor( const T aItem )
{
    return aItem->Shape();
}

template <class T, const SHAPE* (ShapeFunctor) (const T) = defaultShapeFunctor<T> >
class SHAPE_INDEX_LIST
{
    struct SHAPE_ENTRY
    {
        SHAPE_ENTRY( T aParent )
        {
            shape = ShapeFunctor( aParent );
            bbox = shape->BBox( 0 );
            parent = aParent;
        }

        ~SHAPE_ENTRY()
        {
        }

        T parent;
        const SHAPE* shape;
        BOX2I bbox;
    };

    typedef std::vector<SHAPE_ENTRY> SHAPE_VEC;
    typedef typename std::vector<SHAPE_ENTRY>::iterator SHAPE_VEC_ITER;

public:
    // "Normal" iterator interface, for STL algorithms.
    class iterator
    {
    public:
        iterator()
        {}

        iterator( SHAPE_VEC_ITER aCurrent ) :
            m_current( aCurrent )
        {}

        iterator( const iterator& aB ) :
            m_current( aB.m_current )
        {}

        T operator*() const
        {
            return (*m_current).parent;
        }

        void operator++()
        {
            ++m_current;
        }

        iterator& operator++( int aDummy )
        {
            ++m_current;
            return *this;
        }

        bool operator==( const iterator& aRhs ) const
        {
            return m_current == aRhs.m_current;
        }

        bool operator!=( const iterator& aRhs ) const
        {
            return m_current != aRhs.m_current;
        }

        const iterator& operator=( const iterator& aRhs )
        {
            m_current = aRhs.m_current;
            return *this;
        }

    private:
        SHAPE_VEC_ITER m_current;
    };

    // "Query" iterator, for iterating over a set of spatially matching shapes.
    class query_iterator
    {
    public:
        query_iterator()
        {
        }

        query_iterator( SHAPE_VEC_ITER aCurrent, SHAPE_VEC_ITER aEnd, SHAPE* aShape,
                        int aMinDistance, bool aExact ) :
              m_end( aEnd ),
              m_current( aCurrent ),
              m_shape( aShape ),
              m_minDistance( aMinDistance ),
              m_exact( aExact )
        {
         if( aShape )
         {
                m_refBBox = aShape->BBox();
                next();
            }
        }

        query_iterator( const query_iterator& aB ) :
              m_end( aB.m_end ),
              m_current( aB.m_current ),
              m_shape( aB.m_shape ),
              m_minDistance( aB.m_minDistance ),
              m_exact( aB.m_exact ),
              m_refBBox( aB.m_refBBox )
        {
        }

        T operator*() const
        {
            return (*m_current).parent;
        }

        query_iterator& operator++()
        {
            ++m_current;
            next();
             return *this;
        }

        query_iterator& operator++( int aDummy )
        {
            ++m_current;
            next();
            return *this;
        }

        bool operator==( const query_iterator& aRhs ) const
        {
            return m_current == aRhs.m_current;
        }

        bool operator!=( const query_iterator& aRhs ) const
        {
            return m_current != aRhs.m_current;
        }

        const query_iterator& operator=( const query_iterator& aRhs )
        {
            m_end = aRhs.m_end;
            m_current = aRhs.m_current;
            m_shape = aRhs.m_shape;
            m_minDistance = aRhs.m_minDistance;
            m_exact = aRhs.m_exact;
            m_refBBox = aRhs.m_refBBox;
            return *this;
        }

    private:
        void next()
        {
            while( m_current != m_end )
            {
                if( m_refBBox.Distance( m_current->bbox ) <= m_minDistance )
                {
                    if( !m_exact || m_current->shape->Collide( m_shape, m_minDistance ) )
                        return;
                }

                ++m_current;
            }
        }

        SHAPE_VEC_ITER m_end;
        SHAPE_VEC_ITER m_current;
        BOX2I   m_refBBox;
        bool    m_exact;
        SHAPE* m_shape;
        int m_minDistance;
    };

    void Add( T aItem )
    {
        SHAPE_ENTRY s( aItem );

        m_shapes.push_back( s );
    }

    void Remove( const T aItem )
    {
        SHAPE_VEC_ITER i;

        for( i = m_shapes.begin(); i != m_shapes.end(); ++i )
        {
            if( i->parent == aItem )
                break;
        }

        if( i == m_shapes.end() )
            return;

        m_shapes.erase( i );
    }

    int Size() const
    {
        return m_shapes.size();
    }

    template <class Visitor>
    int Query( const SHAPE* aShape, int aMinDistance, Visitor& aV, bool aExact = true )    // const
    {
        SHAPE_VEC_ITER i;
        int n = 0;
        VECTOR2I::extended_type minDistSq = (VECTOR2I::extended_type) aMinDistance * aMinDistance;

        BOX2I refBBox = aShape->BBox();

        for( i = m_shapes.begin(); i != m_shapes.end(); ++i )
        {
            if( refBBox.SquaredDistance( i->bbox ) <= minDistSq )
            {
                if( !aExact || i->shape->Collide( aShape, aMinDistance ) )
                {
                    n++;

                    if( !aV( i->parent ) )
                        return n;
                }
            }
        }

        return n;
    }

    void Clear()
    {
        m_shapes.clear();
    }

    query_iterator qbegin( SHAPE* aShape, int aMinDistance, bool aExact )
    {
        return query_iterator( m_shapes.begin(), m_shapes.end(), aShape, aMinDistance, aExact );
    }

    const query_iterator qend()
    {
        return query_iterator( m_shapes.end(), m_shapes.end(), NULL, 0, false );
    }

    iterator begin()
    {
        return iterator( m_shapes.begin() );
    }

    iterator end()
    {
        return iterator( m_shapes.end() );
    }

private:
    SHAPE_VEC m_shapes;
};

#endif
