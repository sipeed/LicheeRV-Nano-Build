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
package require snit
package require struct::stack 1.5 ; # Requiring peekr, getr, trim* methods
package require pt::ast
package require pt::pe

# # ## ### ##### ######## ############# #####################
## Implementation

snit::type ::pt::rde_tcl {

    # # ## ### ##### ######## ############# #####################
    ## API - Lifecycle

    constructor {} {
	set mystackloc  [struct::stack ${selfns}::LOC]  ; # LS
	set mystackerr  [struct::stack ${selfns}::ERR]  ; # ES
	set mystackast  [struct::stack ${selfns}::AST]  ; # ARS/AS
	set mystackmark [struct::stack ${selfns}::MARK] ; # s.a.
	return
    }

    #TRACE variable count 0

    method reset {{chan {}}} { ; #TRACE puts "[format %8d [incr count]] RDE reset"
	set mychan    $chan      ; # IN
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

    method complete {} { ; #TRACE puts "[format %8d [incr count]] RDE complete"
	if {$myok} {
	    set n [$mystackast size]
	    if {$n > 1} {
		set  pos [$mystackloc peek]
		incr pos
		set children [$mystackast peekr [$mystackast size]] ; # SaveToMark
		return [pt::ast new {} $pos $myloc {*}$children]    ; # Reduce ALL
	    } else {
		return [$mystackast peek]
	    }
	} else {
	    lassign $myerror loc messages
	    return -code error [list pt::rde $loc $messages]
	}
    }

    # # ## ### ##### ######## ############# #####################
    ## API - State accessors

    method chan   {} { return $mychan }

    # - - -- --- ----- --------

    method current  {} { return $mycurrent }
    method location {} { return $myloc }
    method lmarked  {} { return [$mystackloc getr] }

    # - - -- --- ----- --------

    method ok      {} { return $myok      }
    method value   {} { return $mysvalue  }
    method error   {} { return $myerror   }
    method emarked {} { return [$mystackerr getr] }

    # - - -- --- ----- --------

    method tokens {{from {}} {to {}}} { ; #TRACE puts "[format %8d [incr count]] RDE tokens"
	switch -exact [llength [info level 0]] {
	    5 { return $mytoken }
	    6 { return [string range $mytoken $from $from] }
	    7 { return [string range $mytoken $from $to] }
	}
    }

    method symbols {} { ; #TRACE puts "[format %8d [incr count]] RDE symbols"
	return [array get mysymbol]
    }

    method scached {} { ; #TRACE puts "[format %8d [incr count]] RDE scached"
	return [array names mysymbol]
    }

    # - - -- --- ----- --------

    method asts    {} { return [$mystackast  getr] }
    method amarked {} { return [$mystackmark getr] }
    method ast     {} { return [$mystackast  peek] }

    # # ## ### ##### ######## ############# #####################
    ## API - Preloading the token cache.

    method data {data} { ; #TRACE puts "[format %8d [incr count]] RDE data"
	append mytoken $data
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Common instruction sequences

    method si:void_state_push {} {
	# i_loc_push
	# i_error_clear_push
	$mystackloc push $myloc
	set myerror {}
	$mystackerr push {}
	return
    }

    method si:void2_state_push {} {
	# i_loc_push
	# i_error_push
	$mystackloc push $myloc
	$mystackerr push {}
	return
    }

    method si:value_state_push {} {
	# i_ast_push
	# i_loc_push
	# i_error_clear_push
	$mystackmark push [$mystackast size]
	$mystackloc push $myloc
	set myerror {}
	$mystackerr push {}
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:void_state_merge {} {
	# i_error_pop_merge
	# i_loc_pop_rewind/discard

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	set last [$mystackloc pop]
	if {$myok} return
	set myloc $last
	return
    }

    method si:void_state_merge_ok {} {
	# i_error_pop_merge
	# i_loc_pop_rewind/discard
	# i_status_ok

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	set last [$mystackloc pop]
	if {$myok} return
	set myloc $last
	set myok 1
	return
    }

    method si:value_state_merge {} {
	# i_error_pop_merge
	# i_ast_pop_rewind/discard
	# i_loc_pop_rewind/discard

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	set mark [$mystackmark pop]
	set last [$mystackloc pop]
	if {$myok} return
	$mystackast trim* $mark
	set myloc $last
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:value_notahead_start {} {
	# i_loc_push
	# i_ast_push

	$mystackloc  push $myloc
	$mystackmark push [$mystackast size]
	return
    }

    method si:void_notahead_exit {} {
	# i_loc_pop_rewind
	# i_status_negate

	set myloc [$mystackloc pop]
	set myok [expr {!$myok}]
	return
    }

    method si:value_notahead_exit {} {
	# i_ast_pop_discard/rewind
	# i_loc_pop_rewind
	# i_status_negate

	set mark [$mystackmark pop]
	if {$myok} {
	    $mystackast trim* $mark
	}
	set myloc [$mystackloc pop]
	set myok [expr {!$myok}]
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:kleene_abort {} {
	# i_loc_pop_rewind/discard
	# i:fail_return

	set last [$mystackloc pop]
	if {$myok} return
	set myloc $last
	return -code return
    }

    method si:kleene_close {} {
	# i_error_pop_merge
	# i_loc_pop_rewind/discard
	# i:fail_status_ok
	# i:fail_return

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	set last [$mystackloc pop]
	if {$myok} return
	set myok 1
	set myloc $last
	return -code return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:voidvoid_branch {} {
	# i_error_pop_merge
	# i:ok_loc_pop_discard
	# i:ok_return
	# i_loc_rewind
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	if {$myok} {
	    $mystackloc pop
	    return -code return
	}
	set myloc [$mystackloc peek]
	$mystackerr push {}
	return
    }

    method si:voidvalue_branch {} {
	# i_error_pop_merge
	# i:ok_loc_pop_discard
	# i:ok_return
	# i_ast_push
	# i_loc_rewind
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}

	if {$myok} {
	    $mystackloc pop
	    return -code return
	}
	$mystackmark push [$mystackast size]
	set myloc [$mystackloc peek]
	$mystackerr push {}
	return
    }

    method si:valuevoid_branch {} {
	# i_error_pop_merge
	# i_ast_pop_rewind/discard
	# i:ok_loc_pop_discard
	# i:ok_return
	# i_loc_rewind
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}
	set mark [$mystackmark pop]
	if {$myok} {
	    $mystackloc pop
	    return -code return
	}
	$mystackast trim* $mark
	set myloc [$mystackloc peek]
	$mystackerr push {}
	return
    }

    method si:valuevalue_branch {} {
	# i_error_pop_merge
	# i_ast_pop_discard
	# i:ok_loc_pop_discard
	# i:ok_return
	# i_ast_rewind
	# i_loc_rewind
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}
	if {$myok} {
	    $mystackmark pop
	    $mystackloc pop
	    return -code return
	}
	$mystackast trim* [$mystackmark peek]
	set myloc [$mystackloc peek]
	$mystackerr push {}
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:voidvoid_part {} {
	# i_error_pop_merge
	# i:fail_loc_pop_rewind
	# i:fail_return
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}
	if {!$myok} {
	    set myloc [$mystackloc pop]
	    return -code return
	}
	$mystackerr push $myerror
	return
    }

    method si:voidvalue_part {} {
	# i_error_pop_merge
	# i:fail_loc_pop_rewind
	# i:fail_return
	# i_ast_push
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}
	if {!$myok} {
	    set myloc [$mystackloc pop]
	    return -code return
	}
	$mystackmark push [$mystackast size]
	$mystackerr push $myerror
	return
    }

    method si:valuevalue_part {} {
	# i_error_pop_merge
	# i:fail_ast_pop_rewind
	# i:fail_loc_pop_rewind
	# i:fail_return
	# i_error_push

	set olderror [$mystackerr pop]
	# We have either old or new error data, keep it.
	if {![llength $myerror]}  {
	    set myerror $olderror
	} elseif {[llength $olderror]} {
	    # If one of the errors is further on in the input choose
	    # that as the information to propagate.

	    lassign $myerror  loe msgse
	    lassign $olderror lon msgsn

	    if {$lon > $loe} {
		set myerror $olderror
	    } elseif {$loe == $lon} {
		# Equal locations, merge the message lists, set-like.
		set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	    }
	}
	if {!$myok} {
	    $mystackast trim* [$mystackmark pop]
	    set myloc [$mystackloc pop]
	    return -code return
	}
	$mystackerr push $myerror
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:next_str {tok} {
	# String = sequence of characters. No need for all the intermediate
	# stack churn.

	set n    [string length $tok]
	set last [expr {$myloc + $n}]
	set max  [string length $mytoken]

	incr myloc
	if {($last >= $max) && ![ExtendTCN [expr {$last - $max + 1}]]} {
	    set myok    0
	    set myerror [list $myloc [list [list t $tok]]]
	    # i:fail_return
	    return
	}
	set lex       [string range $mytoken $myloc $last]
	set mycurrent [string index $mytoken $last]

	set myok [expr {$tok eq $lex}]

	if {$myok} {
	    set myloc $last
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [list t $tok]]]
	    incr myloc -1
	}
	return
    }

    method si:next_class {tok} {
	# Class = Choice of characters. No need for stack churn.

	# i_input_next "\{t $c\}"
	# i:fail_return
	# i_test_<user class>

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list [list t $tok]]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	# Note what is needle versus hay. The token, i.e. the string
	# of allowed characters is the hay in which the current
	# character is looked, making it the needle.
	set myok [expr {[string first $mycurrent $tok] >= 0}]

	if {$myok} {
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [list t $tok]]]
	    incr myloc -1
	}
	return
    }

    method si:next_char {tok} {
	# i_input_next "\{t $c\}"
	# i:fail_return
	# i_test_char $c

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list [list t $tok]]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [expr {$tok eq $mycurrent}]
	if {$myok} {
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [list t $tok]]]
	    incr myloc -1
	}
	return
    }

    method si:next_range {toks toke} {
	#Asm::Ins i_input_next "\{.. $s $e\}"
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_range $s $e

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list [list .. $toks $toke]]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [expr {
			([string compare $toks $mycurrent] <= 0) &&
			([string compare $mycurrent $toke] <= 0)
		    }] ; # {}
	if {$myok} {
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [pt::pe range $toks $toke]]]
	    incr myloc -1
	}
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:next_alnum {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_alnum"
	#Asm::Ins i_input_next alnum
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_alnum

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list alnum]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is alnum -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list alnum]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_alpha {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_alpha"
	#Asm::Ins i_input_next alpha
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_alpha

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list alpha]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is alpha -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list alpha]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_ascii {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_ascii"
	#Asm::Ins i_input_next ascii
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_ascii

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list ascii]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is ascii -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list ascii]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_ddigit {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_ddigit"
	#Asm::Ins i_input_next ddigit
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_ddigit

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list ddigit]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string match {[0-9]} $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list ddigit]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_digit {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_digit"
	#Asm::Ins i_input_next digit
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_digit

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list digit]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is digit -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list digit]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_graph {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_graph"
	#Asm::Ins i_input_next graph
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_graph

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list graph]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is graph -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list graph]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_lower {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_lower"
	#Asm::Ins i_input_next lower
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_lower

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list lower]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is lower -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list lower]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_print {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_print"
	#Asm::Ins i_input_next print
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_print

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list print]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is print -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list print]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_punct {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_punct"
	#Asm::Ins i_input_next punct
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_punct

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list punct]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is punct -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list punct]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_space {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_space"
	#Asm::Ins i_input_next space
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_space

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list space]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is space -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list space]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_upper {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_upper"
	#Asm::Ins i_input_next upper
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_upper

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list upper]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is upper -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list upper]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_wordchar {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_wordchar"
	#Asm::Ins i_input_next wordchar
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_wordchar

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list wordchar]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is wordchar -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list wordchar]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    method si:next_xdigit {} { ; #TRACE puts "[format %8d [incr count]] RDE si:next_xdigit"
	#Asm::Ins i_input_next xdigit
	#Asm::Ins i:fail_return
	#Asm::Ins i_test_xdigit

	incr myloc
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list xdigit]]
	    # i:fail_return
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok [string is xdigit -strict $mycurrent]
	if {!$myok} {
	    set myerror [list $myloc [list xdigit]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    # - -- --- ----- -------- ------------- ---------------------

    method si:value_symbol_start {symbol} {
	# if @runtime@ i_symbol_restore $symbol
	# i:found:ok_ast_value_push
	# i:found_return
	# i_loc_push
	# i_ast_push

	set k [list $myloc $symbol]
	if {[info exists mysymbol($k)]} { 
	    lassign $mysymbol($k) myloc myok myerror mysvalue
	    if {$myok} {
		$mystackast push $mysvalue
	    }
	    return -code return
	}
	$mystackloc  push $myloc
	$mystackmark push [$mystackast size]
	return
    }

    method si:value_void_symbol_start {symbol} {
	# if @runtime@ i_symbol_restore $symbol
	# i:found_return
	# i_loc_push
	# i_ast_push

	set k [list $myloc $symbol]
	if {[info exists mysymbol($k)]} { 
	    lassign $mysymbol($k) myloc myok myerror mysvalue
	    return -code return
	}
	$mystackloc  push $myloc
	$mystackmark push [$mystackast size]
	return
    }

    method si:void_symbol_start {symbol} {
	# if @runtime@ i_symbol_restore $symbol
	# i:found:ok_ast_value_push
	# i:found_return
	# i_loc_push

	set k [list $myloc $symbol]
	if {[info exists mysymbol($k)]} { 
	    lassign $mysymbol($k) myloc myok myerror mysvalue
	    if {$myok} {
		$mystackast push $mysvalue
	    }
	    return -code return
	}
	$mystackloc push $myloc
	return
    }

    method si:void_void_symbol_start {symbol} {
	# if @runtime@ i_symbol_restore $symbol
	# i:found_return
	# i_loc_push

	set k [list $myloc $symbol]
	if {[info exists mysymbol($k)]} { 
	    lassign $mysymbol($k) myloc myok myerror mysvalue
	    return -code return
	}
	$mystackloc push $myloc
	return
    }

    method si:reduce_symbol_end {symbol} {
	# i_value_clear/reduce $symbol
	# i_symbol_save       $symbol
	# i_error_nonterminal $symbol
	# i_ast_pop_rewind
	# i_loc_pop_discard
	# i:ok_ast_value_push

	set mysvalue {}
	set at [$mystackloc pop]

	if {$myok} {
	    set  mark [$mystackmark peek];# Old size of stack before current nt pushed more.
	    set  newa [expr {[$mystackast size] - $mark}]
	    set  pos  $at
	    incr pos

	    if {!$newa} {
		set mysvalue {}
	    } elseif {$newa == 1} {
		# peek 1 => single element comes back
		set mysvalue [list [$mystackast peek]]     ; # SaveToMark
	    } else {
		# peek n > 1 => list of elements comes back
		set mysvalue [$mystackast peekr $newa]     ; # SaveToMark
	    }

	    if {$at == $myloc} {
		# The symbol did not process any input. As this is
		# signaled to be ok (*) we create a node covering an
		# empty range. (Ad *): Can happen for a RHS using
		# toplevel operators * or ?.
		set mysvalue [pt::ast new0 $symbol $pos {*}$mysvalue]
	    } else {
		set mysvalue [pt::ast new $symbol $pos $myloc {*}$mysvalue] ; # Reduce $symbol
	    }
	}

	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]

	if {[llength $myerror]} {
	    set  pos $at
	    incr pos
	    lassign $myerror loc messages
	    if {$loc == $pos} {
		set myerror [list $loc [list [list n $symbol]]]
	    }
	}

	$mystackast trim* [$mystackmark pop]
	if {$myok} {
	    $mystackast push $mysvalue
	}
	return
    }

    method si:void_leaf_symbol_end {symbol} {
	# i_value_clear/leaf $symbol
	# i_symbol_save       $symbol
	# i_error_nonterminal $symbol
	# i_loc_pop_discard
	# i:ok_ast_value_push

	set mysvalue {}
	set at [$mystackloc pop]

	if {$myok} {
	    set  pos $at
	    incr pos
	    if {$at == $myloc} {
		# The symbol did not process any input. As this is
		# signaled to be ok (*) we create a node covering an
		# empty range. (Ad *): Can happen for a RHS using
		# toplevel operators * or ?.
		set mysvalue [pt::ast new0 $symbol $pos]
	    } else {
		set mysvalue [pt::ast new $symbol $pos $myloc]
	    }
	}

	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]

	if {[llength $myerror]} {
	    set  pos $at
	    incr pos
	    lassign $myerror loc messages
	    if {$loc == $pos} {
		set myerror [list $loc [list [list n $symbol]]]
	    }
	}

	if {$myok} {
	    $mystackast push $mysvalue
	}
	return
    }

    method si:value_leaf_symbol_end {symbol} {
	# i_value_clear/leaf $symbol
	# i_symbol_save       $symbol
	# i_error_nonterminal $symbol
	# i_loc_pop_discard
	# i_ast_pop_rewind
	# i:ok_ast_value_push

	set mysvalue {}
	set at [$mystackloc pop]

	if {$myok} {
	    set  pos $at
	    incr pos
	    if {$at == $myloc} {
		# The symbol did not process any input. As this is
		# signaled to be ok (*) we create a node covering an
		# empty range. (Ad *): Can happen for a RHS using
		# toplevel operators * or ?.
		set mysvalue [pt::ast new0 $symbol $pos]
	    } else {
		set mysvalue [pt::ast new $symbol $pos $myloc]
	    }
	}

	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]

	if {[llength $myerror]} {
	    set  pos $at
	    incr pos
	    lassign $myerror loc messages
	    if {$loc == $pos} {
		set myerror [list $loc [list [list n $symbol]]]
	    }
	}

	$mystackast trim* [$mystackmark pop]
	if {$myok} {
	    $mystackast push $mysvalue
	}
	return
    }

    method si:value_clear_symbol_end {symbol} {
	# i_value_clear
	# i_symbol_save       $symbol
	# i_error_nonterminal $symbol
	# i_loc_pop_discard
	# i_ast_pop_rewind

	set mysvalue {}
	set at [$mystackloc pop]

	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]

	if {[llength $myerror]} {
	    set  pos $at
	    incr pos
	    lassign $myerror loc messages
	    if {$loc == $pos} {
		set myerror [list $loc [list [list n $symbol]]]
	    }
	}

	$mystackast trim* [$mystackmark pop]
	return
    }

    method si:void_clear_symbol_end {symbol} {
	# i_value_clear
	# i_symbol_save       $symbol
	# i_error_nonterminal $symbol
	# i_loc_pop_discard

	set mysvalue {}
	set at [$mystackloc pop]

	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]

	if {[llength $myerror]} {
	    set  pos $at
	    incr pos
	    lassign $myerror loc messages
	    if {$loc == $pos} {
		set myerror [list $loc [list [list n $symbol]]]
	    }
	}
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Control flow

    method i:ok_continue {} { ; #TRACE puts "[format %8d [incr count]] RDE i:ok_continue"
	if {!$myok} return
	return -code continue
    }

    method i:fail_continue {} { ; #TRACE puts "[format %8d [incr count]] RDE i:fail_continue"
	if {$myok} return
	return -code continue
    }

    method i:fail_return {} { ; #TRACE puts "[format %8d [incr count]] RDE i:fail_return"
	if {$myok} return
	return -code return
    }

    method i:ok_return {} { ; #TRACE puts "[format %8d [incr count]] RDE i:ok_return"
	if {!$myok} return
	return -code return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Unconditional matching.

    method i_status_ok {} { ; #TRACE puts "[format %8d [incr count]] RDE i_status_ok"
	set myok 1
	return
    }

    method i_status_fail {} { ; #TRACE puts "[format %8d [incr count]] RDE i_status_fail"
	set myok 0
	return
    }

    method i_status_negate {} { ; #TRACE puts "[format %8d [incr count]] RDE i_status_negate"
	set myok [expr {!$myok}]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Error handling.

    method i_error_clear {} { ; #TRACE puts "[format %8d [incr count]] RDE i_error_clear"
	set myerror {}
	return
    }

    method i_error_push {} { ; #TRACE puts "[format %8d [incr count]] RDE i_error_push"
	$mystackerr push $myerror
	return
    }

    method i_error_clear_push {} { ; #TRACE puts "[format %8d [incr count]] RDE i_error_clear_push"
	set myerror {}
	$mystackerr push {}
	return
    }

    method i_error_pop_merge {} { ; #TRACE puts "[format %8d [incr count]] RDE i_error_pop_merge"
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

	# Equal locations, merge the message lists, set-like.
	set myerror [list $loe [lsort -uniq [list {*}$msgse {*}$msgsn]]]
	return
    }

    method i_error_nonterminal {symbol} { ; #TRACE puts "[format %8d [incr count]] RDE i_error_nonterminal"
	# Inlined: Errors, Expected.
	if {![llength $myerror]} return
	set pos [$mystackloc peek]
	incr pos
	lassign $myerror loc messages
	if {$loc != $pos} return
	set myerror [list $loc [list [list n $symbol]]]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Basic input handling and tracking

    method i_loc_pop_rewind/discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i_loc_pop_rewind/discard (ok $myok ([expr {$myok ? "keep $myloc drop" : "back@"}] [$mystackloc peek]))"
	#$myparser i:fail_loc_pop_rewind
	#$myparser i:ok_loc_pop_discard
	#return
	set last [$mystackloc pop]
	if {$myok} return
	set myloc $last
	return
    }

    method i_loc_pop_discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i_loc_pop_discard"
	$mystackloc pop
	return
    }

    method i:ok_loc_pop_discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i:ok_loc_pop_discard"
	if {!$myok} return
	$mystackloc pop
	return
    }

    method i_loc_pop_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i_loc_pop_rewind"
	set myloc [$mystackloc pop]
	return
    }

    method i:fail_loc_pop_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i:fail_loc_pop_rewind"
	if {$myok} return
	set myloc [$mystackloc pop]
	return
    }

    method i_loc_push {} { ; #TRACE puts "[format %8d [incr count]] RDE i_loc_push (saving @$myloc)"
	$mystackloc push $myloc
	return
    }

    method i_loc_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i_loc_rewind"
	# i_loc_pop_rewind - set myloc [$mystackloc pop]
	# i_loc_push       - $mystackloc push $myloc    

	set myloc [$mystackloc peek]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - AST stack handling

    method i_ast_pop_rewind/discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_pop_rewind/discard"
	#$myparser i:fail_ast_pop_rewind
	#$myparser i:ok_ast_pop_discard
	#return
	set mark [$mystackmark pop]
	if {$myok} return
	$mystackast trim* $mark
	return
    }

    method i_ast_pop_discard/rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_pop_discard/rewind"
	#$myparser i:ok_ast_pop_rewind
	#$myparser i:fail_ast_pop_discard
	#return
	set mark [$mystackmark pop]
	if {!$myok} return
	$mystackast trim* $mark
	return
    }

    method i_ast_pop_discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_pop_discard"
	$mystackmark pop
	return
    }

    method i:ok_ast_pop_discard {} { ; #TRACE puts "[format %8d [incr count]] RDE i:ok_ast_pop_discard"
	if {!$myok} return
	$mystackmark pop
	return
    }

    method i_ast_pop_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_pop_rewind"
	$mystackast trim* [$mystackmark pop]
	return
    }

    method i:fail_ast_pop_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i:fail_ast_pop_rewind"
	if {$myok} return
	$mystackast trim* [$mystackmark pop]
	return
    }

    method i_ast_push {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_push"
	$mystackmark push [$mystackast size]
	return
    }

    method i:ok_ast_value_push {} { ; #TRACE puts "[format %8d [incr count]] RDE i:ok_ast_value_push"
	if {!$myok} return
	$mystackast push $mysvalue
	return
    }

    method i_ast_rewind {} { ; #TRACE puts "[format %8d [incr count]] RDE i_ast_rewind"
	# i_ast_pop_rewind - $mystackast  trim* [$mystackmark pop]
	# i_ast_push       - $mystackmark push [$mystackast size]

	$mystackast trim* [$mystackmark peek]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Nonterminal cache

    method i_symbol_restore {symbol} { ; #TRACE puts "[format %8d [incr count]] RDE i_symbol_restore ($symbol)"
	# Satisfy from cache if possible.
	set k [list $myloc $symbol]
	if {![info exists mysymbol($k)]} { return 0 }
	lassign $mysymbol($k) myloc myok myerror mysvalue
	# We go forward, as the nonterminal matches (or not).
	return 1
    }

    method i_symbol_save {symbol} { ; #TRACE puts "[format %8d [incr count]] RDE i_symbol_save ($symbol)"
	# Store not only the value, but also how far
	# the match went (if it was a match).
	set at [$mystackloc peek]
	set k  [list $at $symbol]
	set mysymbol($k) [list $myloc $myok $myerror $mysvalue]
	return
    }

    # # ## ### ##### ######## ############# #####################
    ##  API - Instructions - Semantic values.

    method i_value_clear {} { ; #TRACE puts "[format %8d [incr count]] RDE i_value_clear"
	set mysvalue {}
	return
    }

    method i_value_clear/leaf {symbol} { ; #TRACE puts "[format %8d [incr count]] RDE i_value_clear/leaf (ok $myok ([expr {[$mystackloc peek]+1}])-@$myloc)"
	# not quite value_lead (guarded, and clear on fail)
	# Inlined clear, reduce, and optimized.
	# Clear ; if {$ok} {Reduce $symbol}
	set mysvalue {}
	if {!$myok} return
	set  pos [$mystackloc peek]
	incr pos

	if {($pos - 1) == $myloc} {
	    # The symbol did not process any input. As this is
	    # signaled to be ok (*) we create a node covering an empty
	    # range. (Ad *): Can happen for a RHS using toplevel
	    # operators * or ?.
	    set mysvalue [pt::ast new0 $symbol $pos]
	} else {
	    set mysvalue [pt::ast new $symbol $pos $myloc]
	}
	return
    }

    method i_value_clear/reduce {symbol} { ; #TRACE puts "[format %8d [incr count]] RDE i_value_clear/reduce"
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
	    set mysvalue [$mystackast peekr $newa]     ; # SaveToMark
	}

	if {($pos - 1) == $myloc} {
	    # The symbol did not process any input. As this is
	    # signaled to be ok (*) we create a node covering an empty
	    # range. (Ad *): Can happen for a RHS using toplevel
	    # operators * or ?.
	    set mysvalue [pt::ast new0 $symbol $pos {*}$mysvalue]
	} else {
	    set mysvalue [pt::ast new $symbol $pos $myloc {*}$mysvalue] ; # Reduce $symbol
	}
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## API - Instructions - Terminal matching

    method i_input_next {msg} { ; #TRACE puts "[format %8d [incr count]] RDE i_input_next"
	# Inlined: Getch, Expected, ClearErrors
	# Satisfy from input cache if possible.

	incr myloc
	# May read from the input (ExtendTC), and remember the
	# information. Note: We are implicitly incrementing the
	# location!
	if {($myloc >= [string length $mytoken]) && ![ExtendTC]} {
	    set myok    0
	    set myerror [list $myloc [list $msg]]
	    return
	}
	set mycurrent [string index $mytoken $myloc]

	set myok    1
	set myerror {}
	return
    }

    method i_test_char {tok} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_char (ok [expr {$tok eq $mycurrent}], [expr {$tok eq $mycurrent ? "@$myloc" : "back@[expr {$myloc-1}]"}])"
	set myok [expr {$tok eq $mycurrent}]
	if {$myok} {
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [pt::pe terminal $tok]]]
	    incr myloc -1
	}
	return
    }

    method i_test_range {toks toke} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_range"
	set myok [expr {
			([string compare $toks $mycurrent] <= 0) &&
			([string compare $mycurrent $toke] <= 0)
		    }] ; # {}
	if {$myok} {
	    set myerror {}
	} else {
	    set myerror [list $myloc [list [pt::pe range $toks $toke]]]
	    incr myloc -1
	}
	return
    }

    method i_test_alnum {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_alnum"
	set myok [string is alnum -strict $mycurrent]
	OkFail alnum
	return
    }

    method i_test_alpha {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_alpha"
	set myok [string is alpha -strict $mycurrent]
	OkFail alpha
	return
    }

    method i_test_ascii {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_ascii"
	set myok [string is ascii -strict $mycurrent]
	OkFail ascii
	return
    }

    method i_test_ddigit {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_ddigit"
	set myok [string match {[0-9]} $mycurrent]
	OkFail ddigit
	return
    }

    method i_test_digit {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_digit"
	set myok [string is digit -strict $mycurrent]
	OkFail digit
	return
    }

    method i_test_graph {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_graph"
	set myok [string is graph -strict $mycurrent]
	OkFail graph
	return
    }

    method i_test_lower {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_lower"
	set myok [string is lower -strict $mycurrent]
	OkFail lower
	return
    }

    method i_test_print {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_print"
	set myok [string is print -strict $mycurrent]
	OkFail print
	return
    }

    method i_test_punct {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_punct"
	set myok [string is punct -strict $mycurrent]
	OkFail punct
	return
    }

    method i_test_space {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_space"
	set myok [string is space -strict $mycurrent]
	OkFail space
	return
    }

    method i_test_upper {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_upper"
	set myok [string is upper -strict $mycurrent]
	OkFail upper
	return
    }

    method i_test_wordchar {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_wordchar"
	set myok [string is wordchar -strict $mycurrent]
	OkFail wordchar
	return
    }

    method i_test_xdigit {} { ; #TRACE puts "[format %8d [incr count]] RDE i_test_xdigit"
	set myok [string is xdigit -strict $mycurrent]
	OkFail xdigit
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Debugging helper. To activate
    ## string map {{; #TRACE} {; TRACE}}

    proc TRACE {args} {
	uplevel 1 $args
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Internals

    proc ExtendTC {} {
	upvar 1 mychan mychan mytoken mytoken

	if {($mychan eq {}) ||
	    [eof $mychan]} {return 0}

	set ch [read $mychan 1]
	if {$ch eq {}} {
	    return 0
	}

	append mytoken $ch
	return 1
    }

    proc ExtendTCN {n} {
	upvar 1 mychan mychan mytoken mytoken

	if {($mychan eq {}) ||
	    [eof $mychan]} {return 0}

	set str [read $mychan $n]
	set k   [string length $str]

	append mytoken $str
	if {$k < $n} {
	    return 0
	}

	return 1
    }

    proc OkFail {msg} {
	upvar 1 myok myok myerror myerror myloc myloc
	# Inlined: Expected, Unget, ClearErrors
	if {!$myok} {
	    set myerror [list $myloc [list $ourmsg($msg)]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    proc OkFailD {msgcmd} {
	upvar 1 myok myok myerror myerror myloc myloc
	# Inlined: Expected, Unget, ClearErrors
	if {!$myok} {
	    set myerror [list $myloc [list [uplevel 1 $msgcmd]]]
	    incr myloc -1
	} else {
	    set myerror {}
	}
	return
    }

    # # ## ### ##### ######## ############# #####################
    ## Data structures.
    ## Mainly the architectural state of the instance's PARAM.

    # # ## ### ###### ######## #############
    ## Configuration

    pragma -hastypeinfo    0
    pragma -hastypemethods 0
    pragma -hasinfo        0

    #pragma -simpledispatch 1 ; # Cannot use this. Doing so breaks
    #                           # the use of 'return -code XXX' in
    #                           # the guarded control flow
    #                           # instructions, i.e.
    #                           # i:{ok,fail}_{continue,return}.

    typevariable ourmsg -array {}

    typeconstructor {
	set ourmsg(alnum)     [pt::pe alnum]
	set ourmsg(alpha)     [pt::pe alpha]
	set ourmsg(ascii)     [pt::pe ascii]
	set ourmsg(ddigit)    [pt::pe ddigit]
	set ourmsg(digit)     [pt::pe digit]
	set ourmsg(graph)     [pt::pe graph]
	set ourmsg(lower)     [pt::pe lower]
	set ourmsg(print)     [pt::pe printable]
	set ourmsg(punct)     [pt::pe punct]
	set ourmsg(space)     [pt::pe space]
	set ourmsg(upper)     [pt::pe upper]
	set ourmsg(wordchar)  [pt::pe wordchar]
	set ourmsg(xdigit)    [pt::pe xdigit]
	return
    }

    # Parser Input (channel, location (line, column)) ...........

    variable mychan          {} ; # IN. Channel we read the characters
				  # from. Its current location is
				  # where the next character will be
				  # read from, when needed.

    # Token, current parsing location, stack of locations .......

    variable mycurrent       {} ; # CC. Current character.
    variable myloc           -1 ; # CL. Location of 'mycurrent' as
				  # offset in the input, relative to
				  # the starting location.
    variable mystackloc      {} ; # LS. Stack object holding parsing
				  # location, see i_loc_mark_set,
				  # i_loc_mark_rewind,
				  # i_loc_mark_drop, and
				  # i_value_(leaf,range,reduce)

    # Match state .  ........ ............. .....................

    variable myok             0 ; # ST. Boolean flag indicating the
				  # success (true) or failure
				  # (failure) of the last match
				  # operation.
    variable mysvalue        {} ; # SV. The semantic value produced by
				  # the last match.
    variable myerror         {} ; # ER. Error information for the last
				  # match. Empty string if the match
				  # was ok, otherwise list (location,
				  # list (message...)).
    variable mystackerr      {} ; # ES. Stack object holding saved
				  # error states, see i_error_mark,
				  # i_error_merge

    # Caches for tokens and nonterminals .. .....................

    # list(list(char line col value))
    variable mytoken         {} ; # TC. String of all read characters,
				  # the tokens.
    variable mysymbol -array {} ; # NC. Cache of data about
				  # nonterminal symbols. Indexed by
				  # location and symbol name, value is
				  # a 4-tuple (go, ok, error, sv)

    # Abstract syntax tree (AST) .......... .....................
    # AS/ARS intertwined. ARS is top of mystackast, with the markers
    # on mystackmark showing there ARS ends and AS with older ARS
    # begins.

    variable mystackast      {} ; # ARS. Stack of semantic values
				  # (i.e. partial ASTs) to use in
				  # further AST construction, see
				  # i_ast_push, and i_ast_pop2mark.
    variable mystackmark     {} ; # AS. Stack of locations into the
				  # previous stack, see
				  # i_ast_mark_set,
				  # i_ast_mark_discard, and
				  # i_ast_mark_rewind.

    # # ## ### ##### ######## ############# #####################
}

# # ## ### ##### ######## ############# #####################
## Ready, return to manager.
return
