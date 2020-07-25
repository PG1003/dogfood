-- Copyright (c) 2019 PG1003
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to
-- deal in the Software without restriction, including without limitation the
-- rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
-- sell copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:

-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.

-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.


local help = [[dogfood v1.0.1

A tool  for creating self running Lua programs.

Usage: dogfood [OPTIONS] OUT MODULE...

Creates a program at location OUT that runs its embedded Lua MODULEs.
The first MODULE from the list is the entry point of the program while
the optional extra modules are accessable by require. The modules are
provided without the '.lua' extention.
C modules cannot be embedded in the resulting program but the program can
load C modules when they are placed in one of the package.cpath search paths.

The following OPTIONS are available;
-c, --compile                   Compile the modules and embed them as bytecode.
-s, --strip-debug-information   Strips the debug information from the bytecode.
-h, --help                      Show this help.
-m                              Adds the given path to package.loaded to search
                                  for modules that are provided as parameter.
-v, --lua-version               Show the Lua language version of the
                                  interpreter used by the resulting program.]]


local function dogfood_error( msg )
    io.stderr:write( "Dogfood error: " .. msg .. "\n" )
    os.exit( 1 )
end

--
-- Parse and validate the commandline parameters
--

local compile                 = false
local strip_debug_information = false
local out                     = false
local modules                 = {}
        
do
    if #arg == 0 then
        dogfood_error( "Use '-h' or '--help' as parameters for information about the usage of dogfood." )
    end

    local displayed_help        = false
    local displayed_lua_version = false

    local option = false
    for _, param in ipairs( arg ) do
        if option then
            if option == "-m" then
                local path   = param:gsub( "\"", "" )
                package.path = package.path .. ";" .. path
            end
            option = false
        elseif param == "-h" or param == "--help" then
            print( help )
            displayed_help = true
        elseif param == "-c" or param == "--compile" then
            compile = true
        elseif param == "-s" or param == "--strip-debug-information" then
            strip_debug_information = true
        elseif param == "-v" or param == "--lua-version" then
            print( _VERSION )
            displayed_lua_version = true
        elseif param == "-m" then
            option = param
        else
            if not out then
                out = param:gsub( "\"", "" )
            else
                modules[ #modules + 1 ] = { name = param }
            end
        end
    end

    -- '-h' and '-v' options display only information and do not require the OUT and MODULE parameters
    if ( displayed_help or displayed_lua_version ) and not out and #modules == 0 then
        os.exit( 0 )
    end

    if not out then
        dogfood_error( "No output file provided." )
    end

    if #modules == 0 then
        dogfood_error( "No modules provided." )
    end
end

--
-- Validate modules
--

do
    local found = {}
    for _, mod in ipairs( modules ) do
        if not found[ mod.name ] then
            local path = package.searchpath( mod.name, package.path )
            if path then
                mod.path = path
            else
                dogfood_error( "Cannot find module '" .. mod.name .. "'." )
            end
            found[ mod.name ] = true
        end
    end
end

--
-- Open the output file to write the executable data to
--

local output = io.open( out, "w+b" )
if not output then
    dogfood_error( "Cannot create output file '" .. out .. "'." )
end

local payload_end = false

--
-- Copy the 'dog' part of this dogfood executable to the new executable
--

do
    local dogfood = io.open( arg[ 0 ], "rb" )
    if not dogfood then
        dogfood_error( "Cannot read own dogfood executable." )
    end

    if not dogfood:seek( "end", -23 ) then
        dogfood_error( "Cannot seek to end of the dogfood executable." )
    end

    payload_end = dogfood:read( 23 )
    if not payload_end then
        dogfood_error( "No end-of-payload marker found." )
    end

    local dog_bytes = payload_end:match( "%s-- dogfood (%x%x%x%x%x%x%x%x)%s" )
    if not dog_bytes then
        dogfood_error( "Invalid end-of-payload marker found." )
    end

    dog_bytes = assert( tonumber( dog_bytes, 16 ) )
    if dog_bytes >= ( dogfood:seek() - 23 ) then
        dogfood_error( "Invalid end-of-payload marker value." )
    end

    dogfood:seek( "set", 0 )
    repeat
        local count = dog_bytes > 4096 and 4096 or dog_bytes
        local chunk = assert( dogfood:read( count ) )
        output:write( chunk )
        dog_bytes = dog_bytes - count
    until dog_bytes <= 0
    dogfood:close()
end

--
-- Append payload
--

do
    for _, mod in ipairs( modules ) do
        -- Validate if the module has syntactic errors by loading the file
        local mod_chunk, error_msg = loadfile( mod.path, "t" );
        if mod_chunk == nil then
            dogfood_error( "Error while loading module.\n" .. error_msg )
        end
        
        local mod_data
        if compile then
            mod_data = assert( string.dump( mod_chunk, strip_debug_information ) )
        else
            -- Work around for 'read all' which has changed with 5.3
            local file = assert( io.open( mod.path, "rb" ) )
            file:seek( "end", 0 )
            local file_size = file:seek( "cur", 0 )
            file:seek( "set", 0 )
            mod_data = assert( file:read( file_size ) )
            file:close()
        end
        output:write( string.format( "\r\n-- %s %08X\r\n", mod.name, #mod_data ) )
        output:write( mod_data )
    end
    
    output:write( payload_end )
    output:close()
end

return 0
