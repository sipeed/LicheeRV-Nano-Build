# Index file to load the TDBC package.

# Make sure that TDBC is running in a compatible version of Tcl, and
# that TclOO is available.

if {[catch {package present Tcl 8.5}]} {
    return
}
apply {{dir} {
    set libraryfile [file join $dir tdbc.tcl]
    if {![file exists $libraryfile] && [info exists ::env(TDBC_LIBRARY)]} {
	set libraryfile [file join $::env(TDBC_LIBRARY) tdbc.tcl]
    }
    package ifneeded tdbc 1.1.0 \
	"package require TclOO 0.6-;\
	[list load [file join $dir libtdbc1.1.0.so] tdbc]\;\
	[list source $libraryfile]"
}} $dir
