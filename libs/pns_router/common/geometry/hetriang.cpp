/*
 * Copyright (C) 1998, 2000-2007, 2010, 2011, 2012, 2013 SINTEF ICT,
 * Applied Mathematics, Norway.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Contact information: E-mail: tor.dokken@sintef.no
 * SINTEF ICT, Department of Applied Mathematics,
 * P.O. Box 124 Blindern,
 * 0314 Oslo, Norway.
 *
 * This file is part of TTL.
 *
 * TTL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TTL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with TTL. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In accordance with Section 7(b) of the GNU Affero General Public
 * License, a covered work must retain the producer line in every data
 * file that is created or manipulated using TTL.
 *
 * Other Usage
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial activities involving the TTL library without
 * disclosing the source code of your own applications.
 *
 * This file may be used in accordance with the terms contained in a
 * written agreement between you and SINTEF ICT.
 */

#include <ttl/halfedge/hetriang.h>
#include <ttl/halfedge/hetraits.h>
#include <ttl/ttl.h>
#include <algorithm>
#include <fstream>
#include <limits>
#include <class_board_connected_item.h>
#include <memory>

using namespace hed;

#ifdef TTL_USE_NODE_ID
  int NODE::id_count = 0;
#endif



//#define DEBUG_HE
#ifdef DEBUG_HE
#include <iostream>
static void errorAndExit( char* aMessage )
{
    cout << "\n!!! ERROR: "<< aMessage << " !!!\n" << endl;
    exit( -1 );
}
#endif


static EDGE_PTR getLeadingEdgeInTriangle( const EDGE_PTR& aEdge )
{
    EDGE_PTR edge = aEdge;

    // Code: 3EF (assumes triangle)
    if( !edge->IsLeadingEdge() )
    {
        edge = edge->GetNextEdgeInFace();

        if( !edge->IsLeadingEdge() )
            edge = edge->GetNextEdgeInFace();
    }

    if( !edge->IsLeadingEdge() )
    {
        return EDGE_PTR();
    }

    return edge;
}


static void getLimits( NODES_CONTAINER::iterator aFirst, NODES_CONTAINER::iterator aLast,
                       int& aXmin, int& aYmin, int& aXmax, int& aYmax)
{
    aXmin = aYmin = std::numeric_limits<int>::min();
    aXmax = aYmax = std::numeric_limits<int>::max();

    NODES_CONTAINER::iterator it;

    for( it = aFirst; it != aLast; ++it )
    {
        aXmin = std::min( aXmin, ( *it )->GetX() );
        aYmin = std::min( aYmin, ( *it )->GetY() );
        aXmax = std::max( aXmax, ( *it )->GetX() );
        aYmax = std::max( aYmax, ( *it )->GetY() );
    }
}


EDGE_PTR TRIANGULATION::InitTwoEnclosingTriangles( NODES_CONTAINER::iterator aFirst,
                                                  NODES_CONTAINER::iterator aLast)
{
    int xmin, ymin, xmax, ymax;
    getLimits( aFirst, aLast, xmin, ymin, xmax, ymax );

    // Add 10% of range:
    double fac = 10.0;
    double dx = ( xmax - xmin ) / fac;
    double dy = ( ymax - ymin ) / fac;

    NODE_PTR n1 = std::make_shared<NODE>( xmin - dx, ymin - dy );
    NODE_PTR n2 = std::make_shared<NODE>( xmax + dx, ymin - dy );
    NODE_PTR n3 = std::make_shared<NODE>( xmax + dx, ymax + dy );
    NODE_PTR n4 = std::make_shared<NODE>( xmin - dx, ymax + dy );

    // diagonal
    EDGE_PTR e1d = std::make_shared<EDGE>();
    EDGE_PTR e2d = std::make_shared<EDGE>();

    // lower triangle
    EDGE_PTR e11 = std::make_shared<EDGE>();
    EDGE_PTR e12 = std::make_shared<EDGE>();

    // upper triangle
    EDGE_PTR e21 = std::make_shared<EDGE>();
    EDGE_PTR e22 = std::make_shared<EDGE>();

    // lower triangle
    e1d->SetSourceNode( n3 );
    e1d->SetNextEdgeInFace( e11 );
    e1d->SetTwinEdge( e2d );
    addLeadingEdge( e1d );

    e11->SetSourceNode( n1 );
    e11->SetNextEdgeInFace( e12 );

    e12->SetSourceNode( n2 );
    e12->SetNextEdgeInFace( e1d );

    // upper triangle
    e2d->SetSourceNode( n1 );
    e2d->SetNextEdgeInFace( e21 );
    e2d->SetTwinEdge( e1d );
    addLeadingEdge( e2d );

    e21->SetSourceNode( n3 );
    e21->SetNextEdgeInFace( e22 );

    e22->SetSourceNode( n4 );
    e22->SetNextEdgeInFace( e2d );

    return e11;
}


TRIANGULATION::TRIANGULATION()
{
    m_helper = new ttl::TRIANGULATION_HELPER( *this );
}


TRIANGULATION::TRIANGULATION( const TRIANGULATION& aTriangulation )
{
    m_helper = 0;   // make coverity and static analysers quiet.
    // Triangulation: Copy constructor not present
    assert( false );
}


TRIANGULATION::~TRIANGULATION()
{
    cleanAll();
    delete m_helper;
}


void TRIANGULATION::CreateDelaunay( NODES_CONTAINER::iterator aFirst,
                                    NODES_CONTAINER::iterator aLast )
{
    cleanAll();

    EDGE_PTR bedge = InitTwoEnclosingTriangles( aFirst, aLast );
    DART dc( bedge );

    DART d_iter = dc;

    NODES_CONTAINER::iterator it;
    for( it = aFirst; it != aLast; ++it )
    {
        m_helper->InsertNode<TTLtraits>( d_iter, *it );
    }

    // In general (e.g. for the triangle based data structure), the initial dart
    // may have been changed.
    // It is the users responsibility to get a valid boundary dart here.
    // The half-edge data structure preserves the initial dart.
    // (A dart at the boundary can also be found by trying to locate a
    // triangle "outside" the triangulation.)

    // Assumes rectangular domain
    m_helper->RemoveRectangularBoundary<TTLtraits>( dc );
}


void TRIANGULATION::RemoveTriangle( EDGE_PTR& aEdge )
{
  EDGE_PTR e1 = getLeadingEdgeInTriangle( aEdge );

#ifdef DEBUG_HE
    if( !e1 )
    errorAndExit( "Triangulation::removeTriangle: could not find leading aEdge" );
#endif

    removeLeadingEdgeFromList( e1 );
    // cout << "No leading edges = " << leadingEdges_.size() << endl;
    // Remove the triangle
    EDGE_PTR e2( e1->GetNextEdgeInFace() );
    EDGE_PTR e3( e2->GetNextEdgeInFace() );

    e1->Clear();
    e2->Clear();
    e3->Clear();
}


void TRIANGULATION::ReverseSplitTriangle( EDGE_PTR& aEdge )
{
    // Reverse operation of splitTriangle
    EDGE_PTR e1( aEdge->GetNextEdgeInFace() );
    EDGE_PTR le( getLeadingEdgeInTriangle( e1 ) );
#ifdef DEBUG_HE
    if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
    removeLeadingEdgeFromList( le );

    EDGE_PTR e2( e1->GetNextEdgeInFace()->GetTwinEdge()->GetNextEdgeInFace() );
    le = getLeadingEdgeInTriangle( e2 );
#ifdef DEBUG_HE
    if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
    removeLeadingEdgeFromList( le );

    EDGE_PTR e3( aEdge->GetTwinEdge()->GetNextEdgeInFace()->GetNextEdgeInFace() );
    le = getLeadingEdgeInTriangle( e3 );
#ifdef DEBUG_HE
    if (!le)
    errorAndExit("Triangulation::removeTriangle: could not find leading edge");
#endif
    removeLeadingEdgeFromList( le );

    // The three triangles at the node have now been removed
    // from the triangulation, but the arcs have not been deleted.
    // Next delete the 6 half edges radiating from the node
    // The node is maintained by handle and need not be deleted explicitly
    EDGE_PTR estar = aEdge;
    EDGE_PTR enext = estar->GetTwinEdge()->GetNextEdgeInFace();
    estar->GetTwinEdge()->Clear();
    estar->Clear();

    estar = enext;
    enext = estar->GetTwinEdge()->GetNextEdgeInFace();
    estar->GetTwinEdge()->Clear();
    estar->Clear();

    enext->GetTwinEdge()->Clear();
    enext->Clear();

    // Create the new triangle
    e1->SetNextEdgeInFace( e2 );
    e2->SetNextEdgeInFace( e3 );
    e3->SetNextEdgeInFace( e1 );
    addLeadingEdge( e1 );
}


DART TRIANGULATION::CreateDart()
{
  // Return an arbitrary CCW dart
    return DART( *m_leadingEdges.begin() );
}


bool TRIANGULATION::removeLeadingEdgeFromList( EDGE_PTR& aLeadingEdge )
{
    // Remove the edge from the list of leading edges,
    // but don't delete it.
    // Also set flag for leading edge to false.
    // Must search from start of list. Since edges are added to the
    // start of the list during triangulation, this operation will
    // normally be fast (when used in the triangulation algorithm)
    std::list<EDGE_PTR>::iterator it;
    for( it = m_leadingEdges.begin(); it != m_leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        if( edge == aLeadingEdge )
        {
            edge->SetAsLeadingEdge( false );
            it = m_leadingEdges.erase( it );

            return true;
        }
    }

    return false;
}


void TRIANGULATION::cleanAll()
{
    for( EDGE_PTR& edge : m_leadingEdges )
        edge->SetNextEdgeInFace( EDGE_PTR() );
}


void TRIANGULATION::swapEdge( DART& aDart )
{
    SwapEdge( aDart.GetEdge() );
}


void TRIANGULATION::splitTriangle( DART& aDart, const NODE_PTR& aPoint )
{
    EDGE_PTR edge = SplitTriangle( aDart.GetEdge(), aPoint );
    aDart.Init( edge );
}


void TRIANGULATION::reverseSplitTriangle( DART& aDart )
{
    ReverseSplitTriangle( aDart.GetEdge() );
}


void TRIANGULATION::removeBoundaryTriangle( DART& aDart )
{
    RemoveTriangle( aDart.GetEdge() );
}


#ifdef TTL_USE_NODE_FLAG
void TRIANGULATION::FlagNodes( bool aFlag ) const
{
    std::list<EDGE_PTR>::const_iterator it;
    for( it = m_leadingEdges.begin(); it != m_leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        for( int i = 0; i < 3; ++i )
        {
            edge->GetSourceNode()->SetFlag( aFlag );
            edge = edge->GetNextEdgeInFace();
        }
    }
}


std::list<NODE_PTR>* TRIANGULATION::GetNodes() const
{
    FlagNodes( false );
    std::list<NODE_PTR>* nodeList = new std::list<NODE_PTR>;
    std::list<EDGE_PTR>::const_iterator it;

    for( it = m_leadingEdges.begin(); it != m_leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        for( int i = 0; i < 3; ++i )
        {
            const NODE_PTR& node = edge->GetSourceNode();

            if( node->GetFlag() == false )
            {
                nodeList->push_back( node );
                node->SetFlag( true );
            }
            edge = edge->GetNextEdgeInFace();
        }
    }
    return nodeList;
}
#endif


void TRIANGULATION::GetEdges( std::list<EDGE_PTR>& aEdges, bool aSkipBoundaryEdges  ) const
{
    // collect all arcs (one half edge for each arc)
    // (boundary edges are also collected).
    std::list<EDGE_PTR>::const_iterator it;

    for( it = m_leadingEdges.begin(); it != m_leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;
        for( int i = 0; i < 3; ++i )
        {
            EDGE_PTR twinedge = edge->GetTwinEdge();
            // only one of the half-edges

            if( ( !twinedge && !aSkipBoundaryEdges )
                    || ( twinedge && ( (size_t) edge.get() > (size_t) twinedge.get() ) ) )
                {
                    aEdges.push_front( edge );
                }

            edge = edge->GetNextEdgeInFace();
        }
    }
}


EDGE_PTR TRIANGULATION::SplitTriangle( EDGE_PTR& aEdge, const NODE_PTR& aPoint )
{
    // Add a node by just splitting a triangle into three triangles
    // Assumes the half aEdge is located in the triangle
    // Returns a half aEdge with source node as the new node

    // e#_n are new edges
    // e# are existing edges
    // e#_n and e##_n are new twin edges
    // e##_n are edges incident to the new node

    // Add the node to the structure
    //NODE_PTR new_node(new Node(x,y,z));

    NODE_PTR n1( aEdge->GetSourceNode() );
    EDGE_PTR e1( aEdge );

    EDGE_PTR e2( aEdge->GetNextEdgeInFace() );
    NODE_PTR n2( e2->GetSourceNode() );

    EDGE_PTR e3( e2->GetNextEdgeInFace() );
    NODE_PTR n3( e3->GetSourceNode() );

    EDGE_PTR e1_n = std::make_shared<EDGE>();
    EDGE_PTR e11_n = std::make_shared<EDGE>();
    EDGE_PTR e2_n = std::make_shared<EDGE>();
    EDGE_PTR e22_n = std::make_shared<EDGE>();
    EDGE_PTR e3_n = std::make_shared<EDGE>();
    EDGE_PTR e33_n = std::make_shared<EDGE>();

    e1_n->SetSourceNode( n1 );
    e11_n->SetSourceNode( aPoint );
    e2_n->SetSourceNode( n2 );
    e22_n->SetSourceNode( aPoint );
    e3_n->SetSourceNode( n3 );
    e33_n->SetSourceNode( aPoint );

    e1_n->SetTwinEdge( e11_n );
    e11_n->SetTwinEdge( e1_n );
    e2_n->SetTwinEdge( e22_n );
    e22_n->SetTwinEdge( e2_n );
    e3_n->SetTwinEdge( e33_n );
    e33_n->SetTwinEdge( e3_n );

    e1_n->SetNextEdgeInFace( e33_n );
    e2_n->SetNextEdgeInFace( e11_n );
    e3_n->SetNextEdgeInFace( e22_n );

    e11_n->SetNextEdgeInFace( e1 );
    e22_n->SetNextEdgeInFace( e2 );
    e33_n->SetNextEdgeInFace( e3 );

    // and update old's next aEdge
    e1->SetNextEdgeInFace( e2_n );
    e2->SetNextEdgeInFace( e3_n );
    e3->SetNextEdgeInFace( e1_n );

    // add the three new leading edges,
    // Must remove the old leading aEdge from the list.
    // Use the field telling if an aEdge is a leading aEdge
    // NOTE: Must search in the list!!!

    if( e1->IsLeadingEdge() )
        removeLeadingEdgeFromList( e1 );
    else if( e2->IsLeadingEdge() )
        removeLeadingEdgeFromList( e2 );
    else if( e3->IsLeadingEdge() )
        removeLeadingEdgeFromList( e3 );
    else
        assert( false );        // one of the edges should be leading

    addLeadingEdge( e1_n );
    addLeadingEdge( e2_n );
    addLeadingEdge( e3_n );

    // Return a half aEdge incident to the new node (with the new node as source node)

    return e11_n;
}


void TRIANGULATION::SwapEdge( EDGE_PTR& aDiagonal )
{
    // Note that diagonal is both input and output and it is always
    // kept in counterclockwise direction (this is not required by all
    // functions in TriangulationHelper now)

    // Swap by rotating counterclockwise
    // Use the same objects - no deletion or new objects
    EDGE_PTR eL( aDiagonal );
    EDGE_PTR eR( eL->GetTwinEdge() );
    EDGE_PTR eL_1( eL->GetNextEdgeInFace() );
    EDGE_PTR eL_2( eL_1->GetNextEdgeInFace() );
    EDGE_PTR eR_1( eR->GetNextEdgeInFace() );
    EDGE_PTR eR_2( eR_1->GetNextEdgeInFace() );

    // avoid node to be dereferenced to zero and deleted
    NODE_PTR nR( eR_2->GetSourceNode() );
    NODE_PTR nL( eL_2->GetSourceNode() );

    eL->SetSourceNode( nR );
    eR->SetSourceNode( nL );

    // and now 6 1-sewings
    eL->SetNextEdgeInFace( eL_2 );
    eL_2->SetNextEdgeInFace( eR_1 );
    eR_1->SetNextEdgeInFace( eL );

    eR->SetNextEdgeInFace( eR_2 );
    eR_2->SetNextEdgeInFace( eL_1 );
    eL_1->SetNextEdgeInFace( eR );

    if( eL->IsLeadingEdge() )
        removeLeadingEdgeFromList( eL );
    else if( eL_1->IsLeadingEdge() )
        removeLeadingEdgeFromList( eL_1 );
    else if( eL_2->IsLeadingEdge() )
        removeLeadingEdgeFromList( eL_2 );

    if( eR->IsLeadingEdge() )
        removeLeadingEdgeFromList( eR );
    else if( eR_1->IsLeadingEdge() )
        removeLeadingEdgeFromList( eR_1 );
    else if( eR_2->IsLeadingEdge() )
        removeLeadingEdgeFromList( eR_2 );

    addLeadingEdge( eL );
    addLeadingEdge( eR );
}


bool TRIANGULATION::CheckDelaunay() const
{
    // ???? outputs !!!!
    // ofstream os("qweND.dat");
    const std::list<EDGE_PTR>& leadingEdges = GetLeadingEdges();

    std::list<EDGE_PTR>::const_iterator it;
    bool ok = true;
    int noNotDelaunay = 0;

    for( it = leadingEdges.begin(); it != leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        for( int i = 0; i < 3; ++i )
        {
            EDGE_PTR twinedge = edge->GetTwinEdge();

            // only one of the half-edges
            if( !twinedge || (size_t) edge.get() > (size_t) twinedge.get() )
            {
                DART dart( edge );
                if( m_helper->SwapTestDelaunay<TTLtraits>( dart ) )
                {
                    noNotDelaunay++;

                    //printEdge(dart,os); os << "\n";
                    ok = false;
                    //cout << "............. not Delaunay .... " << endl;
                }
            }

            edge = edge->GetNextEdgeInFace();
        }
    }

#ifdef DEBUG_HE
    cout << "!!! Triangulation is NOT Delaunay: " << noNotDelaunay << " edges\n" << endl;
#endif

    return ok;
}


void TRIANGULATION::OptimizeDelaunay()
{
    // This function is also present in ttl where it is implemented
    // generically.
    // The implementation below is tailored for the half-edge data structure,
    // and is thus more efficient

    // Collect all interior edges (one half edge for each arc)
    bool skip_boundary_edges = true;
    std::list<EDGE_PTR> elist;
    GetEdges( elist, skip_boundary_edges );

    // Assumes that elist has only one half-edge for each arc.
    bool cycling_check = true;
    bool optimal = false;
    std::list<EDGE_PTR>::const_iterator it;

    while( !optimal )
    {
        optimal = true;

        for( it = elist.begin(); it != elist.end(); ++it )
        {
            EDGE_PTR edge = *it;

            DART dart( edge );
            // Constrained edges should not be swapped
            if( m_helper->SwapTestDelaunay<TTLtraits>( dart, cycling_check ) )
            {
                optimal = false;
                SwapEdge( edge );
            }
        }
    }
}


EDGE_PTR TRIANGULATION::GetInteriorNode() const
{
    const std::list<EDGE_PTR>& leadingEdges = GetLeadingEdges();
    std::list<EDGE_PTR>::const_iterator it;

    for( it = leadingEdges.begin(); it != leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        // multiple checks, but only until found
        for( int i = 0; i < 3; ++i )
        {
            if( edge->GetTwinEdge() )
            {
                if( !m_helper->IsBoundaryNode( DART( edge ) ) )
                    return edge;
            }

            edge = edge->GetNextEdgeInFace();
        }
    }

    return EDGE_PTR(); // no boundary nodes
}


EDGE_PTR TRIANGULATION::GetBoundaryEdgeInTriangle( const EDGE_PTR& aEdge ) const
{
    EDGE_PTR edge = aEdge;

    if( m_helper->IsBoundaryEdge( DART( edge ) ) )
        return edge;

    edge = edge->GetNextEdgeInFace();
    if( m_helper->IsBoundaryEdge( DART( edge ) ) )
        return edge;

    edge = edge->GetNextEdgeInFace();
    if( m_helper->IsBoundaryEdge( DART( edge ) ) )
        return edge;

    return EDGE_PTR();
}


EDGE_PTR TRIANGULATION::GetBoundaryEdge() const
{
    // Get an arbitrary (CCW) boundary edge
    // If the triangulation is closed, NULL is returned
    const std::list<EDGE_PTR>& leadingEdges = GetLeadingEdges();
    std::list<EDGE_PTR>::const_iterator it;
    EDGE_PTR edge;

    for( it = leadingEdges.begin(); it != leadingEdges.end(); ++it )
    {
        edge = GetBoundaryEdgeInTriangle( *it );

        if( edge )
            return edge;
    }
    return EDGE_PTR();
}


void TRIANGULATION::PrintEdges( std::ofstream& aOutput ) const
{
    // Print source node and target node for each edge face by face,
    // but only one of the half-edges.
    const std::list<EDGE_PTR>& leadingEdges = GetLeadingEdges();
    std::list<EDGE_PTR>::const_iterator it;

    for( it = leadingEdges.begin(); it != leadingEdges.end(); ++it )
    {
        EDGE_PTR edge = *it;

        for( int i = 0; i < 3; ++i )
        {
            EDGE_PTR twinedge = edge->GetTwinEdge();

            // Print only one edge (the highest value of the pointer)
            if( !twinedge || (size_t) edge.get() > (size_t) twinedge.get() )
            {
                // Print source node and target node
                NODE_PTR node = edge->GetSourceNode();
                aOutput << node->GetX() << " " << node->GetY() << std::endl;
                node = edge->GetTargetNode();
                aOutput << node->GetX() << " " << node->GetY() << std::endl;
                aOutput << '\n'; // blank line
            }

            edge = edge->GetNextEdgeInFace();
        }
    }
}
