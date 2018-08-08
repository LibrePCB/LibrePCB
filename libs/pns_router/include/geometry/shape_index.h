/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jacobo Aragunde Pérez
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

#ifndef __SHAPE_INDEX_H
#define __SHAPE_INDEX_H

#include <vector>
#include <geometry/shape.h>
#include <geometry/rtree.h>


/**
 * shapeFunctor template function
 *
 * It is used by SHAPE_INDEX to get a SHAPE* from another type.
 * By default relies on T::GetShape() method, should be specialized if the T object
 * doesn't allow that method.
 * @param aItem generic T object
 * @return a SHAPE* object equivalent to object.
 */
template <class T>
static const SHAPE* shapeFunctor( T aItem )
{
    return aItem->Shape();
}

/**
 * boundingBox template method
 *
 * It is used by SHAPE_INDEX to get the bounding box of a generic T object.
 * By default relies on T::BBox() method, should be specialized if the T object
 * doesn't allow that method.
 * @param aObject generic T object
 * @return a BOX2I object containing the bounding box of the T object.
 */
template <class T>
BOX2I boundingBox( T aObject )
{
    return shapeFunctor( aObject )->BBox();
}

/**
 * acceptVisitor template method
 *
 * It is used by SHAPE_INDEX to implement Accept().
 * By default relies on V::operation() redefinition, should be specialized if V class
 * doesn't have its () operation defined to accept T objects.
 * @param aObject generic T object
 * @param aVisitor V visitor object
 */
template <class T, class V>
void acceptVisitor( T aObject, V aVisitor )
{
    aVisitor( aObject );
}

/**
 * collide template method
 *
 * It is used by SHAPE_INDEX to implement Query().
 * By default relies on T::Collide(U) method, should be specialized if the T object
 * doesn't allow that method.
 * @param aObject generic T object
 * @param aAnotherObject generic U object
 * @param aMinDistance minimum collision distance
 * @return if object and anotherObject collide
 */
template <class T, class U>
bool collide( T aObject, U aAnotherObject, int aMinDistance )
{
    return shapeFunctor( aObject )->Collide( aAnotherObject, aMinDistance );
}

template <class T, class V>
bool queryCallback( T aShape, void* aContext )
{
    V* visitor = (V*) aContext;

    acceptVisitor<T, V>( aShape, *visitor );

    return true;
}

template <class T = SHAPE*>
class SHAPE_INDEX
{
    public:
        class Iterator
        {
        private:
            typedef typename RTree<T, int, 2, float>::Iterator RTreeIterator;
            RTreeIterator iterator;

            /**
             * Function Init()
             *
             * Setup the internal tree iterator.
             * @param aTree pointer to a RTREE object
             */
            void Init( RTree<T, int, 2, float>* aTree )
            {
                aTree->GetFirst( iterator );
            }

        public:
            /**
             * Iterator constructor
             *
             * Creates an iterator for the index object
             * @param aIndex SHAPE_INDEX object to iterate
             */
            Iterator( SHAPE_INDEX* aIndex )
            {
                Init( aIndex->m_tree );
            }

            /**
             * Operator * (prefix)
             *
             * Returns the next data element.
             */
            T operator*()
            {
                return *iterator;
            }

            /**
             * Operator ++ (prefix)
             *
             * Shifts the iterator to the next element.
             */
            bool operator++()
            {
                return ++iterator;
            }

            /**
             * Operator ++ (postfix)
             *
             * Shifts the iterator to the next element.
             */
            bool operator++( int )
            {
                return ++iterator;
            }

            /**
             * Function IsNull()
             *
             * Checks if the iterator has reached the end.
             * @return true if it is in an invalid position (data finished)
             */
            bool IsNull()
            {
                return iterator.IsNull();
            }

            /**
             * Function IsNotNull()
             *
             * Checks if the iterator has not reached the end.
             * @return true if it is in an valid position (data not finished)
             */
            bool IsNotNull()
            {
                return iterator.IsNotNull();
            }

            /**
             * Function Next()
             *
             * Returns the current element of the iterator and moves to the next
             * position.
             * @return SHAPE object pointed by the iterator before moving to the next position.
             */
            T Next()
            {
                T object = *iterator;
                ++iterator;

                return object;
            }
        };

        SHAPE_INDEX();

        ~SHAPE_INDEX();

        /**
         * Function Add()
         *
         * Adds a SHAPE to the index.
         * @param aShape is the new SHAPE.
         */
        void Add( T aShape );

        /**
         * Function Remove()
         *
         * Removes a SHAPE to the index.
         * @param aShape is the new SHAPE.
         */
        void Remove( T aShape );

        /**
         * Function RemoveAll()
         *
         * Removes all the contents of the index.
         */
        void RemoveAll();

        /**
         * Function Accept()
         *
         * Accepts a visitor for every SHAPE object contained in this INDEX.
         * @param aVisitor Visitor object to be run
         */
        template <class V>
        void Accept( V aVisitor )
        {
            Iterator iter = this->Begin();

            while( !iter.IsNull() )
            {
                T shape = *iter;
                acceptVisitor( shape, aVisitor );
                iter++;
            }
        }

        /**
         * Function Reindex()
         *
         * Rebuilds the index. This should be used if the geometry of the objects
         * contained by the index has changed.
         */
        void Reindex();

        /**
         * Function Query()
         *
         * Runs a callback on every SHAPE object contained in the bounding box of (shape).
         * @param aShape shape to search against
         * @param aMinDistance distance threshold
         * @param aVisitor object to be invoked on every object contained in the search area.
         */
        template <class V>
        int Query( const SHAPE *aShape, int aMinDistance, V& aVisitor, bool aExact )
        {
            BOX2I box = aShape->BBox();
            box.Inflate( aMinDistance );

            int min[2] = { box.GetX(),         box.GetY() };
            int max[2] = { box.GetRight(),     box.GetBottom() };

            return this->m_tree->Search( min, max, aVisitor );
        }

        /**
         * Function Begin()
         *
         * Creates an iterator for the current index object
         * @return iterator
         */
        Iterator Begin();

    private:
        RTree<T, int, 2, float>* m_tree;
};

/*
 * Class members implementation
 */

template <class T>
SHAPE_INDEX<T>::SHAPE_INDEX()
{
    this->m_tree = new RTree<T, int, 2, float>();
}

template <class T>
SHAPE_INDEX<T>::~SHAPE_INDEX()
{
    delete this->m_tree;
}

template <class T>
void SHAPE_INDEX<T>::Add( T aShape )
{
    BOX2I box = boundingBox( aShape );
    int min[2] = { box.GetX(), box.GetY() };
    int max[2] = { box.GetRight(), box.GetBottom() };

    this->m_tree->Insert( min, max, aShape );
}

template <class T>
void SHAPE_INDEX<T>::Remove( T aShape )
{
    BOX2I box = boundingBox( aShape );
    int min[2] = { box.GetX(), box.GetY() };
    int max[2] = { box.GetRight(), box.GetBottom() };

    this->m_tree->Remove( min, max, aShape );
}

template <class T>
void SHAPE_INDEX<T>::RemoveAll()
{
    this->m_tree->RemoveAll();
}

template <class T>
void SHAPE_INDEX<T>::Reindex()
{
    RTree<T, int, 2, float>* newTree;
    newTree = new RTree<T, int, 2, float>();

    Iterator iter = this->Begin();

    while( !iter.IsNull() )
    {
        T shape = *iter;
        BOX2I box = boundingBox( shape );
        int min[2] = { box.GetX(), box.GetY() };
        int max[2] = { box.GetRight(), box.GetBottom() };
        newTree->Insert( min, max, shape );
        iter++;
    }

    delete this->m_tree;
    this->m_tree = newTree;
}

template <class T>
typename SHAPE_INDEX<T>::Iterator SHAPE_INDEX<T>::Begin()
{
    return Iterator( this );
}

#endif
