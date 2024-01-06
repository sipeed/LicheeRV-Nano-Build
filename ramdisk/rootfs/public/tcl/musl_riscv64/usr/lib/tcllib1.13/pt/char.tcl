# -*- tcl -*-
#
# Copyright (c) 2009 by Andreas Kupries <andreas_kupries@users.sourceforge.net>
# Operations with characters: (Un)quoting.

# ### ### ### ######### ######### #########
## Requisites

package require Tcl 8.5

namespace eval char {
    namespace export unquote quote
    namespace ensemble create
    namespace eval quote {
	namespace export tcl string comment cstring
	namespace ensemble create
    }
}

# ### ### ### ######### ######### #########
## API

proc ::char::unquote {args} {
    if {1 == [llength $args]} { return [Unquote {*}$args] }
    set res {}
    foreach ch $args { lappend res [Unquote $ch] }
    return $res
}

proc ::char::Unquote {ch} {

    # A character, stored in quoted form is transformed back into a
    # proper Tcl character (i.e. the internal representation).

    switch -exact -- $ch {
	"\\n"  {return \n}
	"\\t"  {return \t}
	"\\r"  {return \r}
	"\\["  {return \[}
	"\\]"  {return \]}
	"\\'"  {return '}
	"\\\"" {return "\""}
	"\\\\" {return \\}
    }

    if {[regexp {^\\([0-2][0-7][0-7])$} $ch -> ocode]} {
	return [format %c $ocode]

    } elseif {[regexp {^\\([0-7][0-7]?)$} $ch -> ocode]} {
	return [format %c 0$ocode]

    } elseif {[regexp {^\\u([[:xdigit:]][[:xdigit:]]?[[:xdigit:]]?[[:xdigit:]]?)$} $ch -> hcode]} {
	return [format %c 0x$hcode]

    }

    return $ch
}

proc ::char::quote::tcl {args} {
    if {1 == [llength $args]} { return [Tcl {*}$args] }
    set res {}
    foreach ch $args { lappend res [Tcl $ch] }
    return $res
}

proc ::char::quote::Tcl {ch} {
    # Converts a Tcl character (internal representation) into a string
    # which is accepted by the Tcl parser, will regenerate the
    # character in question and is 7bit ASCII.

    # Special characters

    switch -exact -- $ch {
	"\n" {return "\\n"}
	"\r" {return "\\r"}
	"\t" {return "\\t"}
	"\\" - "\;" -
	" "  - "\"" -
	"("  - ")"  -
	"\{" - "\}" -
	"\[" - "\]" {
	    # Quote space and all the brackets as well, using octal,
	    # for easy impure list-ness.

	    scan $ch %c chcode
	    return \\[format %o $chcode]
	}
    }

    scan $ch %c chcode

    # Control characters: Octal
    if {[::string is control -strict $ch]} {
	return \\[format %o $chcode]
    }

    # Beyond 7-bit ASCII: Unicode

    if {$chcode > 127} {
	return \\u[format %04x $chcode]
    }

    # Regular character: Is its own representation.

    return $ch
}

proc ::char::quote::string {args} {
    if {1 == [llength $args]} { return [String {*}$args] }
    set res {}
    foreach ch $args { lappend res [String $ch] }
    return $res
}

proc ::char::quote::String {ch} {
    # Converts a Tcl character (internal representation) into a string
    # which is accepted by the Tcl parser and will generate a human
    # readable representation of the character in question, one which
    # when written to a channel (via puts) describes the character
    # without using any unprintable characters. It may use backslash-
    # quoting. High utf characters are quoted to avoid problems with
    # the still prevalent ascii terminals. It is assumed that the
    # string will be used in a ""-quoted environment.

    # Special characters

    switch -exact -- $ch {
	" "  {return "<blank>"}
	"\n" {return "\\\\n"}
	"\r" {return "\\\\r"}
	"\t" {return "\\\\t"}
	"\"" - "\\" - "\;" -
	"("  - ")"  -
	"\{" - "\}" -
	"\[" - "\]" {
	    return \\$ch
	}
    }

    scan $ch %c chcode

    # Control characters: Octal
    if {[::string is control -strict $ch]} {
	return \\\\[format %o $chcode]
    }

    # Beyond 7-bit ASCII: Unicode

    if {$chcode > 127} {
	return \\\\u[format %04x $chcode]
    }

    # Regular character: Is its own representation.

    return $ch
}

proc ::char::quote::cstring {args} {
    if {1 == [llength $args]} { return [CString {*}$args] }
    set res {}
    foreach ch $args { lappend res [CString $ch] }
    return $res
}

proc ::char::quote::CString {ch} {
    # Converts a Tcl character (internal representation) into a string
    # which is accepted by the Tcl parser and will generate a human
    # readable representation of the character in question, one which
    # when written to a channel (via puts) describes the character
    # without using any unprintable characters. It may use backslash-
    # quoting. High utf characters are quoted to avoid problems with
    # the still prevalent ascii terminals. It is assumed that the
    # string will be used in a ""-quoted environment.

    # Special characters

    switch -exact -- $ch {
	"\n" {return "\\\\n"}
	"\r" {return "\\\\r"}
	"\t" {return "\\\\t"}
	"\"" - "\\" {
	    return \\$ch
	}
    }

    scan $ch %c chcode

    # Control characters: Octal
    if {[::string is control -strict $ch]} {
	return \\\\[format %o $chcode]
    }

    # Beyond 7-bit ASCII: Unicode

    if {$chcode > 127} {
	return \\\\u[format %04x $chcode]
    }

    # Regular character: Is its own representation.

    return $ch
}

proc ::char::quote::comment {args} {
    if {1 == [llength $args]} { return [Comment {*}$args] }
    set res {}
    foreach ch $args { lappend res [Comment $ch] }
    return $res
}

proc ::char::quote::Comment {ch} {
    # Converts a Tcl character (internal representation) into a string
    # which is accepted by the Tcl parser when used within a Tcl
    # comment.

    # Special characters

    switch -exact -- $ch {
	" "  {return "<blank>"}
	"\n" {return "\\n"}
	"\r" {return "\\r"}
	"\t" {return "\\t"}
	"\"" -
	"\{" - "\}" -
	"("  - ")"  {
	    return \\$ch
	}
    }

    scan $ch %c chcode

    # Control characters: Octal
    if {[::string is control -strict $ch]} {
	return \\[format %o $chcode]
    }

    # Beyond 7-bit ASCII: Unicode

    if {$chcode > 127} {
	return \\u[format %04x $chcode]
    }

    # Regular character: Is its own representation.

    return $ch
}

# ### ### ### ######### ######### #########
## Ready

package provide char 1

