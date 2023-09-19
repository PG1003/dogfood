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
#include <stdarg.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#if defined( _WIN32 ) && defined( EMBED_LUA_DLL )
    #define LOAD_EMBEDDED_LUA_DLL

    #include "resource.h"
    #include <windows.h>
    #include <tchar.h>
    #include <assert.h>
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX_NAME_LENGTH     127
#define MAX_NAME_LENGTH_STR STR( MAX_NAME_LENGTH )


#if LUA_VERSION_NUM < 502 || LUA_VERSION_NUM > 504
    #error "Unsupported Lua version"
#endif

#if defined( LOAD_EMBEDDED_LUA_DLL )

/* This function loads Lua DLL from its resource.
 * Embedding the Lua DLL simplifies the distribution of dogfood and its derived executables.
 *
 * This feature is specific for Windows and is enabled by a EMBED_LUA_DLL preprocessor define.
 *
 * The function requires that:
 * 1) The data is a valid Lua DLL that targets the same platform as dogfood's executable.
 * 2) The DLL is delay loaded.
 * 3) The ID for DLL in the resource is named IRD_LUADLL.
 * 4) The resource type of the Lua DLL is RCDATA.
 *
 * The resource file should contain a line that look like the following;
 *
 *    IDR_LUADLL    RCDATA    "PATH_TO_YOUR\Lua.dll"
 *
 * Its possible to link dogfood static with Lua but then support for C mudules will be effectively
 * disabled. C modules will likely require a Lua DLL that conflicts with the statically linked Lua
 * instance in dogfood. In this case a 'multiple Lua VMs detected' message will be displayed in
 * the console output when you run the resulting executable.
 */
static void load_lua_dll_from_resouce()
{
    /* https://stackoverflow.com/questions/17774103/using-an-embedded-dll-in-an-executable
     * https://learn.microsoft.com/en-us/cpp/windows/how-to-include-resources-at-compile-time */

#pragma warning( push, 3 )
    const HRSRC resource_handle = FindResource( NULL,
                                                MAKEINTRESOURCE( IDR_LUADLL ),
                                                RT_RCDATA );
    assert( resource_handle );
#pragma warning( pop )

    const HGLOBAL file_resouce_handle = LoadResource( NULL, resource_handle );
    assert( file_resouce_handle );

    const LPVOID file_at_resouce = LockResource( file_resouce_handle );
    assert( file_at_resouce );

    const DWORD  file_size = SizeofResource( NULL, resource_handle );

    const TCHAR  lua_dll_name[] = _T("Lua" LUA_VERSION_MAJOR LUA_VERSION_MINOR ".dll");
    const HANDLE file_handle    = CreateFile( lua_dll_name,
                                              GENERIC_READ | GENERIC_WRITE,
                                              0,
                                              NULL,
                                              CREATE_ALWAYS,
                                              FILE_ATTRIBUTE_NORMAL,
                                              NULL );
    assert( file_handle );

    const HANDLE file_mapping_handle = CreateFileMapping( file_handle,
                                                          NULL,
                                                          PAGE_READWRITE,
                                                          0,
                                                          file_size,
                                                          NULL );
    assert( file_mapping_handle );

    const LPVOID mapped_file_address = MapViewOfFile( file_mapping_handle,
                                                      FILE_MAP_WRITE,
                                                      0, 0, 0 );
    assert( mapped_file_address );

    CopyMemory( mapped_file_address, file_at_resouce, file_size );

    UnmapViewOfFile( mapped_file_address );
    CloseHandle( file_mapping_handle );
    CloseHandle( file_handle );
}

#endif

static void dogfood_error( lua_State * L, const char * const format, ... )
{
    va_list args;
    va_start( args, format );

    fputs( "Dogfood error: ", stderr );
    vfprintf( stderr, format, args );
    fputc( '\n', stderr );

    lua_close( L );
    exit( 1 );
}

static void dogfood_errno( lua_State * L )
{
    perror( "Dogfood error" );
    lua_close( L );
    exit( 1 );
}

static void load_module( lua_State * L,
                         char * const buffer,
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
            dogfood_error( L,
                           extra ? "Error while loading module '%s';\n%s" :
                                   "An error has occurred while loading module '%s'.",
                           module_name, extra  );
        }
    }
}

int main( int argc, char *argv[] )
{
#if defined( LOAD_EMBEDDED_LUA_DLL )
    load_lua_dll_from_resouce();
#endif

    lua_State * const L = luaL_newstate();
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
        dogfood_errno( L );
    }

    const long payload_end_marker_length = sizeof( "\r\n-- dogfood DEADBEEF\r\n" ) - 1;
    if( fseek( f, -payload_end_marker_length, SEEK_END ) )
    {
        dogfood_errno( L );
    }

    const long end_of_payload = ftell( f );
    if( end_of_payload == -1L )
    {
        dogfood_errno( L );
    }

    unsigned long payload_offset = 0;
    if( fscanf( f, "\r\n-- dogfood %08lX\r\n", &payload_offset ) != 1 &&
        payload_offset == 0 )
    {
        dogfood_error( L, "No valid payload end marker found." );
    }

    if( fseek( f, ( long )payload_offset, SEEK_SET ) )
    {
        dogfood_errno( L );
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
        dogfood_error( L, "No valid start of payload found." );
    }

    const long main_module_pos = ftell( f );
    if( !( main_module_pos != -1L &&
           fseek( f, ( long )main_module_size, SEEK_CUR ) == 0 ) )
    {
        dogfood_errno( L );
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
            dogfood_error( L, "No valid start of module '%s' found.", module_name );
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
            dogfood_errno( L );
        }

        if( read_count != module_size )
        {
            dogfood_error( L, "Cannot read the entire module '%s'.", module_name );
        }

        load_module( L, buffer, module_size, module_name );

        lua_call( L, 0, 1 );
        lua_setfield( L, -2, module_name );

        pos = ftell( f );
    }

    if( ferror( f ) )
    {
        dogfood_errno( L );
    }

    /* Load and execute the main module (byte)code */
    size_t read_count = 0;
    if( fseek( f, main_module_pos, SEEK_SET ) ||
        ( read_count = fread( buffer, sizeof( char ), main_module_size, f ),
          ferror( f ) ) ||
        fclose( f ) )
    {
        dogfood_errno( L );
    }

    if( read_count != main_module_size )
    {
        dogfood_error( L, "Cannot read the entire main module." );
    }

    load_module( L, buffer, main_module_size, main_module_name );

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
