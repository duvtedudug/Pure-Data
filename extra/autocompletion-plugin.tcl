# META NAME auto-completion plugin
# META DESCRIPTION Does auto-completion for objects
# META AUTHOR <Yvan Volochine> yvan.volochine@gmail.com
# META VERSION 0.3

# LIST OF CHANGES:
# - bugfix: nameclash with tkcanvas popup (thanks Scott McCoid for spotting)
# - added popup menu (!!!) (thanks Hans-Christoph Steiner for insisting =)
# - fixed list objects (thanks Johathan Wilkes)
# - added ::completion namespace (thanks Hans-Christoph Steiner)

package require Tcl 8.4

namespace eval ::completion:: {}

###########################################################
# overwritten procedures
rename ::pd_bindings::sendkey ::pd_bindings::sendkey_old
rename pdtk_text_set pdtk_text_set_old
rename pdtk_text_editing pdtk_text_editing_old
rename ::dialog_font::ok ::dialog_font::ok_old

###########################################################
# GLOBALS

# this is where you can put extra objects/abstractions that you want to
# work with auto-completion. BEWARE, you should have *only one* object/abstraction
# name per line! if your patch/abstraction name has spaces in its name, you *have to*
# put the name into quotes: "my cool abstraction"
set ::user_objects_list "~/pd/list_of_my_objects.txt"
#set ::user_objects_list ""

set ::new_object false
set ::current_canvas ""
set ::current_tag ""
set ::lock_motion false
set ::editx 0
set ::edity 0
set ::font_size 10
set ::current_text ""
set ::first_text ""
set ::erase_text ""
set ::i 0
set ::cycle false
set ::completions {}
# all pd internals (hopefully)
set ::all_externals {random loadbang namecanvas serial cputime realtime \
canvas declare template curve plot drawnumber vradio declare mtof ftom \
rmstodb powtodb dbtopow dbtorms max~ min~ delwrite~ delread~ vd~ inlet~ \
outlet~ block samplerate~ inlet midiin sysexin notein ctlin pgmin bendin \
touchin polytouchin midiclkin midirealtimein midiout noteout ctlout pgmout \
bendout touchout polytouchout makenote stripnote poly bag "list append" \
"list prepend" "list split" "list trim" "list length" hradio print text \
cos~ osc~ vcf~ noise~ hslider hip~ lop~ bp~ biquad~ samphold~ rpole~ rzero~ \
rzero_rev~ cpole~ czero~ czero_rev~ dac~ adc~ sig~ line~ vline~ snapshot~ \
vsnapshot~ env~ threshold~ bang~ fft~ ifft~ rfft~ rifft~ framp~ qlist \
textfile openpanel savepanel key keyup keyname int float symbol bang send \
receive sel route pack unpack trigger spigot moses until makefilename swap \
change value bng pow max min mod div sin cos tan atan atan2 sqrt log exp \
abs wrap clip toggle soundfiler readsf writesf tabwrite~ tabplay~ tabread~ \
tabread4~ tabosc4~ tabsend tabreceive tabread tabread4 tabwrite send~ \
receive~ catch~ throw~ get set getsize setsize append sublist netsend \
netreceive nbx vslider clip~ rsqrt~ sqrt~ wrap~ mtof~ ftom~ dbtorms~ \
rmstodb~ dbtopow~ powtodb~ pow~ exp~ log~ abs~ text vu delay metro line \
timer pipe list}



proc ::completion::init {} {
    # bind Tab for autocompletion
    bind all <Tab> {::completion::trigger}
    ::completion::list_user_externals
    ::completion::list_user_objects $::user_objects_list
    # sort objects list for a quicker search later
    set ::all_externals [lsort $::all_externals]
}

proc ::completion::trigger {} {
    if {$::new_object} {
        # remove whitespaces
        set ::current_text [string map {" " ""} $::current_text]
        ::completion::find_external
    }
}

proc ::completion::list_user_externals {} {
    foreach pathdir [concat $::sys_searchpath $::sys_staticpath] {
        set dir [file normalize $pathdir]
        if { ! [file isdirectory $dir]} {continue}
        foreach filename [glob -directory $dir -nocomplain -types {f} -- \
                              *.pd_linux] {
            set basename [file tail $filename]
            set name [file rootname $basename]
            lappend ::all_externals $name
        }
    }
}

proc ::completion::list_user_objects {afile} {
    set filename [file join $afile]
    if {
        $afile ne ""
        && [file exists $filename]
        && [file readable $filename]
    } {
        set fl [open $filename r]
        while {[gets $fl line] >= 0} {
            if {[string index $line 0] ne "#"} {
                lappend ::all_externals $line
            }
        }
        close $fl
    }
}

proc ::completion::find_external {} {
    set length [llength $::completions]
    set text $::current_text
    
    if {$text ne "" && $::cycle == false} {
        set ::completions [lsearch -all -inline -glob $::all_externals $text*]
        # to retrieve typed text after cycling through all possible completions
        set ::first_text $::current_text
        set ::cycle true
        set length [llength $::completions]
        set ::i 0
    }

    if {$::cycle && $length > 0} {
        if {$::i == $length} {
            # return to what user first typed
            set new_text $::first_text
            set ::i -1
        } {
            set new_text [lindex $::completions $::i]
        }
        if {$::i == 0} { set ::erase_text $text }
        # if there is one result, write it
        # otherwise, bring the popup menu
        if {$length == 1} {
            ::completion::write_text $new_text
        } {
            ::completion::popup
        }
        set ::i [expr $::i + 1]
    }
}

# experimental (for testing)
proc ::completion::popup {} {
    set mytoplevel [winfo toplevel $::current_canvas]
    set geom [wm geometry $mytoplevel]
    regexp -- {([0-9]+)x([0-9]+)\+([0-9]+)\+([0-9]+)} $geom -> \
          width height decoLeft decoTop
    set left [expr $decoLeft + $::editx]
    set top [expr $decoTop + $::edity]
    # popup menu
    catch { destroy .completion_popup }
    menu .completion_popup -tearoff 0
    foreach name $::completions {
        .completion_popup add command -label $name -command "::completion::write_text $name"
    }
    tk_popup .completion_popup $left $top
}

# simulate backspace keys
proc ::completion::erase_previoustext {} {
    set mytoplevel [winfo toplevel $::current_canvas]
    set i [string length $::erase_text]
    while {$i > 0} {
        pdsend "$mytoplevel key 1 8 0"
        pdsend "$mytoplevel key 0 8 0"
        set i [expr {$i - 1}]
    }
}

# write text into the object box
proc ::completion::write_text {args} {
    ::completion::erase_previoustext
    set text ""
    # in case of spaces
    foreach arg $args { set text [concat $text $arg] }
    # write letters (!)
    for {set i 0} {$i < [string length $text]} {incr i 1} {
        set cha [string index $text $i]
        scan $cha %c keynum
        pdsend "pd key 1 $keynum 0"
    }
    # to be able to erase it later
    set ::erase_text $text
}

###########################################################
# overwritten

# I overwrite this just to be able to reset the cycle in auto-completion
proc ::pd_bindings::sendkey {window state key iso shift} {
    switch -- $key {
        "BackSpace" { set iso ""; set key 8 }
        "Tab"       { set iso ""; set key 9 }
        "Return"    { set iso ""; set key 10 }
        "Escape"    { set iso ""; set key 27 }
        "Space"     { set iso ""; set key 32 }
        "Delete"    { set iso ""; set key 127 }
        "KP_Delete" { set iso ""; set key 127 }
    }
    if {$iso ne ""} {
        scan $iso %c key
    }
    if {! [winfo exists $window]} {return}
    set mytoplevel [winfo toplevel $window]
    if {[winfo class $mytoplevel] eq "PatchWindow"} {
        pdsend "$mytoplevel key $state $key $shift"
        # auto-completion
        # something was typed in so reset $::cycle
        if {$key != 9} {set ::cycle false}
    }
}

# change the text in an existing text box
proc pdtk_text_set {tkcanvas tag text} {
    $tkcanvas itemconfig $tag -text $text
    # auto-completion: store typed text
    set ::current_text $text
    set ::current_canvas $tkcanvas
    set ::current_tag $tag
}

# not used anymore
proc pdtk_text_editing_OLD {mytoplevel tag editing} {
    set tkcanvas [tkcanvas_name $mytoplevel]
    if {$editing == 0} {
        selection clear $tkcanvas
        # auto-completion
        set ::completions {}
        set ::new_object false
        set ::lock_motion false
        set ::cycle false
    } {
        set ::editingtext($mytoplevel) $editing
        # auto-completion
        set ::new_object $editing
    }
    $tkcanvas focus $tag
}

# thanks Hans-Christoph Steiner
proc pdtk_text_editing {mytoplevel tag editing} {
    set tkcanvas [tkcanvas_name $mytoplevel]
    set rectcoords [$tkcanvas coords ${tag}R]
    if {$rectcoords ne ""} {
        set ::editx [expr int([lindex $rectcoords 0])]
        set ::edity [expr int([lindex $rectcoords 1]) + ($::font_size * 7)]
    }
    if {$editing == 0} {
        selection clear $tkcanvas
        # auto-completion
        set ::completions {}
        set ::new_object false
        set ::lock_motion false
        set ::cycle false
    } {
        set ::editingtext($mytoplevel) $editing
        # auto-completion
        set ::new_object $editing
    }
    $tkcanvas focus $tag
}

proc ::dialog_font::ok {gfxstub} {
    variable fontsize
    apply $gfxstub $fontsize
    cancel $gfxstub
    # auto-completion
    set $::font_size $fontsize
}

###########################################################
# main

::completion::init

pdtk_post "loaded: autocompletion-plugin 0.3\n"
