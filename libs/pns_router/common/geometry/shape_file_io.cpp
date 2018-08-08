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

#include <string>
#include <cassert>

#include <geometry/shape.h>
#include <geometry/shape_file_io.h>

SHAPE_FILE_IO::SHAPE_FILE_IO()
{
    m_groupActive = false;
    m_mode = IOM_WRITE;
    m_file = stdout;
}

SHAPE_FILE_IO::SHAPE_FILE_IO( const std::string& aFilename, SHAPE_FILE_IO::IO_MODE aMode )
{
    m_groupActive = false;

    if( aFilename.length() )
    {
        switch( aMode )
        {
            case IOM_READ: m_file = fopen( aFilename.c_str(), "rb" ); break;
            case IOM_WRITE: m_file = fopen( aFilename.c_str(), "wb" ); break;
            case IOM_APPEND: m_file = fopen( aFilename.c_str(), "ab" ); break;
            default:
                return;
        }
    }
    else
    {
        m_file = NULL;
    }

    m_mode = aMode;
    // fixme: exceptions
}


SHAPE_FILE_IO::~SHAPE_FILE_IO()
{
    if( !m_file )
        return;

    if( m_groupActive && m_mode != IOM_READ )
        fprintf( m_file, "endgroup\n" );

    if ( m_file != stdout )
    {
        fclose( m_file );
    }
}


SHAPE* SHAPE_FILE_IO::Read()
{
 /*   char tmp[1024];

    do {

        if (fscanf(m_file, "%s", tmp) != 1)
            return NULL;

        if( !strcmp( tmp, "shape" )
            break;
    }

    int type;

    SHAPE *rv = NULL;

    fscanf(m_file,"%d %s", &type, tmp);

    printf("create shape %d\n", type);

    switch(type)
    {
        case SHAPE::LINE_CHAIN:
            rv = new SHAPE_LINE_CHAIN;
            break;
    }

    if(!rv)
        return NULL;

    rv.Parse ( )

    fprintf(m_file,"shape %d %s %s\n", aShape->Type(), aName.c_str(), sh.c_str() );
*/
    assert( false );
    return NULL;
}


void SHAPE_FILE_IO::BeginGroup( const std::string& aName )
{
    assert( m_mode != IOM_READ );

    if( !m_file )
        return;

    fprintf( m_file, "group %s\n", aName.c_str() );
    m_groupActive = true;
}


void SHAPE_FILE_IO::EndGroup()
{
    assert( m_mode != IOM_READ );

    if( !m_file || !m_groupActive )
        return;

    fprintf( m_file, "endgroup\n" );
    m_groupActive = false;
}


void SHAPE_FILE_IO::Write( const SHAPE* aShape, const std::string& aName )
{
    assert( m_mode != IOM_READ );

    if( !m_file )
        return;

    if( !m_groupActive )
        fprintf( m_file,"group default\n" );

    std::string sh = aShape->Format();

    fprintf( m_file, "shape %d %s %s\n", aShape->Type(), aName.c_str(), sh.c_str() );
    fflush( m_file );
}
