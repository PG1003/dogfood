local bar = require( "bar" )

assert( string.match( arg[ 0 ], "/foobar$" ) )
assert( arg[ 1 ] == "-param" )

print( "foo" )
bar.do_bar()
