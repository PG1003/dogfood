local bar = require( "bar" )
local baz = require( "bar.baz" )

-- Test if we get the expected arguments
assert( string.match( arg[ 0 ], "/foobar$" ) )
assert( arg[ 1 ] == "-param" )

-- Call functions from other modules
assert( bar.bar() == "bar" )
assert( baz.baz() == "baz" )

-- Retun success
return 0
