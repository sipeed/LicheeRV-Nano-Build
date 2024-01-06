## -- Tcl Module -- -*- tcl -*-
# # ## ### ##### ######## #############

# @@ Meta Begin
# Package coroutine::auto 1
# Meta platform        tcl
# Meta require         {Tcl 8.6}
# Meta require         coroutine
# Meta license         BSD
# Meta as::author      {Andreas Kupries}
# Meta summary         Coroutine Event and Channel Support
# Meta description     Built on top of coroutine, this
# Meta description     package intercepts various builtin
# Meta description     commands to make the code using them
# Meta description     coroutine-oblivious, i.e. able to run
# Meta description     inside and outside of a coroutine
# Meta description     without changes.
# @@ Meta End

# Copyright (c) 2009 Andreas Kupries

## $Id: coro_auto.tcl,v 1.1 2009/11/10 21:04:39 andreas_kupries Exp $
# # ## ### ##### ######## #############
## Requisites, and ensemble setup.

package require Tcl 8.6
package require coroutine

namespace eval ::coroutine::auto {}

# # ## ### ##### ######## #############
## Internal. Setup.

proc ::coroutine::auto::Init {} {

    # Replaces the builtin commands with coroutine-aware
    # counterparts. We cannot use the coroutine commands
    # directly, because the replacements have to use the saved builtin
    # commands when called outside of a coroutine. And some (read,
    # gets, update) even need full re-implementations, as they use the
    # builtin command they replace themselves to implement their
    # functionality.

    foreach cmd {
	global
	exit
	after
	vwait
	update
    } {
	rename ::$cmd [namespace current]::core_$cmd
	rename [namespace current]::wrap_$cmd ::$cmd
    }

    foreach cmd {
	gets
	read
    } {
	rename ::tcl::chan::$cmd [namespace current]::core_$cmd
	rename [namespace current]::wrap_$cmd ::tcl::chan::$cmd
    }

    return
}

# # ## ### ##### ######## #############
## API implementations. Uses the coroutine commands where
## possible.

proc ::coroutine::auto::wrap_global {args} {
    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_global {*}$args
    }

    tailcall ::coroutine::global {*}$args
}

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_after {delay args} {
    if {
	([info coroutine] eq {}) ||
	([llength $args] > 0)
    } {
	# We use the core builtin when called from either outside of a
	# coroutine, or for an asynchronous delay.

	tailcall [namespace current]::core_after $delay {*}$args
    }

    # Inside of coroutine, and synchronous delay (args == "").
    tailcall ::coroutine::after $delay
}

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_exit {{status 0}} {
    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_exit $status
    }

    tailcall ::coroutine::exit $status
}

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_vwait {varname} {
    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_vwait $varname
    }

    tailcall ::coroutine::vwait $varname
}

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_update {{what {}}} {
    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_update {*}$what
    }

    # This is a full re-implementation of mode (1), because the
    # coroutine-aware part uses the builtin itself for some
    # functionality, and this part cannot be taken as is.

    if {$what eq "idletasks"} {
        after idle [info coroutine]
    } elseif {$what ne {}} {
        # Force proper error message for bad call.
        tailcall [namespace current]::core_update $what
    } else {
        after 0 [info coroutine]
    }
    yield
    return
} 

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_gets {args} {
    # Process arguments.
    # Acceptable syntax:
    # * gets CHAN ?VARNAME?

    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_gets {*}$args
    }

    # This is a full re-implementation of mode (1), because the
    # coroutine-aware part uses the builtin itself for some
    # functionality, and this part cannot be taken as is.

    if {[llength $args] > 2} {
	# Calling the builtin gets command with the bogus arguments
	# gives us the necessary error with the proper message.
	tailcall [namespace current]::core_gets {*}$args
    } elseif {[llength $args] == 2} {
	lassign $args chan varname
        upvar 1 $varname line
    } else {
	# llength args == 1
	lassign $args chan
    }

    # Loop until we have a complete line. Yield to the event loop
    # where necessary. During 

    while {1} {
        set blocking [::chan configure $chan -blocking]
        ::chan configure $chan -blocking 0

	try {
	    [namespace current]::core_gets $chan line
	} on error {result opts} {
            ::chan configure $chan -blocking $blocking
            return -code $result -options $opts
	}

	if {[::chan blocked $chan]} {
            ::chan event $chan readable [list [info coroutine]]
            yield
            ::chan event $chan readable {}
        } else {
            ::chan configure $chan -blocking $blocking

            if {[llength $args] == 2} {
                return $result
            } else {
                return $line
            }
        }
    }
}

# - -- --- ----- -------- -------------

proc ::coroutine::auto::wrap_read {args} {
    # Process arguments.
    # Acceptable syntax:
    # * read ?-nonewline ? CHAN
    # * read               CHAN ?n?

    if {[info coroutine] eq {}} {
	tailcall [namespace current]::core_read {*}$args
    }

    # This is a full re-implementation of mode (1), because the
    # coroutine-aware part uses the builtin itself for some
    # functionality, and this part cannot be taken as is.

    if {[llength $args] > 2} {
	# Calling the builtin read command with the bogus arguments
	# gives us the necessary error with the proper message.
	[namespace current]::core_read {*}$args
	return
    }

    set total Inf ; # Number of characters to read. Here: Until eof.
    set chop  no  ; # Boolean flag. Determines if we have to trim a
    #               # \n from the end of the read string.

    if {[llength $args] == 2} {
	lassign $args a b
	if {$a eq "-nonewline"} {
	    set chan $b
	    set chop yes
	} else {
	    lassign $args chan total
	}
    } else {
	lassign $args chan
    }

    # Run the read loop. Yield to the event loop where
    # necessary. Differentiate between loop until eof, and loop until
    # n characters have been read (or eof reached).

    set buf {}

    if {$total eq "Inf"} {
	# Loop until eof.

	while {1} {
	    set blocking [::chan configure $chan -blocking]
	    ::chan configure $chan -blocking 0

	    try {
		[namespace current]::core_read $chan
	    } on error {result opts} {
		::chan configure $chan -blocking $blocking
		return -code $result -options $opts
	    }

	    if {[fblocked $chan]} {
		::chan event $chan readable [list [info coroutine]]
		yield
		::chan event $chan readable {}
	    } else {
		::chan configure $chan -blocking $blocking
		append buf $result

		if {[::chan eof $chan]} {
		    ::chan close $chan
		    break
		}
	    }
	}
    } else {
	# Loop until total characters have been read, or eof found,
	# whichever is first.

	set left $total
	while {1} {
	    set blocking [::chan configure $chan -blocking]
	    ::chan configure $chan -blocking 0

	    try {
		[namespace current]::core_read $chan $left
	    } on error {result opts} {
		::chan configure $chan -blocking $blocking
		return -code $result -options $opts
	    }

	    if {[::chan blocked $chan]} {
		::chan event $chan readable [list [info coroutine]]
		yield
		::chan event $chan readable {}
	    } else {
		::chan configure $chan -blocking $blocking
		append buf $result
		incr   left -[string length $result]

		if {[::chan eof $chan]} {
		    ::chan close $chan
		    break
		} elseif {!$left} {
		    break
		}
	    }
	}
    }

    if {$chop && [string index $buf end] eq "\n"} {
	set buf [string range $buf 0 end-1]
    }

    return $buf
}

# # ## ### ##### ######## #############
## Ready
::coroutine::auto::Init
package provide coroutine::auto 1
return
