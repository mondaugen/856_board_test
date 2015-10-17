# How I converted some of the functions of the old synth_control.c 
# to midi_control.c
BEGIN {
    param="param"
}
{
    if ($0 ~ /MIDIMsg_free\(msg\)/) {
        # print nothing
    } else if (($1 == "void") && ($2 ~ /MIDI_synth_cc/)) {
        tmp = $0
        split($2,strings,"_")
        param=strings[4]
        sub(/MIDI_synth_cc/,"synth_control",tmp)
        sub(/MIDIMsg \*msg/,sprintf("uint32_t %s_param",param),tmp)
        print tmp
    } else {
        gsub(/msg->data\[2\]/,sprintf("%s_param",param));
        print $0
    }
}
