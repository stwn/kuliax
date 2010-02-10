#
# Tcl/Tk windowing routines for cnet v2.0.10 (March 2004)
#
# Written by Chris McDonald (chris@csse.uwa.edu.au)
# with thanks to Michael J. Robins.
#
# The cnet network simulator (v2.0.10)
# Copyright (C) 1992-2006, Chris McDonald
# 
# Chris McDonald, chris@csse.uwa.edu.au
# Department of Computer Science & Software Engineering
# The University of Western Australia,
# Crawley, Western Australia, 6009
# PH: +61 8 9380 2533, FAX: +61 8 9380 1089.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#
# All of the following constants may be safely modified:
#
set buttonfont		"-adobe-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*"
set labelfont		"-adobe-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*"
set stdiofont		"-*-courier-medium-r-*-*-10-*-*-*-*-*-*-*"
set stdiocols	 	80
set stdiorows	 	24
set stdiohistory 	60
set stdiobg	 	wheat
set stdiofg	 	black
set tracerows	 	24
set tracehistory 	60
set tracebg	 	wheat
set tracefg	 	black
set scrollside	 	right
set stdbg	 	grey80

# ----------------------------------------------------------------

set fw [font measure $labelfont m]
set fh [expr [font metrics $labelfont -linespace] + 2]

proc SetWindowTitle {w str} {
    toplevel $w
    wm title $w "cnet: $str"
}

proc InitMainWindow {progname topology exitproc runproc saveproc} {
    global buttonfont labelfont
    global stdbg
    global tk_version
    global CN_DELIVERY

    wm withdraw .

    set w .$progname
    SetWindowTitle $w $topology
    wm protocol $w WM_DELETE_WINDOW "$exitproc"
    wm resizable $w 0 0

    set f [frame $w.title]
    button $f.exit	-text	"Exit $progname" \
	-background $stdbg \
	-font $buttonfont \
	-command $exitproc \
	-relief raised -bd 2
    label $f.msgs	\
	-background $stdbg \
	-font $labelfont \
	-text "Messages delivered correctly:"
    set CN_DELIVERY	"     0 (100.0%)"
    label $f.deld \
	-background $stdbg \
	-font $buttonfont \
	-textvariable CN_DELIVERY

    pack $f.exit $f.msgs $f.deld -side left -fill both -expand yes
    pack $f -side top -fill both -expand yes

    set f [frame $w.top]
    button $f.run	-textvariable CN_RUN_STRING \
	-background $stdbg \
	-font $buttonfont \
	-command	"$runproc run" \
	-width 8 \
	-relief raised -bd 2
    button $f.single	-text	"Single event" \
	-background $stdbg \
	-font $buttonfont \
	-command	"$runproc step" \
	-relief raised -bd 2
    button $f.save	-text	"Save topology" \
	-background $stdbg \
	-font $buttonfont \
	-command	"ToggleSave $saveproc" \
	-relief raised -bd 2

    menubutton $f.subwins \
	-background $stdbg \
	-text Subwindows \
	-indicatoron true \
	-direction flush \
	-font $buttonfont \
	-relief raised -bd 2

    set m [menu $f.subwins.popup]
    if {$tk_version > 3.6} \
	then {$m configure -tearoff false}
    $m add command \
	-background $stdbg \
	-label "default node attributes" \
	-font $buttonfont \
	-command "ToggleNodeAttributes default"
    $m add command \
	-background $stdbg \
	-label "default link attributes" \
	-font $buttonfont \
	-command "ToggleLinkAttributes default default default $w 0 0"
    $m add command \
	-background $stdbg \
	-label "network events" \
	-font $buttonfont \
	-command "ToggleLoad"
    $m add command \
	-background $stdbg \
	-label "statistics" \
	-font $buttonfont \
	-command "ToggleStatistics"

    $f.subwins configure \
	-menu $m

    pack $f.run $f.single $f.save $f.subwins -side left -fill both -expand yes
    pack $f -side top -fill both -expand yes
}

proc InitCanvas {w width height} {
    canvas $w -width $width -height $height  \
	-background white -relief sunken -border 2
    pack $w -side top -fill both -expand yes
}

proc InitCanvasFrame {w name cmd} {
    global buttonfont labelfont
    global stdbg
    global UPDATE_TITLE

    frame $w
    button $w.close \
	-background $stdbg \
	-font	$buttonfont \
	-text	 "Close $name" \
	-command $cmd \
	-relief raised -bd 2
    label $w.update \
	-background $stdbg \
	-font $labelfont \
	-text $UPDATE_TITLE

    pack $w.close $w.update -side left -fill both -expand yes
    pack $w -side top -fill both -expand yes
}

# ----------------------------------------------------------------

proc InitTraceWindow {w} {
    global stdiofont fw
    global tracerows tracehistory tracewin
    global tracebg tracefg
    global scrollside

    frame $w
    set tracewin [text $w.t \
		    -font $stdiofont \
		    -height $tracerows \
		    -background $tracebg \
		    -foreground $tracefg \
		    -wrap none \
		    -yscrollcommand "$w.ysbar set" ]
    scrollbar $w.ysbar -width 10 -orient vertical   -command "$w.t yview"
    if {$scrollside == "right"} then {
	pack $w.t -side left -fill both -expand yes
	pack $w.ysbar -side left -fill both -expand no
    } else {
	pack $w.ysbar -side left -fill both -expand no
	pack $w.t -side left -fill both -expand yes
    }
    pack $w -side top -fill both -expand yes
    for {set l1 1} {$l1 < $tracehistory} {incr l1} {
	$tracewin insert $l1.0 "\n"
    }
    $tracewin see $tracehistory.0
}

proc traceaddstr {str newline} {
    global tracehistory tracewin

#    $tracewin configure -state normal
    $tracewin insert $tracehistory.end "$str"
    if {$newline} then {
	for {set l1 1; set l2 2} {$l1 <= $tracehistory} {incr l1; incr l2} {
	    $tracewin delete $l1.0 $l1.end
	    $tracewin insert $l1.0 [$tracewin get $l2.0 $l2.end]
	}
    }
#    $tracewin configure -state disabled
}


# ----------------------------------------------------------------

proc ToggleLoad {} {
    global cnet_evname load_displayed
    global labelfont fw fh

    set w .load
    if {[info command $w] == ""} {
	SetWindowTitle $w "load"
	wm protocol $w WM_DELETE_WINDOW "ToggleLoad"
	InitCanvasFrame $w.top "load" "ToggleLoad"
	InitCanvas $w.can [expr 19 * $fw + 160 ] [expr 25 * $fh]

	set x [expr 15 * $fw]
	set y $fh
	for {set n 0} {$n < 23} {incr n} {
	    $w.can create text $x $y -anchor e -font $labelfont \
		    -text $cnet_evname($n)
	    incr y $fh
	}

	set x [expr $x + 150 + $fw]
	set y $fh
	$w.can create text $x $y -anchor w -font $labelfont \
		-text "#events"

	set x  [expr 15 * $fw + $fw / 2]
	set y  [expr $fh / 2]
	set y2 [expr $y + 23 * $fh]
	$w.can create rectangle $x $y [expr $x + 150] $y2
	for {set p 0} {$p <= 100} {incr p 20} {
	    $w.can create line $x $y $x $y2 -stipple gray12
	    $w.can create text $x $y2 -anchor n -font $labelfont -text $p%
	    incr x 30
	}

	set load_displayed true
	wm resizable $w 0 0

    } elseif {[wm state $w] == "withdrawn"} {
	wm deiconify $w
	set load_displayed true
    } else {
	wm withdraw $w
	set load_displayed false
    }
}

proc UpdateLoad {total s} {
    global labelfont stdiofont fw fh

    set w .load

    $w.can delete s
    set x  [expr 15 * $fw + $fw / 2]
    set t  [expr $x + 150 + 4 * $fw]
    set y  [expr $fh / 2]
    set y2 $fh
# no need to consider EV_NULL
    for {set n 1} {$n < 23} {incr n} {
	incr y  $fh
	incr y2 $fh
	set v [lindex $s $n]
	if {$v != 0} {
	    set len [expr $v * 150 / $total ]
	    $w.can create rectangle $x $y [expr $x + $len] [expr $y + $fh] \
		    -fill blue -tags {s} -stipple gray75
	    $w.can create text $t $y2 -anchor e \
		    -font $stdiofont -text $v -tags {s}
	}
    }
}

# ----------------------------------------------------------------

proc SaveTopology {saveproc e} {
    $saveproc [$e get]
    wm withdraw .save
}

proc ToggleSave {saveproc} {
    global buttonfont labelfont
    global stdbg

    set w .save
    if {[info command $w] == ""} {
	SetWindowTitle $w "save topology"
	wm protocol $w WM_DELETE_WINDOW "ToggleSave $saveproc"
	set s [frame $w.f]
	label $s.l \
	    -background $stdbg \
	    -font $labelfont \
	    -text "Topology filename:"
	entry $s.e	-background white -width 30
	bind $s.e <Return> "SaveTopology $saveproc $s.e"
	pack $s.l $s.e -side left -fill both -expand yes
	pack $s

	button $w.save \
	    -background $stdbg \
	    -font $buttonfont \
	    -text save \
	    -command "SaveTopology $saveproc $s.e"
	button $w.cancel \
	    -background $stdbg \
	    -font $buttonfont \
	    -text cancel \
	    -command "wm withdraw $w"
	pack $w.save $w.cancel -side left -fill both -expand yes
    } elseif {[wm state $w] == "withdrawn"} {
	wm deiconify $w
    } else {
	wm withdraw $w
    }
}

# ----------------------------------------------------------------

proc ToggleStatistics {} {
    global STATS_TITLE stats_displayed
    global labelfont stdiofont fw fh

    set w .stats
    if {[info command $w] == ""} {
	SetWindowTitle $w "statistics"
	wm protocol $w WM_DELETE_WINDOW "ToggleStatistics"
	InitCanvasFrame $w.top "statistics" "ToggleStatistics"

	InitCanvas $w.can [expr 29 * $fw] [expr 14 * $fh]

	set x [expr 2 * $fw]
	set y $fh
	for {set n 0} {$n < 13} {incr n} {
	    $w.can create text $x $y -anchor w -font $labelfont \
		    -text $STATS_TITLE($n)
	    incr y $fh
	}

	set x [expr 26 * $fw]
	set y [expr  1 * $fh]
	$w.can create text $x $y -anchor w -font $labelfont \
		    -text $STATS_TITLE(13)
	set y [expr  6 * $fh]
	$w.can create text $x $y -anchor w -font $labelfont \
		    -text $STATS_TITLE(14)
	set y [expr  7 * $fh]
	$w.can create text $x $y -anchor w -font $labelfont \
		    -text $STATS_TITLE(15)
	set y [expr  8 * $fh]
	$w.can create text $x $y -anchor w -font $labelfont \
		    -text $STATS_TITLE(16)

	set stats_displayed true
	wm resizable $w 0 0

    } elseif {[wm state $w] == "withdrawn"} {
	wm deiconify $w
	set stats_displayed true
    } else {
	wm withdraw $w
	set stats_displayed false
    }
}

proc UpdateMainstats {s} {
    global labelfont stdiofont fw fh

    set w .stats

    $w.can delete s
    set x [expr 25 * $fw]
    set y $fh
    for {set n 0} {$n < 13} {incr n} {
	$w.can create text $x $y -anchor e -font $stdiofont \
		-text [lindex $s $n] -tags {s}
	incr y $fh
    }
}


# ----------------------------------------------------------------

proc InitNode {n nodename nodetype mainwin x y nattr} {

    global buttonfont labelfont fw fh
    global stdbg
    global trace_events stdio_quiet
    global node_choice_title node_choice_set

    set w .node$n
    if {$nodename == "default"} {
	set wintitle "default node attributes"
	set tracetitle "trace all events"
	set nn -1
    } else {
	set wintitle "$nodename"
	set tracetitle "trace this node's events"
	set nn 0
    }
    SetWindowTitle $w "$wintitle"
    wm protocol $w WM_DELETE_WINDOW "ToggleNodeAttributes $n"

    set f [frame $w.top]
    button $f.close \
	-background $stdbg \
	-font		$buttonfont \
	-text		"Close $wintitle" \
	-command	"ToggleNodeAttributes $n" \
	-relief raised -bd 2
    checkbutton $f.trace \
	-text	$tracetitle \
	-background $stdbg \
	-font	$buttonfont \
	-variable "trace_events($n)" \
	-relief flat
    checkbutton $f.quiet \
	-text	"stdio quiet" \
	-background $stdbg \
	-font	$buttonfont \
	-variable "stdio_quiet($n)" \
	-relief flat

    pack $f.close $f.trace $f.quiet -side left -anchor e -fill both -expand yes
    pack $w.top -side top -fill both -expand yes

    if {$nodetype != "router"} {
	set na 0
	foreach d {0 1 2} {
	    set f [frame $w.choice$d]
	    label $f.lab \
		-background $stdbg \
		-width 20 \
		-font	$labelfont \
		-text	$node_choice_title($d)
	    pack $f.lab -anchor w -side left
	    foreach c {0 1 2 3} {
		radiobutton $f.$c \
		    -anchor w \
		    -background $stdbg \
		    -width  10 \
		    -font   $buttonfont \
		    -text   [lindex $nattr $na] \
		    -value  $c \
		    -variable $f.selected \
		    -relief raised -bd 2 \
		    -command "node_choice $n $d $c"
		incr na
	    }
	    $f.$node_choice_set($nn,$d) select
	    pack $f.0 $f.1 $f.2 $f.3 -side left -fill both -expand yes
	}
	pack $w.choice0 $w.choice1 $w.choice2 -side top -fill both -expand no
    }
    if {$nodetype != "default"} {
	if {$nodetype == "host"} {
	    InitCanvas $w.stats [expr 50 * $fw] [expr 5 * $fh]

	    set x [expr 20 * $fw]
	    set y $fh
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "MESSAGES"
	    set x [expr 32 * $fw]
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "BYTES"
	    set x [expr 42 * $fw]
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "KBytes/sec"

	    set x [expr 6 * $fw]
	    set y [expr 2 * $fh]
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "Generated"
	    incr y $fh
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "Received OK"
	    incr y $fh
	    $w.stats create text $x $y -anchor w -font $labelfont \
			-text "Errors Recv'd"
	}

	set f [frame $w.d]
	foreach i {0 1 2 3 4} {
	    button "$f.debug$i"	-text " " \
		-background $stdbg \
		-font	$buttonfont \
		-command	"node_debug_button $n $i" \
		-relief raised -bd 2
	}
	pack $f.debug0 $f.debug1 $f.debug2 $f.debug3 $f.debug4 \
	    -side left -fill both -expand yes
	pack $f -side top -fill both -expand yes

	stdioinit $w $n
    }
    set node_displayed($n) false
    wm withdraw $w
    wm resizable $w 0 0
}

proc UpdateNodeStats {n s} {
    global labelfont stdiofont fw fh

    set w .node$n
    $w.stats delete s

    for {set n 0} {$n < 8} {incr n} {
	set x [expr $fw * (26 + ($n % 3)*10)]
	set y [expr $fh * ( 2 + ($n / 3))]
	$w.stats create text $x $y -anchor w -font $stdiofont \
		    -anchor e \
		    -text [lindex $s $n] -tags {s}
    }
}

proc ToggleNodeAttributes {n} {
    global node_displayed

    set w .node$n
    if {[wm state $w] == "withdrawn"} {
	wm deiconify $w
	set node_displayed($n) true
    } else {
	wm withdraw $w
	set node_displayed($n) false
    }
}

proc SetDebugString {w str} {
    global buttonfont
    $w configure -font $buttonfont -text $str
}

# ----------------------------------------------------------------

proc ToggleLinkAttributes {from to linkname mainwin x y } {
    global buttonfont labelfont fw fh
    global stdbg
    global link_scale_title link_scale_max link_scale_value
    global link_displayed

    set w .link($from,$to)
    if {[info command $w] == ""} {

	regsub -all "(\\+|x)" [wm geometry $mainwin] " " geom
	if {$linkname == "default"} {
	    set x [expr [lindex $geom 0] + [lindex $geom 2] - 100]
	    set y [expr [lindex $geom 1] + [lindex $geom 3] +  20]
	    set wintitle "default link attributes"
	} else {
	    set x [expr $x + [lindex $geom 2] + 30]
	    set y [expr $y + [lindex $geom 3] + 90]
	    set wintitle "$linkname"
	}
	SetWindowTitle $w "$wintitle"
	wm protocol $w WM_DELETE_WINDOW \
		"ToggleLinkAttributes $from $to \"$linkname\" $mainwin 0 0"
	wm geometry $w +$x+$y

	set t [frame $w.top]
	button $t.close \
	    -background $stdbg \
	    -font $buttonfont \
	    -text	"Close $wintitle" \
	    -command	"ToggleLinkAttributes $from $to \"$linkname\" $w 0 0" \
	    -relief raised -bd 2
	pack $t.close -side left -anchor e -fill both -expand yes
	pack $t -side top -fill both -expand yes

	set min  0
	if {$from != "default"} {
	    InitCanvas $w.stats [expr 45 * $fw] [expr 5 * $fh]

	    if {$from == "eth"} {
		set sh { "Packets" "Bytes" "KBytes/sec" }
		set sv { "Transmitted (10Mbps)" "Received" "Collisions" }
	    } else {
		set sh { "Frames" "Bytes" "KBytes/sec" }
		set sv { "Transmitted" "Received" "Errors Introduced" }
	    }

	    set y $fh
	    set x [expr 22 * $fw]
	    foreach n {0 1 2} {
		$w.stats create text $x $y -anchor e -font $labelfont \
			    -text [lindex $sh $n]
		incr x [expr 10 * $fw]
	    }

	    set x [expr 2 * $fw]
	    set y [expr 2 * $fh]
	    foreach n {0 1 2} {
		$w.stats create text $x $y -anchor w -font $labelfont \
			    -text [lindex $sv $n]
		incr y $fh
	    }
	    set min -1
	}
	link_created $from $to

	if {$from != "eth"} {
	    set b [frame $w.bottom]
	    foreach c {0 1 2 3} {
		set f [frame $b.scale$c]
		label $f.lab \
		    -background $stdbg \
		    -width 25 \
		    -font	$labelfont \
		    -anchor e \
		    -text	"$link_scale_title($c) : "
		pack $f.lab -anchor w -side left -fill both -expand yes
		scale $f.s	-from $min -to $link_scale_max($c) \
		    -background $stdbg \
		    -orient horizontal \
		    -length 220 \
		    -showvalue 1 \
		    -font $labelfont \
		    -command "link_scale $from $to $c"
		$f.s set $link_scale_value($c)
		pack $f.s -side left -fill both -expand yes
	    }

	    pack $b.scale0 $b.scale1 $b.scale2 $b.scale3 \
		-side top -fill both -expand yes
	    pack $b -side top -fill both -expand yes
	}

	set link_displayed($from,$to) true
	wm resizable $w 0 0

    } elseif {[wm state $w] == "withdrawn"} {
	wm deiconify $w
	set link_displayed($from,$to) true
    } else {
	wm withdraw $w
	set link_displayed($from,$to) false
    }
}

proc UpdateLinkStats {from to s} {
    global labelfont stdiofont fw fh

    set w .link($from,$to)
    $w.stats delete s

    for {set n 0} {$n < 8} {incr n} {
	set x [expr $fw * (22 + ($n % 3)*10)]
	set y [expr $fh * ( 2 + ($n / 3))]
	$w.stats create text $x $y -anchor e -font $stdiofont \
		    -text [lindex $s $n] -tags {s}
    }
}

# ----------------------------------------------------------------

set nodemenu_displayed -1
# only allow one nodemenu

proc PopupNodeMenu {n nodename mainwin activemask x y} {
    global nodemenu_label nodemenu_displayed tk_version
    global stdbg

    set m .nodemenu
    if {[info command $m] == ""} {
	if {$tk_version > 3.6} \
	    then {menu $m -tearoff false -background $stdbg} \
	    else {menu $m -background $stdbg}

	$m add command -label $nodename -state disabled
	$m add separator
	foreach s {0 1 2 3 4} {
	    $m add command -label $nodemenu_label($s) \
			   -command "PopdownNodeMenu $m $s"
	}
    }
    if {$nodemenu_displayed != -1} {
	set nodemenu_displayed -1
	$m unpost
	return
    }

    $m entryconfigure 0 -label $nodename
    foreach s {0 1 2 3 4} {
	$m entryconfigure [expr $s + 2] \
	    -state [expr {[expr $activemask&(1<<$s)] ? "normal" : "disabled"}]
    }
    set nodemenu_displayed $n

    regsub -all "(\\+|x)" [wm geometry $mainwin] " " geom
    set x [expr $x + [lindex $geom 2]]
    set y [expr $y + [lindex $geom 3] + 10]
    $m post $x $y
}

proc PopdownNodeMenu {m s} {
    global nodemenu_displayed nodemenu_select

    nodemenu_select $nodemenu_displayed $s
    set nodemenu_displayed -1
    $m unpost
}

# ----------------------------------------------------------------

set linkmenu_displayed -1
# only allow one linkmenu

proc PopupLinkMenu {l linkname mainwin activemask x y} {
    global linkmenu_label linkmenu_displayed tk_version
    global stdbg

    set m .linkmenu
    if {[info command $m] == ""} {
	if {$tk_version > 3.6} \
	    then {menu $m -tearoff false -background $stdbg} \
	    else {menu $m -background $stdbg}

	$m add command -label $linkname -state disabled
	$m add separator
	foreach s {0 1 2 3} {
	    $m add command -label $linkmenu_label($s) \
			   -command "PopdownLinkMenu $m $s"
	}
    }
    if {$linkmenu_displayed != -1} {
	set linkmenu_displayed -1
	$m unpost
	return
    }

    $m entryconfigure 0 -label $linkname
    foreach s {0 1 2 3} {
	$m entryconfigure [expr $s + 2] \
	    -state [expr {[expr $activemask&(1<<$s)] ? "normal" : "disabled"}]
    }
    set linkmenu_displayed $l

    regsub -all "(\\+|x)" [wm geometry $mainwin] " " geom
    set x [expr $x + [lindex $geom 2]]
    set y [expr $y + [lindex $geom 3] + 10]
    $m post $x $y
}

proc PopdownLinkMenu {m s} {
    global linkmenu_displayed linkmenu_select

    linkmenu_select $linkmenu_displayed $s
    set linkmenu_displayed -1
    $m unpost
}

# ------------------------------------------------------------------------

proc DrawFrame {progname nfields x y tag cvec lvec dir str} {
    global labelfont
    set colours {green purple cyan red yellow blue}

    set x0 $x
    set y0 [expr $y + 16]
    for {set n 0} {$n < $nfields} {incr n} {
	set dx [lindex $lvec $n]
	set x1 [expr $x + ($dir * $dx)]
	set c  [expr [lindex $cvec $n] - 1]
	.$progname.df create rectangle $x $y $x1 $y0 \
	    -fill [lindex $colours $c] -tags $tag
	set x $x1
    }
    .$progname.df create text [expr ($x0 + $x) / 2] [expr $y + 8] \
	-anchor c -font $labelfont \
	-text $str -tags t$tag
}

# ------------------------------------------------------------------------

proc stdioinit {w n} {
    global stdiowin stdiofont stdioinput
    global stdiorows stdiocols stdiohistory stdiobg stdiofg
    global scrollside

    set win		$w.stdio
    set stdiowin($n)	$win
    set stdioinput($n)	""

    text $win \
	-font $stdiofont \
	-width $stdiocols \
	-height $stdiorows \
	-background $stdiobg \
	-foreground $stdiofg \
	-wrap none \
	-yscrollcommand "$w.ysbar set"
    scrollbar $w.ysbar -width 10 -orient vertical   -command "$win yview"
    if {$scrollside == "right"} then {
	pack $win -side left -fill both -expand yes
	pack $w.ysbar -side left -fill both -expand no
    } else {
	pack $w.ysbar -side left -fill both -expand no
	pack $win -side left -fill both -expand yes
    }
    for {set l1 1} {$l1 < $stdiohistory} {incr l1} {
	$win insert $l1.0 "\n"
    }
    bind $win <Enter>    { focus %W ; break }
    bind $win <KeyPress> "stdiokey $n %K %A ; break"
    $win see $stdiohistory.0
}

proc stdioclr {n} {
    global stdiowin stdiohistory

    set win $stdiowin($n)
    $win delete 1.0 end
    for {set l1 1} {$l1 < $stdiohistory} {incr l1} {
	$win insert $l1.0 "\n"
    }
    $win see $stdiohistory.0
}

proc stdioaddstr {n str newline} {
    global stdiowin stdiohistory

    set win $stdiowin($n)
    $win insert $stdiohistory.end "$str"
    if {$newline} then {
	for {set l1 1; set l2 2} {$l1 <= $stdiohistory} {incr l1; incr l2} {
	    $win delete $l1.0 $l1.end
	    $win insert $l1.0 [$win get $l2.0 $l2.end]
	}
    }
}

proc stdiokey {n keysym ascii} {
    global stdiowin stdioinput stdiohistory

    set win $stdiowin($n)
    if {$keysym == "BackSpace" || $keysym == "Delete"} then {
	set len [string length $stdioinput($n)]
	if {$len == 0} then {
	    bell
	} else {
	    set s $stdioinput($n)
	    set s [string range $s 0 [expr $len - 2]]
	    set stdioinput($n) $s
	    $win delete "insert - 1 char"
	}
    } elseif {$keysym == "Return" } then {
	for {set l1 1; set l2 2} {$l1 < $stdiohistory} {incr l1; incr l2} {
	    $win delete $l1.0 $l1.end
	    $win insert $l1.0 [$win get $l2.0 $l2.end]
	}
	$win delete $stdiohistory.0 $stdiohistory.end
	stdio_input $n "$stdioinput($n)"
	set stdioinput($n)	""
    } else {
	set stdioinput($n) "$stdioinput($n)$ascii"
	$win insert $stdiohistory.end $ascii
    }
}

# ------------------------------------------------------------------------

set exitplease 0

proc show_error {msg filenm errline} {
    global stdbg
    global stdiorows stdiocols
    global stdiobg stdiofg stdiofont
    global buttonfont labelfont
    global exitplease

    set win [toplevel .error]
    set f [frame $win.f]

    button $f.close -font $buttonfont -text "Exit cnet" \
		-background $stdbg \
		-command {set exitplease 1}
    label $f.l -justify left \
		-background $stdbg \
		-font $labelfont -text "$msg"
    pack $f.close $f.l -side left -fill both -expand yes
    pack $f -side top -fill both -expand yes

    if {$filenm != ""} {
        set cmd {
            set fid [open $filenm "r"]
        }
        if {[catch $cmd result] == 0} {
	    set win [frame $win.code]
	    set t [text $win.t -wrap none \
		-width $stdiocols -height $stdiorows \
		-background $stdiobg -foreground $stdiofg \
		-font $stdiofont \
		-xscrollcommand "$win.xsbar set" \
		-yscrollcommand "$win.ysbar set" ]
	    $t tag configure oops -background red -foreground white
	    scrollbar $win.xsbar -width 12 \
				-orient horizontal -command "$t xview"
	    scrollbar $win.ysbar -width 12 \
				-orient vertical   -command "$t yview"
	    grid $t $win.ysbar -sticky nsew
	    grid $win.xsbar -sticky nsew
	    grid columnconfigure $win 0 -weight 1
	    grid    rowconfigure $win 0 -weight 1

	    $t configure -state normal
	    $t delete 1.0 end

	    set lc 0
	    while {1} {
		set line [gets $fid]
		if {[eof $fid]} {break}
		incr lc
		if {$lc == $errline} {
		    $t insert end [format "%4d\t%s\n" $lc $line] oops
		} elseif {[expr $lc % 10] == 0} {
		    $t insert end [format "%4d\t%s\n" $lc $line]
		} else {
		    $t insert end "\t$line\n"
		}
	    }
            close $fid
	    $t configure -state disabled
	    pack $win -side top -fill both -expand yes

	    set half [expr $errline - [expr $stdiorows / 2 ] ]
	    $t see $half.0
	    bind $t <ButtonPress> "$t see $half.0"
        }
    }
    grab set .error
    vwait exitplease
    grab release .error
}
