local bar = require( "bar" )

-- Test if we get the expected arguments
assert( string.match( arg[ 0 ], "/foobar$" ) )
assert( arg[ 1 ] == "-param" )

-- Call a function from another module
assert( bar.bar() == "bar" )

-- Retun success
return 0
