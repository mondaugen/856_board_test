#include "synth_control.h"
#include "midi_setup.h" 
#include "poly_management.h" 
#include "wavetables.h" 
#include "mm_time.h" 
#include <math.h> 
#include "audio_setup.h" 
#include "scheduling.h" 
#include "quantization_tables.h" 
#include "env_map.h" 
#include "mm_mark_zerox.h" 
#include <string.h> 
#include "mm_common_calcs.h" 
#include "leds.h" 

/* Stuff that could be saved */
NoteParamSet                noteParamSets[NUM_NOTE_PARAM_SETS];
MMSample                    tempoBPM; 
SynthControlPosMode         posMode;
SynthControlDeltaButtonMode deltaButtonMode;
SynthControlPitchMode       pitchMode;
SynthControlGainMode        gainMode;
SynthControlRecMode         recMode;

/* Stuff that shouldn't really be saved */
int                         noteDeltaFromBuffer;
SynthControlEditingWhichParamsIndex editingWhichParams;
int                         currentPreset;
int                         feedbackState;
int                         scheduleRecording;
int                         firstScheduledRecording;
/* Is the scheduler on or off ? */
int                         schedulerState;
/* What preset would be recalled/stored. First preset is numbered 0. */
SynthControlPresetNumber    presetNumber;

/* Stuff that might not make it into the final application */
int16_t                     dryGain;

static void schedulerState_off_helper(void *data);
static void schedulerState_on_helper(void);

void autorelease_on_done(MMEnvedSamplePlayer * esp)
{
    pm_yield_params_to_allocator((void*)&voiceAllocator,
            (void *)&(MMEnvedSamplePlayer_getSamplePlayerSigProc(esp).note));
}

void synth_control_set_envelopeTime(float envelopeTime_param)
{
    env_map_attack_release_f(
            &noteParamSets[editingWhichParams].attackTime,
            &noteParamSets[editingWhichParams].releaseTime,
            envelopeTime_param,
            SYNTH_CONTROL_MIN_ATTACK_TIME,
            SYNTH_CONTROL_MAX_ATTACK_TIME,
            SYNTH_CONTROL_MIN_RELEASE_TIME,
            SYNTH_CONTROL_MAX_RELEASE_TIME);
}

void synth_control_envelopeTime_control(void *data_, float envelopeTime_param)
{
    synth_control_set_envelopeTime(envelopeTime_param);
}

void synth_control_set_sustainTime(float sustainTime_param)
{
    /* Sustain time is relative to length of recording, so here just 0-1.
     * It is scaled this way so that the length selection is more precise for
     * short lengths and less precise for longer ones */
    noteParamSets[editingWhichParams].sustainTime
        = powf(2.,-7.*(1 - sustainTime_param));
}

void synth_control_sustainTime_control(void *data_, float sustainTime_param)
{
    synth_control_set_sustainTime(sustainTime_param);
}

void synth_control_tempoNudge(float tempoNudge_param)
{
    /* the tempo is calculated so that 1 buffer * K is played per 1 beat where K
     * is some scalar. The value is negated so if K < 1, the tempo is slower and
     * K > 1 the tempo is faster */
    MMSample K =  1.05 - tempoNudge_param * 0.1;
    tempoBPM = 60. * (MMSample)audio_hw_get_sample_rate(NULL) 
        / ((MMSample)((MMArray*)theSound.wavtab)->length * K);
}

void synth_control_set_tempoBPM(float tempoBPM_param)
{
    tempoBPM = tempoBPM_param;
}

void synth_control_set_tempo(float tempo_param)
{
    synth_control_set_tempoBPM(40. + (240. - 40.)*tempo_param);
}

void synth_control_set_numRepeats(int numRepeats_param)
{
    noteParamSets[editingWhichParams].numRepeats = numRepeats_param;
}

void synth_control_set_repeats(float repeats_param)
{
    synth_control_set_numRepeats((int)(16. * repeats_param));        
}

void synth_control_tempoBPM_control(void *data_, float tempoBPM_param)
{
    if (editingWhichParams == 0) {
        if (noteDeltaFromBuffer == 1) {
            synth_control_tempoNudge(tempoBPM_param);
        } else {
            synth_control_set_tempo(tempoBPM_param);
        }
    } else {
        synth_control_set_repeats(tempoBPM_param);
    }
}

/* Set the pitch to any pitch between midi note 48 and 72, limited by the
 * precision of pitch_param */
void synth_control_set_pitch_chrom(float pitch_param)
{
    noteParamSets[editingWhichParams].pitch
        = 48. + (72. - 48.) * pitch_param;
}

void synth_control_set_pitch_4ths5ths(float pitch_param)
{
    static int32_t ivals[] = {-19,-17,-12,-7,-5,0,5,7,12,17,19};
    /* Half the total number of intervals that aren't 0 */
    static uint32_t n_ivals = 5; 
    pitch_param = pitch_param * 2. - 1.;
    uint32_t idx = (uint32_t)((int32_t)n_ivals
            + (int32_t)((float)n_ivals*pitch_param));
    noteParamSets[editingWhichParams].pitch
        = 60. + (float)ivals[idx];
}

void synth_control_set_pitch_arp(float pitch_param)
{
    /* Not yet capable of supporting this, just set to middle C */
    noteParamSets[editingWhichParams].pitch = 60.;
}

void synth_control_pitch_control(void *data_, float pitch_param)
{
    synth_control_set_pitch_chrom(pitch_param);
}

void synth_control_set_startPoint(float startPoint_param)
{
    noteParamSets[editingWhichParams].startPoint
        = startPoint_param;
}

void synth_control_set_positionStride(float positionStride_param)
{
            noteParamSets[editingWhichParams].positionStride
                = positionStride_param * 0.2 - 0.1;
}

void synth_control_startPoint_control(void *data_, float startPoint_param)
{
    switch (posMode) {
        case SynthControlPosMode_ABSOLUTE:
            synth_control_set_startPoint(startPoint_param);
            break;
        case SynthControlPosMode_UNKNOWN:
        case SynthControlPosMode_STRIDE:
            synth_control_set_positionStride(startPoint_param);
            break;
    }
}

void synth_control_set_eventDelta_quant(float eventDeltaBeats_param)
{
    noteParamSets[editingWhichParams].eventDeltaBeats
        = (MMSample)(1 + (int)(3. * (MMSample)eventDeltaBeats_param));
}

void synth_control_set_eventDelta_free(float eventDeltaBeats_param)
{
    noteParamSets[editingWhichParams].eventDeltaBeats
        = powf(2.,-6.*(1 - eventDeltaBeats_param));
}

void synth_control_set_intermittency(float intermittency_param)
{
    noteParamSets[editingWhichParams].intermittency =
        0 + (int)(3. * intermittency_param);
}

void synth_control_eventDeltaBeats_control(void *data_, float eventDeltaBeats_param)
{
    switch (deltaButtonMode) {
        case SynthControlDeltaButtonMode_EVENT_DELTA_QUANT:
            synth_control_set_eventDelta_quant(eventDeltaBeats_param);
        case SynthControlDeltaButtonMode_EVENT_DELTA_FREE:
            synth_control_set_eventDelta_free(eventDeltaBeats_param);
            break;
        case SynthControlDeltaButtonMode_INTERMITTENCY:
            synth_control_set_intermittency(eventDeltaBeats_param);
            break;
    }
}

void synth_control_set_offset(float offset_param)
{
    if (editingWhichParams == 0) {
        noteParamSets[editingWhichParams].offsetBeats
            = offset_param + 0.001;
    } else {
        /* The offset is relative to the total event delta of the note event
         * with parameterSet 0 */
        noteParamSets[editingWhichParams].offsetBeats
            = noteParamSets[0].eventDeltaBeats * offset_param;
    }
}

void synth_control_offsetBeats_control(void *data_, float offsetBeats_param)
{
    if (editingWhichParams == 0) {
        noteParamSets[editingWhichParams].offsetBeats
            = offsetBeats_param + 0.001;
    } else {
        /* The offset is relative to the total event delta of the note event
         * with parameterSet 0 */
        noteParamSets[editingWhichParams].offsetBeats
            = noteParamSets[0].eventDeltaBeats * offsetBeats_param;
    }
}

void synth_control_noteDeltaFromBuffer_control(void *data_,
        uint32_t noteDeltaFromBuffer_param)
{
    noteDeltaFromBuffer = (int)noteDeltaFromBuffer_param;
}

void MIDI_synth_record_stop_helper(void *data)
{
    /* Only do something if it was recording */
    if (((MMWavTabRecorder*)data)->state == MMWavTabRecorderState_STOPPED) {
        return;
    }

    /* Set the length to the index the recorder got to */
    ((MMArray*)((MMWavTabRecorder*)data)->buffer)->length =
        ((MMWavTabRecorder*)data)->currentIndex;
    ((MMWavTabRecorder*)data)->state = MMWavTabRecorderState_STOPPED;
    /* If the index the recorder got to is greater than the window length of a
     * Hann window, window the beginning and ends of the file using this window
     * */
    if (((MMWavTabRecorder*)data)->currentIndex >= hannWindowTableLength) {
        int n;
        for (n = 0; n < hannWindowTableLength/2; n++) {
            MMWavTab_get(((MMWavTabRecorder*)data)->buffer,n) *= hannWindowTable[n];
            MMWavTab_get(((MMWavTabRecorder*)data)->buffer,
                    ((MMArray*)((MMWavTabRecorder*)data)->buffer)->length - n - 1)
                *= hannWindowTable[hannWindowTableLength - n - 1];
        }
    }
    /* If the noteDeltaFromBuffer flag is set, compute the tempo from
     * the buffer length, set the playback rate to 1 and set the eventDelta
     * to 1 beat so that the recording plays once per beat and the tempo is
     * one beat per length of recording */
//    if (noteDeltaFromBuffer == 1) {
    SynthControlRecMode _recMode = synth_control_get_recMode();
    if ((_recMode == SynthControlRecMode_REC_LEN_1_BEAT)
            || (_recMode == SynthControlRecMode_REC_LEN_1_BEAT_REC_SCHED)) {
        /* When REC_LEN_1_BEAT* flag set
         *  - the tempo is adjusted so the recording plays in the time of one
         *    beat.
         *
         * If REC_LEN_1_BEAT* flag is set and the feedback is enabled,
         * then:
         *  - the sustain time is made to be 1
         *  - the the amplitudes of notes that aren't the 0th are set to 0
         *  - the pitch of note 0 is set to unison (60 so that rate is 1)
         *  - the delta time of note 0 is set to 1 beat
         *  - the intermittence of note 0 is set to 0
         *  - the offset of note 0 is set to 0
         *
         * The scheduler is (re) started immediately in this case.
         */
        tempoBPM = 60. * (MMSample)audio_hw_get_sample_rate(NULL) 
            / (MMSample)((MMArray*)((MMWavTabRecorder*)data)->buffer)->length;
        if (feedbackState == 1) {
            int n;
            noteParamSets[0].eventDeltaBeats = 1;
            noteParamSets[0].pitch = 60.;
            for (n = 1; n < NUM_NOTE_PARAM_SETS; n++) {
                noteParamSets[n].amplitude = 0;
            }
            noteParamSets[0].sustainTime = 1.;
            noteParamSets[0].intermittency = 0;
            noteParamSets[0].offsetBeats = 0;
            noteParamSets[0].startPoint = 0;
        }
        if (schedulerState == 1) {
            schedulerState_off_helper((void*)noteOnEventListHead);
        }
        schedulerState_on_helper();
    }
    /* Swap the playing and the recording sounds */
    WavTabAreaPair tmp = recordingSound;
    recordingSound = theSound;
    theSound = tmp;
}

void MIDI_synth_record_start_helper(void *data)
{
    /* Set to max length so it would be possible to record all the way to
     * the end of allocated space */
    ((MMArray*)recordingSound.wavtab)->length = soundSampleMaxLength;
    /* Set the area it is pointing to to the beginning of the allocated space */
    ((MMArray*)recordingSound.wavtab)->data = recordingSound.area;
    ((MMWavTabRecorder*)data)->buffer = recordingSound.wavtab;
    ((MMWavTabRecorder*)data)->currentIndex = 0;
    ((MMWavTabRecorder*)data)->state = MMWavTabRecorderState_RECORDING;
}

/* Start recording with non-zero control change value. Stop with value of 0. */
void synth_control_record_trig(void *data_, uint32_t record_param)
{
    if (record_param > 0) {
        /* Only allow recording if recording is not scheduled */
        if (scheduleRecording == 0) {
            MIDI_synth_record_start_helper(&wtr);
        }
    } else if (wtr.state == MMWavTabRecorderState_STOPPED) {
        MIDI_synth_record_stop_helper(&wtr);
    }
}

/* Starts recording if recording is stopped, stops recording if recording going
 * on */
void synth_control_record_tog(void)
{
    if (wtr.state == MMWavTabRecorderState_RECORDING) {
        MIDI_synth_record_stop_helper(&wtr);
    } else if (wtr.state == MMWavTabRecorderState_STOPPED) {
        /* Only allow recording if recording is not scheduled */
        if (scheduleRecording == 0) {
            MIDI_synth_record_start_helper(&wtr);
        }
    }
}

void synth_control_feedback_control(void *data_, uint32_t feedback_param)
{
    if (feedback_param > 0) {
        /* Move fbBusSplitter to onNode */
        MMSigProc_insertAfter(fbOnNode,&fbBusSplitter);
        feedbackState = 1;
    } else {
        /* Move fbBusSplitter to offNode */
        MMSigProc_remove(&fbBusSplitter);
        /* Zero the feedback bus */
        memset(fbBusSplitter.destBus->data,0,
                sizeof(MMSample)
                * fbBusSplitter.destBus->size
                * fbBusSplitter.destBus->channels);
        feedbackState = 0;
    }
}

void synth_control_feedback_tog(void)
{
    if (feedbackState) {
        synth_control_feedback_control(NULL,0);
    } else {
        synth_control_feedback_control(NULL,1);
    }
}

void synth_control_dryGain_control(void *data_, float dryGain_param)
{
    dryGain = (int16_t)(dryGain_param * 127.);
}

static void free_playing_spsp_voice(void *voice_number)
{
    MMEnvelope_startRelease(
            ((MMEnvedSamplePlayer*)&spsps[*((int*)voice_number)])->envelope);
}

/* Pass a pointer to the first NoteOnEventListNode */
static void schedulerState_off_helper(void *data)
{
    int n;
    for (n = 0; n < NUM_NOTE_PARAM_SETS; n++) {
        /* Disactivate all events of all parameter sets */
        set_noteOnEvents_inactive(
                (NoteOnEventListNode*)MMDLList_getNext(
                    &(((NoteOnEventListNode*)data)[n])));
        /* Reset the note on event counts */
        noteOnEventCount[n] = 0;
    }
    /* Disactivate the noteSchedEvents */
    set_noteSchedEvents_inactive(
            (NoteSchedEventListNode*)MMDLList_getNext(&noteSchedEventListHead));
    /* Turn off all playing notes */
    pm_do_for_each_busy_voice(&voiceAllocator,free_playing_spsp_voice);
    if (scheduleRecording == 1) {
        /* Discard what was last recorded */
        wtr.state = MMWavTabRecorderState_STOPPED;
    }
    schedulerState = 0;
}

static void schedulerState_on_helper(void)
{
    /* schedule 1st event which is initially active */
    schedule_noteSched_event(0, NoteSchedEvent_new(1));
    schedulerState = 1;
}

void synth_control_schedulerState_control(void *data_, uint32_t schedulerState_param)
{
    if (schedulerState_param > 0) {
        schedulerState_on_helper();
    } else {
        schedulerState_off_helper(&noteOnEventListHead);
    }
}

void synth_control_schedulerState_tog(void)
{
    if (schedulerState) {
        schedulerState_off_helper(&noteOnEventListHead);
    } else {
        schedulerState_on_helper();
    }
}

void synth_control_editingWhichParams_control(void *data_,
        uint32_t editingWhichParams_param)
{
    if (editingWhichParams_param >= NUM_NOTE_PARAM_SETS) {
            editingWhichParams_param = NUM_NOTE_PARAM_SETS - 1;
    } 
    editingWhichParams = editingWhichParams_param;
}

void synth_control_deltaButtonMode_control(void *data_,
        uint32_t deltaButtonMode_param)
{
    deltaButtonMode =
        (SynthControlDeltaButtonMode)deltaButtonMode_param;
}

void synth_control_recordScheduling_control(void *data_,
        uint32_t recordScheduling_param)
{
    if (recordScheduling_param) {
        /* If recording in progress, stop it */
        if (wtr.state == MMWavTabRecorderState_RECORDING) {
            wtr.state = MMWavTabRecorderState_STOPPED;
/*             MIDI_synth_record_stop_helper((void*)&wtr); */
        }
        scheduleRecording = 1;
        /* Set first scheduled recording to true so that when the first
         * scheduled recording happens, the buffers aren't swapped. This is
         * because the buffer it swaps with might contain garbage. */
        firstScheduledRecording = 1;
    } else {
        scheduleRecording = 0;
        /* Stop recording (it will most likely be in progress) but don't swap
         * the recording and playing sounds. We discard the most recent
         * recording to give the user time to flip the switch if they like the
         * previous recording */
        wtr.state = MMWavTabRecorderState_STOPPED;
    }
}

void synth_control_gainMode_control(void *data_,
        uint32_t gainMode_param)
{
    gainMode = (SynthControlGainMode)gainMode_param;
}

void synth_control_set_wet(float gain_param)
{
    /* Gain in dB */
    gain_param = -1 * (96 - 96. * gain_param);
    if (gain_param < -80.) {
        noteParamSets[editingWhichParams].amplitude = 0.;
    } else {
        noteParamSets[editingWhichParams].amplitude =
            powf(10.,gain_param / 10.);
    }
}

void synth_control_set_fade(float gain_param)
{
    /* To control the fade rate via "number of repeats before note is below
     * -60dB" the following formula can be employed, assuming the original
     *  amplitude is 1.
     *  pow(10^-6,1./n_repeats)
     *  
     *  This should be optimized using linearly interpolated table lookup for
     *  select values./
     */
    noteParamSets[editingWhichParams].fadeRate
        = gain_param * 2.;
}

void synth_control_gain_control(void *data_, float gain_param)
{
    switch (gainMode) {
        case SynthControlGainMode_WET:
            synth_control_set_wet(gain_param);
            break;
        case SynthControlGainMode_FADE:
            synth_control_set_fade(gain_param);
            break;
    }
}

void synth_control_posMode_control(void *data_, uint32_t posMode_param)
{
    posMode = (SynthControlPosMode)posMode_param;
}

void synth_control_presetNumber_control(void *data_, uint32_t presetNumber_param)
{
    if (presetNumber_param >= NUM_SYNTH_CONTROL_PRESETS) {
        presetNumber_param = NUM_SYNTH_CONTROL_PRESETS - 1;
    }
    presetNumber = (int)(presetNumber_param);
}

void synth_control_presetStore_control(void *data_, uint32_t presetStore_param)
{
    sc_presets_store(presetNumber);
}

void synth_control_presetRecall_control(void *data_, uint32_t presetRecall_param)
{
    sc_presets_recall(presetNumber);
}

void synth_control_presetStore_tog(void)
{
    sc_presets_store(presetNumber);
}

void synth_control_presetRecall_tog(void)
{
    sc_presets_recall(presetNumber);
}

void synth_control_setup(void)
{
    noteParamSets[0] = (NoteParamSet) {
        .attackTime = 0.01,     /* attackTime */
        .sustainTime  = 1,      /* sustainTime */
        .releaseTime = 0.01,    /* releaseTime */
        .eventDeltaBeats = 1,   /* eventDeltaBeats */
        .pitch = 60,            /* pitch */
        .amplitude = .5,        /* amplitude */
        .startPoint = 0,        /* startPoint */
        .numRepeats = 0,        /* The number of times repeated */
        .offsetBeats = 0,       /* The amount of beats offset from the beginning
                                   of the bar */
        .intermittency = 0,      /* Canonically the number of repeats that are
                                    ignored */
        .fadeRate      = 0,      /* Fade rate doesn't apply to the first
                                    parameter set */
        .positionStride = 0      /* The amount the starting point in the sample
                                    is advanced each time it is scheduled (if
                                    stride enabled) */
    };
    int n;
    for (n = 1; n < NUM_NOTE_PARAM_SETS; n++) {
        noteParamSets[n] = (NoteParamSet) {
            .attackTime = 0.01,     /* attackTime */
            .sustainTime  = 1,      /* sustainTime */
            .releaseTime = 0.01,    /* releaseTime */
            .eventDeltaBeats = 1,   /* eventDeltaBeats */
            .pitch = 60,            /* pitch */
            .amplitude = 0,         /* amplitude */
            .startPoint = 0,        /* startPoint */
            .numRepeats = 0,        /* The number of times repeated */
            .offsetBeats = 0,       /* The amount of beats offset from the
                                       beginning of the bar */
            .intermittency = 0,      /* Canonically the number of repeats that
                                        are ignored */
            .fadeRate      = 1,      /* Default fade rate of 1 means no fade */
            .positionStride = 0      /* The amount the starting point in the
                                        sample is advanced each time it is
                                        scheduled (if stride enabled) */
        };
    }
    noteDeltaFromBuffer = 0;
    dryGain             = 0;
    editingWhichParams  = 0;
    tempoBPM            = 120;
    posMode             = SynthControlPosMode_ABSOLUTE;
    deltaButtonMode     = SynthControlDeltaButtonMode_EVENT_DELTA_FREE;
    recMode             = SynthControlRecMode_NORMAL;
    pitchMode           = SynthControlPitchMode_CHROM;
    feedbackState       = 0;
    scheduleRecording   = 0;
    schedulerState      = 0;
    /* The recorder trigger requires the zero crossing search be initialized */
    HannWindowTable_init(REC_LOOP_FADE_TIME_S * 2.);
}

SynthControlEditingWhichParamsIndex synth_control_get_editingWhichParams(void)
{
    return editingWhichParams;
}

void synth_control_set_editingWhichParams(
        SynthControlEditingWhichParamsIndex editingWhichParams_param)
{
    if (editingWhichParams_param >= NUM_NOTE_PARAM_SETS) {
            editingWhichParams_param = NUM_NOTE_PARAM_SETS - 1;
    } 
    editingWhichParams = editingWhichParams_param;
}

void synth_control_set_deltaButtonMode(SynthControlDeltaButtonMode 
        deltaButtonMode_param)
{
    deltaButtonMode = deltaButtonMode_param;
}

SynthControlDeltaButtonMode synth_control_get_deltaButtonMode(void)
{
    return deltaButtonMode;
}

void synth_control_set_recMode(SynthControlRecMode recMode_param)
{
    recMode = recMode_param;
}

SynthControlRecMode synth_control_get_recMode(void)
{
    return recMode;
}

void synth_control_set_pitchMode(SynthControlPitchMode pitchMode_param)
{
    pitchMode = pitchMode_param;
}

SynthControlPitchMode synth_control_get_pitchMode(void)
{
    return pitchMode;
}

void synth_control_set_posMode(SynthControlPosMode posMode_param)
{
    posMode = (SynthControlPosMode)posMode_param;
}

SynthControlPosMode synth_control_get_posMode(void)
{
    return posMode;
}

void synth_control_set_gainMode(SynthControlGainMode gainMode_param)
{
    gainMode = (SynthControlGainMode)gainMode_param;
}

SynthControlGainMode synth_control_get_gainMode(void)
{
    return gainMode;
}

void synth_control_set_presetNumber(SynthControlPresetNumber presetNumber_param)
{
    if (presetNumber_param >= NUM_SYNTH_CONTROL_PRESETS) {
        presetNumber_param = NUM_SYNTH_CONTROL_PRESETS - 1;
    }
    presetNumber = presetNumber_param;
}

SynthControlPresetNumber synth_control_get_presetNumber(void)
{
    return presetNumber;
}

int synth_control_get_noteDeltaFromBuffer(void)
{
    return noteDeltaFromBuffer;
}

/* Returns 1 if currently recording, 0 otherwise */
int synth_control_get_recordState(void)
{
    switch (wtr.state) {
        case MMWavTabRecorderState_RECORDING:
            return 1;
    }
    return 0;
}

/* Returns 1 if scheduler play, 0 otherwise */
int synth_control_get_schedulerState(void)
{
    return schedulerState;
}

/* Returns 1 if feedback enable, 0 otherwise */
int synth_control_get_feedbackState(void)
{
    return feedbackState;
}
