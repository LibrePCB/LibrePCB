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

#ifndef __PNS_NODE_H
#define __PNS_NODE_H

#include <vector>
#include <list>
#include <unordered_set>
#include <set>
#include <unordered_map>

#include <core/optional.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_index.h>

#include "pns_item.h"
#include "pns_joint.h"
#include "pns_itemset.h"

namespace PNS {

class SEGMENT;
class LINE;
class SOLID;
class VIA;
class INDEX;
class ROUTER;
class NODE;

/**
 * Class RULE_RESOLVER
 *
 * An abstract function object, returning a design rule (clearance, diff pair gap, etc) required between two items.
 **/

class RULE_RESOLVER
{
public:
    virtual ~RULE_RESOLVER() {}

    virtual int Clearance( const ITEM* aA, const ITEM* aB ) const = 0;
    virtual int Clearance( int aNetCode ) const = 0;
    virtual int DpCoupledNet( int aNet ) = 0;
    virtual int DpNetPolarity( int aNet ) = 0;
    virtual bool DpNetPair( ITEM* aItem, int& aNetP, int& aNetN ) = 0;
};

/**
 * Struct OBSTACLE
 *
 * Holds an object colliding with another object, along with
 * some useful data about the collision.
 **/
struct OBSTACLE
{
    ///> Item we search collisions with
    const ITEM* m_head;

    ///> Item found to be colliding with m_head
    ITEM* m_item;

    ///> Hull of the colliding m_item
    SHAPE_LINE_CHAIN m_hull;

    ///> First and last intersection point between the head item and the hull
    ///> of the colliding m_item
    VECTOR2I m_ipFirst, m_ipLast;

    ///> ... and the distance thereof
    int m_distFirst, m_distLast;
};

/**
 * Struct OBSTACLE_VISITOR
 **/
class OBSTACLE_VISITOR {

public:

    OBSTACLE_VISITOR( const ITEM* aItem );

    void SetWorld( const NODE* aNode, const NODE* aOverride = NULL );

    virtual bool operator()( ITEM* aCandidate ) = 0;

protected:

    bool visit( ITEM* aCandidate );

    ///> the item we are looking for collisions with
    const ITEM* m_item;

    ///> node we are searching in (either root or a branch)
    const NODE* m_node;

    ///> node that overrides root entries
    const NODE* m_override;

    ///> additional clearance
    int m_extraClearance;
};

/**
 * Class NODE
 *
 * Keeps the router "world" - i.e. all the tracks, vias, solids in a
 * hierarchical and indexed way.
 * Features:
 * - spatial-indexed container for PCB item shapes
 * - collision search & clearance checking
 * - assembly of lines connecting joints, finding loops and unique paths
 * - lightweight cloning/branching (for recursive optimization and shove
 * springback)
 **/
class NODE
{
public:
    typedef OPT<OBSTACLE>   OPT_OBSTACLE;
    typedef std::vector<ITEM*>          ITEM_VECTOR;
    typedef std::vector<OBSTACLE>       OBSTACLES;

    NODE();
    ~NODE();

    ///> Returns the expected clearance between items a and b.
    int GetClearance( const ITEM* aA, const ITEM* aB ) const;

    ///> Returns the pre-set worst case clearance between any pair of items
    int GetMaxClearance() const
    {
        return m_maxClearance;
    }

    ///> Sets the worst-case clerance between any pair of items
    void SetMaxClearance( int aClearance )
    {
        m_maxClearance = aClearance;
    }

    ///> Assigns a clerance resolution function object
    void SetRuleResolver( RULE_RESOLVER* aFunc )
    {
        m_ruleResolver = aFunc;
    }

    RULE_RESOLVER* GetRuleResolver()
    {
        return m_ruleResolver;
    }

    ///> Returns the number of joints
    int JointCount() const
    {
        return m_joints.size();
    }

    ///> Returns the number of nodes in the inheritance chain (wrs to the root node)
    int Depth() const
    {
        return m_depth;
    }

    /**
     * Function QueryColliding()
     *
     * Finds items collliding (closer than clearance) with the item aItem.
     * @param aItem item to check collisions against
     * @param aObstacles set of colliding objects found
     * @param aKindMask mask of obstacle types to take into account
     * @param aLimitCount stop looking for collisions after finding this number of colliding items
     * @return number of obstacles found
     */
    int QueryColliding( const ITEM*  aItem,
                        OBSTACLES&   aObstacles,
                        int          aKindMask = ITEM::ANY_T,
                        int          aLimitCount = -1,
                        bool         aDifferentNetsOnly = true,
                        int          aForceClearance = -1 );

    int QueryColliding( const ITEM* aItem,
                         OBSTACLE_VISITOR& aVisitor
                      );

    /**
     * Function NearestObstacle()
     *
     * Follows the line in search of an obstacle that is nearest to the starting to the line's starting
     * point.
     * @param aItem the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE NearestObstacle( const LINE*             aItem,
                                  int                     aKindMask = ITEM::ANY_T,
                                  const std::set<ITEM*>*  aRestrictedSet = NULL );

    /**
     * Function CheckColliding()
     *
     * Checks if the item collides with anything else in the world,
     * and if found, returns the obstacle.
     * @param aItem the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const ITEM*     aItem,
                                 int             aKindMask = ITEM::ANY_T );


    /**
     * Function CheckColliding()
     *
     * Checks if any item in the set collides with anything else in the world,
     * and if found, returns the obstacle.
     * @param aSet set of items to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const ITEM_SET&  aSet,
                                 int              aKindMask = ITEM::ANY_T );


    /**
     * Function CheckColliding()
     *
     * Checks if 2 items collide.
     * and if found, returns the obstacle.
     * @param aItemA  first item to find collisions with
     * @param aItemB  second item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    bool CheckColliding( const ITEM*    aItemA,
                         const ITEM*    aItemB,
                         int            aKindMask = ITEM::ANY_T,
                         int            aForceClearance = -1 );

    /**
     * Function HitTest()
     *
     * Finds all items that contain the point aPoint.
     * @param aPoint the point
     * @return the items
     */
    const ITEM_SET HitTest( const VECTOR2I& aPoint ) const;

    /**
     * Function Add()
     *
     * Adds an item to the current node.
     * @param aSegment item to add
     * @param aAllowRedundant if true, duplicate items are allowed (e.g. a segment or via
     * @return true if added
     * at the same coordinates as an existing one)
     */
    bool Add( std::unique_ptr< SEGMENT > aSegment, bool aAllowRedundant = false );
    void Add( std::unique_ptr< SOLID >   aSolid );
    void Add( std::unique_ptr< VIA >     aVia );

    void Add( LINE& aLine, bool aAllowRedundant = false );

private:
    void Add( std::unique_ptr< ITEM > aItem, bool aAllowRedundant = false );

public:
    /**
     * Function Remove()
     *
     * Just as the name says, removes an item from this branch.
     */
    void Remove( SOLID* aSolid );
    void Remove( VIA* aVia );
    void Remove( SEGMENT* aSegment );
    void Remove( ITEM* aItem );

public:
    /**
     * Function Remove()
     *
     * Just as the name says, removes a line from this branch.
     * @param aLine item to remove
     */
    void Remove( LINE& aLine );

    /**
     * Function Replace()
     *
     * Just as the name says, replaces an item with another one.
     * @param aOldItem item to be removed
     * @param aNewItem item add instead
     */
    void Replace( ITEM* aOldItem, std::unique_ptr< ITEM > aNewItem );
    void Replace( LINE& aOldLine, LINE& aNewLine );

    /**
     * Function Branch()
     *
     * Creates a lightweight copy (called branch) of self that tracks
     * the changes (added/removed items) wrs to the root. Note that if there are
     * any branches in use, their parents must NOT be deleted.
     * @return the new branch
     */
    NODE* Branch();

    /**
     * Function AssembleLine()
     *
     * Follows the joint map to assemble a line connecting two non-trivial
     * joints starting from segment aSeg.
     * @param aSeg the initial segment
     * @param aOriginSegmentIndex index of aSeg in the resulting line
     * @return the line
     */
    const LINE AssembleLine( SEGMENT* aSeg, int* aOriginSegmentIndex = NULL,
                                 bool aStopAtLockedJoints = false );

    ///> Prints the contents and joints structure
    void Dump( bool aLong = false );

    /**
     * Function GetUpdatedItems()
     *
     * Returns the lists of items removed and added in this branch, with
     * respect to the root branch.
     * @param aRemoved removed items
     * @param aAdded added items
     */
    void GetUpdatedItems( ITEM_VECTOR& aRemoved, ITEM_VECTOR& aAdded );

    /**
     * Function Commit()
     *
     * Applies the changes from a given branch (aNode) to the root branch. Called on
     * a non-root branch will fail. Calling commit also kills all children nodes of the root branch.
     * @param aNode node to commit changes from
     */
    void Commit( NODE* aNode );

    /**
     * Function FindJoint()
     *
     * Searches for a joint at a given position, layer and belonging to given net.
     * @return the joint, if found, otherwise empty
     */
    JOINT* FindJoint( const VECTOR2I& aPos, int aLayer, int aNet );

    void LockJoint( const VECTOR2I& aPos, const ITEM* aItem, bool aLock );

    /**
     * Function FindJoint()
     *
     * Searches for a joint at a given position, linked to given item.
     * @return the joint, if found, otherwise empty
     */
    JOINT* FindJoint( const VECTOR2I& aPos, const ITEM* aItem )
    {
        return FindJoint( aPos, aItem->Layers().Start(), aItem->Net() );
    }

#if 0
    void MapConnectivity( JOINT* aStart, std::vector<JOINT*> & aFoundJoints );

    ITEM* NearestUnconnectedItem( JOINT* aStart, int* aAnchor = NULL,
                                      int aKindMask = ITEM::ANY_T);

#endif

    ///> finds all lines between a pair of joints. Used by the loop removal procedure.
    int FindLinesBetweenJoints( JOINT&                  aA,
                                JOINT&                  aB,
                                std::vector<LINE>&      aLines );

    ///> finds the joints corresponding to the ends of line aLine
    void FindLineEnds( const LINE& aLine, JOINT& aA, JOINT& aB );

    ///> Destroys all child nodes. Applicable only to the root node.
    void KillChildren();

    void AllItemsInNet( int aNet, std::set<ITEM*>& aItems );

    void ClearRanks( int aMarkerMask = MK_HEAD | MK_VIOLATION );

    int FindByMarker( int aMarker, ITEM_SET& aItems );
    int RemoveByMarker( int aMarker );

    ITEM* FindItemByParent( const class PNS_HORIZON_PARENT_ITEM* aParent, int net);

    bool HasChildren() const
    {
        return !m_children.empty();
    }

    ///> checks if this branch contains an updated version of the m_item
    ///> from the root branch.
    bool Overrides( ITEM* aItem ) const
    {
        return m_override.find( aItem ) != m_override.end();
    }

private:
    struct DEFAULT_OBSTACLE_VISITOR;
    typedef std::unordered_multimap<JOINT::HASH_TAG, JOINT, JOINT::JOINT_TAG_HASH> JOINT_MAP;
    typedef JOINT_MAP::value_type TagJointPair;

    /// nodes are not copyable
    NODE( const NODE& aB );
    NODE& operator=( const NODE& aB );

    ///> tries to find matching joint and creates a new one if not found
    JOINT& touchJoint( const VECTOR2I&     aPos,
                       const LAYER_RANGE&  aLayers,
                       int                 aNet );

    ///> touches a joint and links it to an m_item
    void linkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere );

    ///> unlinks an item from a joint
    void unlinkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere );

    ///> helpers for adding/removing items
    void addSolid( SOLID* aSeg );
    void addSegment( SEGMENT* aSeg );
    void addVia( VIA* aVia );

    void removeLine( LINE& aLine );
    void removeSolidIndex( SOLID* aSeg );
    void removeSegmentIndex( SEGMENT* aSeg );
    void removeViaIndex( VIA* aVia );

    void doRemove( ITEM* aItem );
    void unlinkParent();
    void releaseChildren();
    void releaseGarbage();

    bool isRoot() const
    {
        return m_parent == NULL;
    }

    SEGMENT* findRedundantSegment( const VECTOR2I& A, const VECTOR2I& B,
                                   const LAYER_RANGE & lr, int aNet );
    SEGMENT* findRedundantSegment( SEGMENT* aSeg );

    ///> scans the joint map, forming a line starting from segment (current).
    void followLine( SEGMENT*    aCurrent,
                     bool        aScanDirection,
                     int&        aPos,
                     int         aLimit,
                     VECTOR2I*   aCorners,
                     SEGMENT**   aSegments,
                     bool&       aGuardHit,
                     bool        aStopAtLockedJoints );

    ///> hash table with the joints, linking the items. Joints are hashed by
    ///> their position, layer set and net.
    JOINT_MAP m_joints;

    ///> node this node was branched from
    NODE* m_parent;

    ///> root node of the whole hierarchy
    NODE* m_root;

    ///> list of nodes branched from this one
    std::set<NODE*> m_children;

    ///> hash of root's items that have been changed in this node
    std::unordered_set<ITEM*> m_override;

    ///> worst case item-item clearance
    int m_maxClearance;

    ///> Design rules resolver
    RULE_RESOLVER* m_ruleResolver;

    ///> Geometric/Net index of the items
    INDEX* m_index;

    ///> depth of the node (number of parent nodes in the inheritance chain)
    int m_depth;

    std::unordered_set<ITEM*> m_garbageItems;
};

}

#endif
