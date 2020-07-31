// Copyright (c) 2019 PG1003
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX_NAME_LENGTH     127
#define MAX_NAME_LENGTH_STR STR( MAX_NAME_LENGTH )


#if LUA_VERSION_NUM < 502 || LUA_VERSION_NUM > 504
    #error "Unsupported Lua version"
#endif

static lua_State * L = 0;


static void dogfood_error( const char * const message )
{
    fprintf( stderr, "Dogfood error: %s\n", message );
    lua_close( L );
    exit( 1 );
}

static void dogfood_error_extra( const char * const message, const char * const extra )
{
    fprintf( stderr, "Dogfood error: %s\n", message );
    fputs( extra, stderr );
    lua_close( L );
    exit( 1 );
}

static void dogfood_errno()
{
    perror( "Dogfood error" );
    lua_close( L );
    exit( 1 );
}

static void load_module( char * const buffer,
                         size_t module_size,
                         const char * const module_name )
{
    const int result = luaL_loadbuffer( L, buffer, module_size, module_name );
    switch( result )
    {
        case LUA_OK:
        case LUA_YIELD:
            break;

        case LUA_ERRSYNTAX:
        case LUA_ERRMEM:
#if LUA_VERSION_NUM < 504
        case LUA_ERRGCMM:
#endif
        case LUA_ERRERR:
        {
            const char * const extra = lua_tostring( L, -1 );
            dogfood_error_extra( "Error while loading module",
                                 extra ? extra : "An error has occurred."  );
        }
    }
}

int main( int argc, char *argv[] )
{
    L = luaL_newstate();
    luaL_openlibs( L );

    /* Create and fill argument table */
    lua_createtable( L, argc, 0 );
    int i = 0;
    for( ; i < argc ; ++i )
    {
        lua_pushstring( L, argv[ i ] );
        lua_rawseti( L, -2, i );
    }
    lua_setglobal( L, "arg" );

    /* Open the executable to read the modules that are appended the file */
    FILE * const f = fopen( argv[ 0 ], "rb" );
    if( !f )
    {
        dogfood_errno();
    }

    const long payload_end_marker_length = sizeof( "\r\n-- dogfood DEADBEEF\r\n" ) - 1;
    if( fseek( f, -payload_end_marker_length, SEEK_END ) )
    {
        dogfood_errno();
    }

    const long end_of_payload = ftell( f );
    if( end_of_payload == -1L )
    {
        dogfood_errno();
    }

    unsigned long payload_offset = 0;
    if( fscanf( f, "\r\n-- dogfood %08lX\r\n", &payload_offset ) != 1 &&
        payload_offset == 0 )
    {
        dogfood_error( "No valid payload end marker found." );
    }

    if( fseek( f, ( long )payload_offset, SEEK_SET ) )
    {
        dogfood_errno();
    }

    const char module_header[] = "\r\n-- %" MAX_NAME_LENGTH_STR "s %08lX%c%c";

    /* The main module is the first module in the payload */
    char          main_module_name[ MAX_NAME_LENGTH + 1 ] = { 0 };
    unsigned long main_module_size                        = 0;
    char          carriage_return                         = 0;
    char          new_line                                = 0;
    if( fscanf( f, module_header, main_module_name, &main_module_size,
        &carriage_return, &new_line ) != 4 ||
        main_module_size == 0 ||
        carriage_return != '\r' ||
        new_line != '\n' )
    {
        dogfood_error( "No valid start of payload found." );
    }

    const long main_module_pos = ftell( f );
    if( !( main_module_pos != -1L &&
           fseek( f, ( long )main_module_size, SEEK_CUR ) == 0 ) )
    {
        dogfood_errno();
    }

    unsigned long buffer_size = main_module_size;
    char *buffer              = malloc( buffer_size );

    char module_name[ MAX_NAME_LENGTH + 1 ] = { 0 };

    /* Load modules (byte)code and add it to the package.loaded table */
    lua_getglobal( L, "package" );
    lua_getfield( L, -1, "loaded" );

    long pos = ftell( f );
    while( ( pos >= 0 ) && ( pos < end_of_payload ) )
    {
        unsigned long module_size = 0;
        if( fscanf( f, module_header, module_name, &module_size,
            &carriage_return , &new_line ) != 4 ||
            carriage_return != '\r' ||
            new_line != '\n' )
        {
            dogfood_error( "No valid start of module found." );
        }

        if( buffer_size < module_size )
        {
            free( buffer );
            buffer      = malloc( module_size );
            buffer_size = module_size;
        }

        const size_t read_count = fread( buffer, sizeof( char ), module_size, f );
        if( ferror( f ) )
        {
            dogfood_errno();
        }

        if( read_count != module_size )
        {
            dogfood_error( "Cannot read the entire module." );
        }

        load_module( buffer, module_size, module_name );

        lua_call( L, 0, 1 );
        lua_setfield( L, -2, module_name );

        pos = ftell( f );
    }

    if( ferror( f ) )
    {
        dogfood_errno();
    }

    /* Load and execute the main module (byte)code */
    size_t read_count = 0;
    if( fseek( f, main_module_pos, SEEK_SET ) ||
        ( read_count = fread( buffer, sizeof( char ), main_module_size, f ),
          ferror( f ) ) ||
        fclose( f ) )
    {
        dogfood_errno();
    }

    if( read_count != main_module_size )
    {
        dogfood_error( "Cannot read the entire main module." );
    }

    load_module( buffer, main_module_size, main_module_name );

    free( buffer ); /* Free it so the memory can be reused by Lua */

    const int status = lua_pcall( L, 0, 1, 0 );

    if( status != LUA_OK )
    {
        /* Show Lua errors.
         * Do not prepend 'Dogfood error:' to the error message. The error has nothing
         * to do with dogfood since it is emited by the Lua VM or the Lua modules. */
        const char * const msg = lua_tostring( L, -1 );
        fprintf( stderr, "%s\n", msg ? msg : "An error has occurred." );
    }

    /* Get the exit status if the return value is a number */
    const int exit_status = ( int )( lua_isnumber( L, 0 ) ? lua_tonumber( L, 0 ) : 0.0 );

    lua_close( L );

    /* Return exit status when OK, else default to 1 when there was an error */
    return status == LUA_OK ? exit_status : 1;
}
