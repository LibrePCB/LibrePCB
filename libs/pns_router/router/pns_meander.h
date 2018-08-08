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

#ifndef __PNS_MEANDER_H
#define __PNS_MEANDER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

namespace PNS {

class MEANDER_PLACER_BASE;

///< Shapes of available meanders
enum MEANDER_TYPE {
        MT_SINGLE,          // _|^|_, single-sided
        MT_START,           // _|^|
        MT_FINISH,          // |^|_
        MT_TURN,            // |^| or |_|
        MT_CHECK_START,     // try fitting a start type, but don't produce a line
        MT_CHECK_FINISH,    // try fitting a finish type, but don't produce a line
        MT_CORNER,          // line corner
        MT_EMPTY            // no meander (straight line)
};

///> meander corner shape
enum MEANDER_STYLE {
    MEANDER_STYLE_ROUND = 1,          // rounded (90 degree arc)
    MEANDER_STYLE_CHAMFER             // chamfered (45 degree segment)
};

/**
 * Class MEANDER_SETTINGS
 *
 * Holds dimensions for the meandering algorithm.
 */
class MEANDER_SETTINGS
{
public:

    MEANDER_SETTINGS()
    {
        m_minAmplitude = 100000;
        m_maxAmplitude = 1000000;
        m_step = 50000;
        m_spacing = 600000;
        m_targetLength = 100000000;
        m_targetSkew = 0;
        m_cornerStyle = MEANDER_STYLE_ROUND;
        m_cornerRadiusPercentage = 100;
        m_lengthTolerance = 100000;
        m_cornerArcSegments = 8;
    }

    ///> minimum meandering amplitude
    int m_minAmplitude;
    ///> maximum meandering amplitude
    int m_maxAmplitude;
    ///> meandering period/spacing (see dialog picture for explanation)
    int m_spacing;
    ///> amplitude/spacing adjustment step
    int m_step;
    ///> desired length of the tuned line/diff pair
    int m_targetLength;
    ///> type of corners for the meandered line
    MEANDER_STYLE m_cornerStyle;
    ///> rounding percentage (0 - 100)
    int m_cornerRadiusPercentage;
    ///> allowable tuning error
    int m_lengthTolerance;
    ///> number of line segments for arc approximation
    int m_cornerArcSegments;
    ///> target skew value for diff pair de-skewing
    int m_targetSkew;
};

class MEANDERED_LINE;

/**
 * Class MEANDER_SETTINGS
 *
 * Holds the geometry of a single meander.
 */
class MEANDER_SHAPE
{
public:
    /**
     * Constructor
     *
     * @param aPlacer the meander placer instance
     * @param aWidth width of the meandered line
     * @param aIsDual when true, the shape contains two meandered
     *                lines at a given offset (diff pairs)
     */
    MEANDER_SHAPE( MEANDER_PLACER_BASE* aPlacer, int aWidth, bool aIsDual = false ) :
        m_placer( aPlacer ),
        m_dual( aIsDual ),
        m_width( aWidth ),
        m_baselineOffset( 0 )
    {
        // Do not leave unitialized members, and keep static analyser quiet:
        m_type = MT_SINGLE;
        m_amplitude = 0;
        m_side = false;
        m_baseIndex = 0;
        m_currentTarget = NULL;
        m_meanCornerRadius = 0;
    }

    /**
     * Function SetType()
     *
     * Sets the type of the meander.
     */
    void SetType( MEANDER_TYPE aType )
    {
        m_type = aType;
    }

    /**
     * Function Type()
     *
     * @return the type of the meander.
     */
    MEANDER_TYPE Type() const
    {
        return m_type;
    }

    /**
     * Function SetBaseIndex()
     *
     * Sets an auxillary index of the segment being meandered in its original LINE.
     */
    void SetBaseIndex( int aIndex )
    {
        m_baseIndex = aIndex;
    }

    /**
     * Function BaseIndex()
     *
     * @return auxillary index of the segment being meandered in its original LINE.
     */
    int BaseIndex() const
    {
        return m_baseIndex;
    }

    /**
     * Function Amplitude()
     *
     * @return the amplitude of the meander shape.
     */
    int Amplitude() const
    {
        return m_amplitude;
    }

    /**
     * Function MakeCorner()
     *
     * Creates a dummy meander shape representing a line corner. Used to define
     * the starts/ends of meandered segments.
     * @param aP1 corner point of the 1st line
     * @param aP2 corner point of the 2nd line (if m_dual == true)
     */
    void MakeCorner( VECTOR2I aP1, VECTOR2I aP2 = VECTOR2I( 0, 0 ) );

    /**
     * Function Resize()
     *
     * Changes the amplitude of the meander shape to aAmpl and recalculates
     * the resulting line chain.
     * @param aAmpl new amplitude.
     */
    void Resize( int aAmpl );

    /**
     * Function Recalculate()
     *
     * Recalculates the line chain representing the meanders's shape.
     */
    void Recalculate();

    /**
     * Function IsDual()
     *
     * @return true if the shape represents 2 parallel lines (diff pair).
     */
    bool IsDual() const
    {
        return m_dual;
    }

    /**
     * Function Side()
     *
     * @return true if the meander is to the right of its base segment.
     */
    bool Side() const
    {
        return m_side;
    }

    /**
     * Function End()
     *
     * @return end vertex of the base segment of the meander shape.
     */
    VECTOR2I End() const
    {
        return m_clippedBaseSeg.B;
    }

    /**
     * Function CLine()
     *
     * @return the line chain representing the shape of the meander.
     */
    const SHAPE_LINE_CHAIN& CLine( int aShape ) const
    {
        return m_shapes[aShape];
    }

    /**
     * Function MakeEmpty()
     *
     * Replaces the meander with straight bypass line(s), effectively
     * clearing it.
     */
    void MakeEmpty();

    /**
     * Function Fit()
     *
     * Attempts to fit a meander of a given type onto a segment, avoiding
     * collisions with other board features.
     * @param aType type of meander shape
     * @param aSeg base segment for meandering
     * @param aP start point of the meander
     * @param aSide side of aSeg to put the meander on (true = right)
     * @return true on success.
     */
    bool Fit( MEANDER_TYPE aType, const SEG& aSeg, const VECTOR2I& aP, bool aSide );

    /**
     * Function BaseSegment()
     *
     * Returns the base segment the meadner was fitted to.
     * @return the base segment.
     */
    const SEG& BaseSegment() const
    {
        return m_clippedBaseSeg;
    }

    /**
     * Function BaselineLength()
     *
     * @return length of the base segment for the meander (i.e.
     *         the minimum tuned length.
     */
    int BaselineLength() const;

    /**
     * Function MaxTunableLength()
     *
     * @return the length of the fitted line chain.
     */
    int MaxTunableLength() const;

    /**
     * Function Settings()
     *
     * @return the current meandering settings.
     */
    const MEANDER_SETTINGS& Settings() const;

    /**
     * Function Width()
     *
     * @return width of the meandered line.
     */
    int Width() const
    {
        return m_width;
    }

    /**
     * Function SetBaselineOffset()
     *
     * Sets the parallel offset between the base segment and the meandered
     * line. Used for dual menaders (diff pair) only.
     * @param aOffset the offset
     */
    void SetBaselineOffset( int aOffset )
    {
        m_baselineOffset = aOffset;
    }

private:
    friend class MEANDERED_LINE;

    ///> starts turtle drawing
    void start( SHAPE_LINE_CHAIN* aTarget, const VECTOR2D& aWhere, const VECTOR2D& aDir );
    ///> moves turtle forward by aLength
    void forward( int aLength );
    ///> turns the turtle by aAngle
    void turn( int aAngle );
    ///> tells the turtle to draw a mitered corner of given radius and turn direction
    void miter( int aRadius, bool aSide );
    ///> tells the turtle to draw an U-like shape
    void uShape( int aSides, int aCorner, int aTop );

    ///> generates a 90-degree circular arc
    SHAPE_LINE_CHAIN makeMiterShape( VECTOR2D aP, VECTOR2D aDir, bool aSide );

    ///> reflects a point onto other side of a given segment
    VECTOR2I reflect( VECTOR2I aP, const SEG& aLine );

    ///> produces a meander shape of given type
    SHAPE_LINE_CHAIN genMeanderShape( VECTOR2D aP, VECTOR2D aDir, bool aSide, MEANDER_TYPE aType, int aAmpl, int aBaselineOffset = 0 );

    ///> recalculates the clipped baseline after the parameters of
    ///> the meander have been changed.
    void updateBaseSegment();

    ///> returns sanitized corner radius value
    int cornerRadius() const;

    ///> returns sanitized spacing value
    int spacing() const;

    ///> the type
    MEANDER_TYPE m_type;
    ///> the placer that placed this meander
    MEANDER_PLACER_BASE* m_placer;
    ///> dual or single line
    bool m_dual;
    ///> width of the line
    int m_width;
    ///> amplitude of the meander
    int m_amplitude;
    ///> offset wrs the base segment (dual only)
    int m_baselineOffset;
    ///> average radius of meander corners (for correction of DP meanders)
    int m_meanCornerRadius;
    ///> first point of the meandered line
    VECTOR2I m_p0;
    ///> base segment (unclipped)
    SEG m_baseSeg;
    ///> base segment (clipped)
    SEG m_clippedBaseSeg;
    ///> side (true = right)
    bool m_side;
    ///> the actual shapes (0 used for single, both for dual)
    SHAPE_LINE_CHAIN m_shapes[2];
    ///> index of the meandered segment in the base line
    int m_baseIndex;
    ///> current turtle direction
    VECTOR2D m_currentDir;
    ///> current turtle position
    VECTOR2D m_currentPos;
    ///> the line the turtle is drawing on
    SHAPE_LINE_CHAIN* m_currentTarget;
};


/**
 * Class MEANDERED_LINE
 *
 * Represents a set of meanders fitted over a single or two lines.
 */
class MEANDERED_LINE
{
public:
    MEANDERED_LINE()
    {
        // Do not leave unitialized members, and keep static analyser quiet:
        m_placer = NULL;
        m_dual = false;
        m_width = 0;
        m_baselineOffset = 0;
    }

    /**
     * Constructor
     *
     * @param aPlacer the meander placer instance
     * @param aIsDual when true, the meanders are generated for two coupled lines
     */
    MEANDERED_LINE( MEANDER_PLACER_BASE* aPlacer, bool aIsDual = false ) :
        m_placer( aPlacer ),
        m_dual( aIsDual )
    {
        // Do not leave unitialized members, and keep static analyser quiet:
        m_width = 0;
        m_baselineOffset = 0;
    }

    ~MEANDERED_LINE()
    {
        Clear();
    }

    /**
     * Function AddCorner()
     *
     * Creates a dummy meander shape representing a line corner. Used to define
     * the starts/ends of meandered segments.
     * @param aA corner point of the 1st line
     * @param aB corner point of the 2nd line (if m_dual == true)
     */
    void AddCorner( const VECTOR2I& aA, const VECTOR2I& aB = VECTOR2I( 0, 0 ) );

    /**
     * Function AddMeander()
     *
     * Adds a new meander shape the the meandered line.
     * @param aShape the meander shape to add
     */
    void AddMeander( MEANDER_SHAPE* aShape );

    /**
     * Function Clear()
     *
     * Clears the line geometry, removing all corners and meanders.
     */
    void Clear();

    /**
     * Function SetWidth()
     *
     * Sets the line width.
     */
    void SetWidth( int aWidth )
    {
        m_width = aWidth;
    }

    /**
     * Function MeanderSegment()
     *
     * Fits maximum amplitude meanders on a given segment and adds to the
     * current line.
     * @param aSeg the base segment to meander
     * @param aBaseIndex index of the base segment in the original line
     */
    void MeanderSegment( const SEG& aSeg, int aBaseIndex = 0 );

    /// @copydoc MEANDER_SHAPE::SetBaselineOffset()
    void SetBaselineOffset( int aOffset )
    {
        m_baselineOffset = aOffset;
    }

    /**
     * Function Meanders()
     *
     * @return set of meander shapes for this line
     */
    std::vector<MEANDER_SHAPE*>& Meanders()
    {
        return m_meanders;
    }

    /**
     * Function CheckSelfIntersections()
     *
     * Checks if the given shape is intersecting with any other meander in
     * the current line.
     * @param aShape the shape to check
     * @param aClearance clearance value
     * @return true, if the meander shape is not colliding
     */
    bool CheckSelfIntersections( MEANDER_SHAPE* aShape, int aClearance );

    /**
     * Function Settings()
     *
     * @return the current meandering settings.
     */
    const MEANDER_SETTINGS& Settings() const;

private:
    VECTOR2I m_last;

    MEANDER_PLACER_BASE* m_placer;
    std::vector<MEANDER_SHAPE*> m_meanders;

    bool m_dual;
    int m_width;
    int m_baselineOffset;
};

}

#endif    // __PNS_MEANDER_H
