# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded mpg123 1.0 \
	    [list load [file join $dir libtcl9mpg1231.0.so] [string totitle mpg123]]
} else {
    package ifneeded mpg123 1.0 \
	    [list load [file join $dir libmpg1231.0.so] [string totitle mpg123]]
}
