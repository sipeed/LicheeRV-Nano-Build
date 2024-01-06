# -*- tcl -*-
#
# Copyright (c) 2009 by Andreas Kupries <andreas_kupries@users.sourceforge.net>

# # ## ### ##### ######## ############# #####################
## Package description

## Implementation of the PackRat Machine (PARAM), a virtual machine on
## top of which parsers for Parsing Expression Grammars (PEGs) can be
## realized. This implementation is tied to Tcl for control flow. We
## (will) have alternate implementations written in TclOO, and critcl,
## all exporting the same API.
#
## RD stands for Recursive Descent.

# # ## ### ##### ######## ############# #####################
## Requisites

package require Tcl 8.5
package require TclOO
package require struct::stack 1.4 ; # Requiring get, trim methods
package require pt::ast
package require pt::pe

# # ## ### ##### ######## ############# #####################
## Implementation

oo::class create ::pt::rde::oo {

    # # ## ### ##### ######## ############# #####################
    ## API - Lifecycle

    constructor {} {
	set selfns [info object namespace]

	set mystackloc  [struct::stack ${selfns}::LOC]  ; # LS
	set mystackerr  [struct::stack ${selfns}::ERR]  ; # ES
	set mystackast  [struct::stack ${selfns}::AST]  ; # ARS/AS
	set mystackmark [struct::stack ${selfns}::MARK] ; # s.a.

	my reset
	return
    }

    method reset {chan} {
	set mychan    $chan      ; # IN
	set myline    1          ; #
	set mycolumn  0          ; #
	set mycurrent {}         ; # CC
	set myloc     -1         ; # CL
	set myok      0          ; # ST
	set msvalue   {}         ; # SV
	set myerror   {}         ; # ER
	set mytoken   {}         ; # TC
	array unset   mysymbol * ; # NC

	$mystackloc  clear
	$mystackerr  clear
	$mystackast  clear
	$mystackmark clear
	return
    }

    method complete {} {
	if {$myok} {
	    set n [$mystackast size]
	    if {$n > 1} {
		set  pos [$mystackloc peek]
		incr pos
		set children [lreverse [$mystackast peek [$mystackast size]]]     ; # SaveToMark
		return [pt::ast new {} $pos $myloc {*}$children] ; # Reduce ALL
	    } else {
		return [$mystackast peek]
	    }
	} else {
	    lassign $myerror loc messages
	    return -code error [list pt::rde $loc [$self position $loc] $messages]
	}
    }

    # # ## ### ##### ######## ############# #####################
    ## API - State accessors

    method chan   {} { return $mychan }
    method line   {} { return $myline }
    method column {} { return $mycolumn }

    # - - -- --- ----- --------

    method current  {} { return $mycurrent }
    method location {} { return $myloc }
    method lmarked  {} { return [lreverse [$mystackloc get]] }

    # - - -- --- ----- --------

    method ok      {} { return $myok      }
    method value   {} { return $mysvalue  }
    method error   {} { return $myerror   }
    method emarked {} { return [lreverse [$mystackerr get]] }

    # - - -- --- ----- --------

    method tokens {{from {}} {to {}}} {
	switch -exact [llength [info level 0]] {
	    4 { return $mytoken }
	    5 { return [lrange $mytoken $from $from] }
	    6 { return [lrange $mytoken $from $to] }
	}
    }

    method symbols {} {
	return [array get mysymbol]
    }

    method scached {} {
	return [array names mysymbol]
    }

    # - - -- --- ----- --------

    method asts    {} { return [lreverse [$mystackast  get]] }
    method amarked {} { return [lreverse [$mystackmark get]] }
    method ast     {} { return [$mystackast peek] }

    # - - -- --- ----- --------

    method position {loc} {
	return [lrange [lindex $mytoken $loc] 1 2]
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Control flow

    method i:ok_continue {} {
	if {!$myok} return
	return -code continue
    }

    method i:fail_continue {} {
	if {$myok} return
	return -code continue
    }

    method i:fail_return {} {
	if {$myok} return
	return -code return
    }

    method i:ok_return {} {
	if {!$myok} return
	return -code return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Unconditional matching.

    method i_status_ok {} {
	set myok 1
	return
    }

    method i_status_fail {} {
	set myok 0
	return
    }

    method i_status_negate {} {
	set myok [expr {!$myok}]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Error handling.

    method i_error_clear {} {
	set myerror {}
	return
    }

    method i_error_push {} {
	$mystackerr push $myerror
	return
    }

    method i_error_pop_merge {} {
	set olderror [$mystackerr pop]

	# We have either old or new error data, keep it.

	if {![llength $myerror]}  { set myerror $olderror ; return }
	if {![llength $olderror]} return

	# If one of the errors is further on in the input choose that as
	# the information to propagate.

	lassign $myerror  loe msgse
	lassign $olderror lon msgsn

	if {$lon > $loe} { set myerror $olderror ; return }
	if {$loe > $lon} return

	# Equal locations, merge the message lists.
	#set myerror [list $loe [struct::set union $msgse $msgsn]]
	set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	return
    }

    method i_error_nonterminal {symbol} {
	# Inlined: Errors, Expected.
	if {![llength $myerror]} return
	set pos [$mystackloc peek]
	incr pos
	lassign $myerror loc messages
	if {$loc != $pos} return
	set myerror [list $loc [list $symbol]]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Basic input handling and tracking

    method i_loc_pop_rewind/discard {} {
	#$myparser i:fail_loc_pop_rewind
	#$myparser i:ok_loc_pop_discard
	#return
	set last [$mystackloc pop]
	if {!$myok} {
	    set myloc $last
	}
	return
    }

    method i_loc_pop_discard {} {
	$mystackloc pop
	return
    }

    method i_loc_pop_rewind {} {
	set myloc [$mystackloc pop]
	return
    }

    method i:fail_loc_pop_rewind {} {
	if {$myok} return
	set myloc [$mystackloc pop]
	return
    }

    method i_loc_push {} {
	$mystackloc push $myloc
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - AST stack handling

    method i_ast_pop_rewind/discard {} {
	#$myparser i:fail_ast_pop_rewind
	#$myparser i:ok_ast_pop_discard
	#return
	set mark [$mystackmark pop]
	if {$myok} return
	$mystackast trim $mark
	return
    }

    method i_ast_pop_discard/rewind {} {
	#$myparser i:ok_ast_pop_rewind
	#$myparser i:fail_ast_pop_discard
	#return
	set mark [$mystackmark pop]
	if {!$myok} return
	$mystackast trim $mark
	return
    }

    method i_ast_pop_discard {} {
	$mystackmark pop
	return
    }

    method i_ast_pop_rewind {} {
	$mystackast trim [$mystackmark pop]
	return
    }

    method i:fail_ast_pop_rewind {} {
	if {$myok} return
	$mystackast trim [$mystackmark pop]
	return
    }

    method i_ast_push {} {
	$mystackmark push [$mystackast size]
	return
    }

    method i:ok_ast_value_push {} {
	if {!$myok} return
	$mystackast push $mysvalue
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Nonterminal cache

    method i_symbol_restore {symbol} {
	# Satisfy from cache if possible.
	set k [list $myloc $symbol]
	if {![info exists mysymbol($k)]} { return 0 }
	lassign $mysymbol($k) myloc myok myerror mysvalue
	# We go forward, as the nonterminal matches (or not).
	return 1
    }

    method i_symbol_save {symbol} {
	# Store not only the value, but also how far
	# the match went (if it was a match).
	set at [$mystackloc peek]
	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Semantic values.

    method i_value_clear {} {
	set mysvalue {}
	return
    }

    method i_value_clear/leaf {symbol} {
	# not quite value_lead (guarded, and clear on fail)
	# Inlined clear, reduce, and optimized.
	# Clear ; if {$ok} {Reduce $symbol}
	set mysvalue {}
	if {!$myok} return
	set  pos [$mystackloc peek]
	incr pos
	set mysvalue [pt::ast new $symbol $pos $myloc]
	return
    }

    method i_value_clear/reduce {symbol} {
	set mysvalue {}
	if {!$myok} return

	set  mark [$mystackmark peek];# Old size of stack before current nt pushed more.
	set  newa [expr {[$mystackast size] - $mark}]

	set  pos  [$mystackloc  peek]
	incr pos

	if {!$newa} {
	    set mysvalue {}
	} elseif {$newa == 1} {
	    # peek 1 => single element comes back
	    set mysvalue [list [$mystackast peek]]     ; # SaveToMark
	} else {
	    # peek n > 1 => list of elements comes back
	    set mysvalue [lreverse [$mystackast peek $newa]]     ; # SaveToMark
	}

	set mysvalue [pt::ast new $symbol $pos $myloc {*}$mysvalue] ; # Reduce $symbol
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Terminal matching

    method i_input_next {msg} {
	# Inlined: Getch, Expected, ClearErrors
	# Satisfy from input cache if possible.

	incr myloc
	if {$myloc < [llength $mytoken]} {
	    set mycurrent [lindex $mytoken $myloc 0]
	    set myok    1
	    set myerror {}
	    return
	}

	# Actually read from the input, and remember
	# the information.
	# Note: We are implicitly incrementing the location!

	set token [my ReadChar]

	if {![llength $token]} {
	    set myok    0
	    set myerror [list $myloc [list $msg]]
	    return
	}

	lappend mytoken   $token
	set     mycurrent [lindex $token 0]
	set     myok      1
	set     myerror   {}
	return
    }

    method i_test_alnum {} {
	set myok [string is alnum -strict $mycurrent]
	my OkFail [pt::pe alnum]
	return
    }

    method i_test_alpha {} {
	set myok [string is alpha -strict $mycurrent]
	my OkFail [pt::pe alpha]
	return
    }

    method i_test_ascii {} {
	set myok [string is ascii -strict $mycurrent]
	my OkFail [pt::pe ascii]
	return
    }

    method i_test_char {tok} {
	set myok [expr {$tok eq $mycurrent}]
	my OkFail [pt::pe terminal $tok]
	return
    }

    method i_test_ddigit {} {
	set myok [string match {[0-9]} $mycurrent]
	my OkFail [pt::pe ddigit]
	return
    }

    method i_test_digit {} {
	set myok [string is digit -strict $mycurrent]
	my OkFail [pt::pe digit]
	return
    }

    method i_test_graph {} {
	set myok [string is graph -strict $mycurrent]
	my OkFail [pt::pe graph]
	return
    }

    method i_test_lower {} {
	set myok [string is lower -strict $mycurrent]
	my OkFail [pt::pe lower]
	return
    }

    method i_test_print {} {
	set myok [string is print -strict $mycurrent]
	my OkFail [pt::pe printable]
	return
    }

    method i_test_punct {} {
	set myok [string is punct -strict $mycurrent]
	my OkFail [pt::pe punct]
	return
    }

    method i_test_range {toks toke} {
	set myok [expr {
			([string compare $toks $mycurrent] <= 0) &&
			([string compare $mycurrent $toke] <= 0)
		    }] ; # {}
	my OkFail [pt::pe range $toks $toke]
	return
    }

    method i_test_space {} {
	set myok [string is space -strict $mycurrent]
	my OkFail [pt::pe space]
	return
    }

    method i_test_upper {} {
	set myok [string is upper -strict $mycurrent]
	my OkFail [pt::pe upper]
	return
    }

    method i_test_wordchar {} {
	set myok [string is wordchar -strict $mycurrent]
	my OkFail [pt::pe wordchar]
	return
    }

    method i_test_xdigit {} {
	set myok [string is xdigit -strict $mycurrent]
	my OkFail [pt::pe xdigit]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Internals

    method ReadChar {} {
	upvar 1 mychan mychan myline myline mycolumn mycolumn

	if {[eof $mychan]} {return {}}

	set ch [read $mychan 1]
	if {$ch eq ""} {return {}}

	set token [list $ch $myline $mycolumn]

	if {$ch eq "\n"} {
	    incr myline
	    set  mycolumn 0
	} else {
	    incr mycolumn
	}

	return $token
    }

    method OkFail {msg} {
	upvar 1 myok myok myerror myerror myloc myloc
	# Inlined: Expected, Unget, ClearErrors
	if {!$myok} {
	    set myerror [list $myloc [list $msg]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Data structures.
    ## Mainly the architectural state of the instance's PARAM.

    variable \
	mychan myline mycolumn \
	mycurrent myloc mystackloc \
	myok mysvalue myerror mystackerr \
	mytoken mysymbol \
	mystackast mystackmark

    # Parser Input (channel, location (line, column)) ...........
    # Token, current parsing location, stack of locations .......
    # Match state .  ........ ............. .....................
    # Caches for tokens and nonterminals .. .....................
    # Abstract syntax tree (AST) .......... .....................

    # # ## ### ##### ######## ############# #####################
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide pt::rde 1
return
