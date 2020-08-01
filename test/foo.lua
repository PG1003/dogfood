local bar    = require( "bar" )
local baz    = require( "bar.baz" )
local pg1003 = require( "pg1003" )

-- Test if we get the expected arguments
assert( string.match( arg[ 0 ], "/foobar$" ) )
assert( arg[ 1 ] == "-param" )

-- Call functions from other modules
assert( bar.bar() == "bar" )
assert( baz.baz() == "baz" )
assert( pg1003.pg1003() == "PG1003" )

-- Retun success
return 0
