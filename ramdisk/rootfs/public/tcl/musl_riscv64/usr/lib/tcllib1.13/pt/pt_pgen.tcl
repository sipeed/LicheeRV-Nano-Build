# -*- tcl -*-
#
# Copyright (c) 2009 by Andreas Kupries <andreas_kupries@users.sourceforge.net>
# Grammars / Parsing Expression Grammars / Parser Generator

# ### ### ### ######### ######### #########
## Package description

# A package exporting a parser generator command.

# ### ### ### ######### ######### #########
## Requisites

package require Tcl 8.5
package require fileutil
package require pt::peg::from::json    ; # Frontends: json, and PEG text form
package require pt::peg::from::peg     ; #
package require pt::peg::to::container ; # Backends: json, peg, container code,
package require pt::peg::to::json      ; #           param assembler, 
package require pt::peg::to::peg       ; #
package require pt::peg::to::param     ; # PARAM assembly, raw
package require pt::peg::to::tclparam  ; # PARAM assembly, embedded into Tcl
package require pt::peg::to::cparam    ; # PARAM assembly, embedded into C
package require pt::tclparam::configuration::snit  ; # PARAM/Tcl, snit::type
package require pt::tclparam::configuration::tcloo ; # PARAM/Tcl, TclOO class
package require pt::cparam::configuration::critcl  ; # PARAM/C, in critcl

# ### ### ### ######### ######### #########
## Implementation

namespace eval ::pt::pgen {
    namespace export json peg serial
    namespace ensemble create
}

# # ## ### ##### ######## #############
## Public API - Processing the input.

proc ::pt::pgen::serial {input args} {
    #lappend args -file $inputfile
    return [Write {*}$args $input]
}

proc ::pt::pgen::json {input args} {
    #lappend args -file $inputfile
    return [Write {*}$args [pt::peg::from::json convert $input]]
}

proc ::pt::pgen::peg {input args} {
    #lappend args -file $inputfile
    return [Write {*}$args [pt::peg::from::peg convert $input]]
}

# # ## ### ##### ######## #############
## Internals - Generating the parser.

namespace eval ::pt::pgen::Write {
    namespace export json peg container param snit oo critcl c
    namespace ensemble create
}

proc ::pt::pgen::Write::json {args} {
    # args = (option value)... grammar
    pt::peg::to::json configure {*}[lrange $args 0 end-1]
    return [pt::peg::to::json convert [lindex $args end]]
}

proc ::pt::pgen::Write::peg {args} {
    # args = (option value)... grammar
    pt::peg::to::peg configure {*}[lrange $args 0 end-1]
    return [pt::peg::to::peg convert [lindex $args end]]
}

proc ::pt::pgen::Write::container {args} {
    # args = (option value)... grammar
    pt::peg::to::container configure {*}[lrange $args 0 end-1]
    return [pt::peg::to::container convert [lindex $args end]]
}

proc ::pt::pgen::Write::param {args} {
    # args = (option value)... grammar
    pt::peg::to::param configure {*}[lrange $args 0 end-1]
    return [pt::peg::to::param convert [lindex $args end]]
}

proc ::pt::pgen::Write::snit {args} {
    # args = (option value)... grammar
    pt::peg::to::tclparam configure {*}[Package [Class [lrange $args 0 end-1]]]

    pt::tclparam::configuration::snit def \
	$class $package \
	{pt::peg::to::tclparam configure}

    return [pt::peg::to::tclparam convert [lindex $args end]]
}

proc ::pt::pgen::Write::oo {args} {
    # args = (option value)... grammar
    pt::peg::to::tclparam configure {*}[Package [Class [lrange $args 0 end-1]]]

    pt::tclparam::configuration::tcloo def \
	$class $package \
	{pt::peg::to::tclparam configure}

    return [pt::peg::to::tclparam convert [lindex $args end]]
}

proc ::pt::pgen::Write::critcl {args} {
    # args = (option value)... grammar
    # Class   -> touches/defines variable 'class'
    # Package -> touches/defines variable 'package'
    pt::peg::to::cparam configure {*}[Package [Class [lrange $args 0 end-1]]]

    pt::cparam::configuration::critcl def \
	$class $package \
	{pt::peg::to::cparam configure}

    return [pt::peg::to::cparam convert [lindex $args end]]
}

proc ::pt::pgen::Write::c {args} {
    # args = (option value)... grammar
    pt::peg::to::cparam configure {*}[lrange $args 0 end-1]
    return [pt::peg::to::cparam convert [lindex $args end]]
}

# ### ### ### ######### ######### #########
## Internals: Special option handling handling.

proc ::pt::pgen::Write::Class {optiondict} {
    upvar 1 class class
    set class CLASS
    set res {}
    foreach {option value} $optiondict {
	if {$option eq "-class"} {
	    set class $value
	    continue
	}
	lappend res $option $value
    }
    return $res
}

proc ::pt::pgen::Write::Package {optiondict} {
    upvar 1 package package
    set package PACKAGE
    set res {}
    foreach {option value} $optiondict {
	if {$option eq "-package"} {
	    set package $value
	    continue
	}
	lappend res $option $value
    }
    return $res
}

# ### ### ### ######### ######### #########
## Package Management

package provide pt::pgen 1
