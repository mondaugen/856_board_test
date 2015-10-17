#include "scheduling.h" 
#include "audio_setup.h" 
#include "synth_control.h" 
#include "wavetables.h" 
#include "poly_management.h" 
#include "signal_chain.h" 
#include "mm_common_calcs.h" 
#include <math.h> 

/* The scheduler is set up so that the 32 LSB of the time are fractional part 
 * and the 32 MSB are the integer part of a beat index. So that this works, the
 * scheduler time is incremented 0xffffffff*tempoBPM beats per minute and events
 * are scheduled eventDeltaBeats*0xffffffff ticks apart. */

struct __NoteOnEvent {
    MMEvent head;
    NoteOnEventListNode *parent;
    int active; /* 1 if active, 0 if not */
    int parameterSet; /* Which set of parameters to use */
    int numRepeats;   /* The number of times to repeat (reschedule) after
                         playing this event */
    int repeatIndex;  /* 0 means the first time this has been played, 1 means
                         the first repeat, etc. */
    MMSample currentFade; /* a value of 1 means no fade, and this decreases
                             every time if the parameterSet's fadeRate value < 1
                             */
    MMSample currentPosition; /* a value between 0 and 1 determining the
                                 starting point within the valid range of the
                                 buffer */
};

/* Event that schedules other notes to play. */
struct __NoteSchedEvent {
    MMEvent head;
    NoteSchedEventListNode *parent;
    int active;
};

MMSeq *sequence;
NoteOnEventListNode noteOnEventListHead[NUM_NOTE_PARAM_SETS];
NoteSchedEventListNode noteSchedEventListHead;

/* Keeps track of how many times a note has been played. If the number of times
 * is divisible by the note-parameter set's intermittency value, this count gets
 * reset */
int noteOnEventCount[NUM_NOTE_PARAM_SETS];

static void NoteOnEvent_happen(MMEvent *event);
static void NoteSchedEvent_happen(MMEvent *event);

void scheduler_setup(void)
{
    sequence = MMSeq_new();
    MMSeq_init(sequence, 0);
    int n;
    for (n = 0; n < NUM_NOTE_PARAM_SETS; n++) {
        noteOnEventCount[n] = 0;
        /* Initialize first list member as if it were a list item because the
         * program should never call on its child. Nodes whose children are
         * called are appended to this list node. */
        MMDLList_init(&noteOnEventListHead[n]);
    }
    MMDLList_init(&noteSchedEventListHead);
}

NoteOnEvent *NoteOnEvent_new(int active,
        int parameterSet,
        int numRepeats,
        int repeatIndex,
        MMSample currentFade,
        MMSample currentPosition)
{
    NoteOnEvent *ev = (NoteOnEvent*)malloc(sizeof(NoteOnEvent));
    if (!ev) {
        return NULL;
    }
    ((MMEvent*)ev)->happen = NoteOnEvent_happen;
    ev->active = active;
    ev->parameterSet = parameterSet;
    ev->numRepeats = numRepeats;
    ev->repeatIndex = repeatIndex;
    ev->currentFade = currentFade;
    ev->currentPosition = currentPosition;
    return ev;
}

NoteSchedEvent *NoteSchedEvent_new(int active)
{
    NoteSchedEvent *ev = (NoteSchedEvent*)malloc(sizeof(NoteSchedEvent));
    if (!ev) {
        return NULL;
    }
    ((MMEvent*)ev)->happen = NoteSchedEvent_happen;
    ev->active = active;
    return ev;
}

void schedule_noteOn_event(uint64_t timeFromNow, NoteOnEvent *ev)
{
    if (!ev) {
        return;
    }
    ev->parent = (NoteOnEventListNode*)malloc(sizeof(NoteOnEventListNode));
    if (!ev->parent) {
        free(ev);
        return;
    }
    ev->parent->child = ev;
    MMDLList_insertAfter((MMDLList*)&noteOnEventListHead[ev->parameterSet],
            (MMDLList*)ev->parent);
    MMSeq_scheduleEvent(sequence, (MMEvent*)ev,
            MMSeq_getCurrentTime(sequence) + timeFromNow);
}

void schedule_noteSched_event(uint64_t timeFromNow, NoteSchedEvent *ev)
{
    if (!ev) {
        return;
    }
    ev->parent = (NoteSchedEventListNode*)malloc(sizeof(NoteSchedEventListNode));
    if (!ev->parent) {
        free(ev);
        return;
    }
    ev->parent->child = ev;
    MMDLList_insertAfter((MMDLList*)&noteSchedEventListHead,(MMDLList*)ev->parent);
    MMSeq_scheduleEvent(sequence, (MMEvent*)ev,
            MMSeq_getCurrentTime(sequence) + timeFromNow);
}

static void NoteOnEvent_happen(MMEvent *event)
{
    /* only play if event is active */
    if (((NoteOnEvent*)event)->active == 1) {
        /* If numRepeats greater than 0, schedule the note to occur again. The
         * number of repeats should never be greater than 0 for an event of note
         * 0 */
        if (((NoteOnEvent*)event)->numRepeats > 0) {
            schedule_noteOn_event(
                    noteParamSets[((NoteOnEvent*)event)->parameterSet].eventDeltaBeats
                    * 0xffffffffULL,
                    NoteOnEvent_new(1,
                        ((NoteOnEvent*)event)->parameterSet,
                        ((NoteOnEvent*)event)->numRepeats - 1,
                        ((NoteOnEvent*)event)->repeatIndex + 1,
                        ((NoteOnEvent*)event)->currentFade 
                            * noteParamSets[((NoteOnEvent*)event)->parameterSet].fadeRate,
                        /* If stride enabled, increment the previous current
                         * position by the stride amount, wrapping between 0 and
                         * 1, otherwise just put the position as dictated by the
                         * parameter set */
                        (posMode == SynthControlPosMode_STRIDE) ?
                            MM_fwrap(((NoteOnEvent*)event)->currentPosition
                                + noteParamSets[((NoteOnEvent*)event)->parameterSet].positionStride,
                                0,1) :
                            noteParamSets[((NoteOnEvent*)event)->parameterSet].startPoint));
        }
        MMSample voiceNum = pm_get_next_free_voice_number();
        noteOnEventCount[((NoteOnEvent*)event)->parameterSet] = 0;
        if (voiceNum != -1 && 
                ((noteParamSets[((NoteOnEvent*)event)->parameterSet].amplitude
                    * ((NoteOnEvent*)event)->currentFade) > SCHEDULING_AMP_FLOOR)) { 
            /* there is a voice free */
            pm_claim_params_from_allocator((void*)&voiceAllocator,
                    (void*)&voiceNum);
            ((MMEnvedSamplePlayer*)&spsps[(int)voiceNum])->onDone =
                autorelease_on_done;
            MMTrapEnvedSamplePlayer_noteOnStruct no;
            no.note = voiceNum;
            no.amplitude =
                (noteParamSets[((NoteOnEvent*)event)->parameterSet].amplitude
                    * ((NoteOnEvent*)event)->currentFade) > 1. ? 
                    1. :
                    (noteParamSets[((NoteOnEvent*)event)->parameterSet].amplitude
                            * ((NoteOnEvent*)event)->currentFade);
            no.index = ((NoteOnEvent*)event)->currentPosition
                        * MMArray_get_length(theSound.wavtab);
            /* sustainTime is the length of the audio, times
             * noteParamSets[parameterSet].sustainTime *
             * length_of_sound_seconds * (1 -
             * noteParamSets[parameterSet].attackTime -
             * noteParamSets[parametersSet].releaseTime) */
            no.sustainTime =
                noteParamSets[((NoteOnEvent*)event)->parameterSet].sustainTime
                * (MMSample)MMArray_get_length(theSound.wavtab)
                / (MMSample)audio_hw_get_sample_rate(NULL)
                * (1.
                        - noteParamSets[((NoteOnEvent*)event)->parameterSet].attackTime
                        - noteParamSets[((NoteOnEvent*)event)->parameterSet].releaseTime);
            no.attackTime = 
                noteParamSets[((NoteOnEvent*)event)->parameterSet].sustainTime
                * (MMSample)MMArray_get_length(theSound.wavtab)
                / (MMSample)audio_hw_get_sample_rate(NULL)
                * noteParamSets[((NoteOnEvent*)event)->parameterSet].attackTime;
            no.releaseTime = 
                noteParamSets[((NoteOnEvent*)event)->parameterSet].sustainTime
                * (MMSample)MMArray_get_length(theSound.wavtab)
                / (MMSample)audio_hw_get_sample_rate(NULL)
                * noteParamSets[((NoteOnEvent*)event)->parameterSet].releaseTime;
            no.samples = theSound.wavtab;
            /* 9 is added because MMCC_et12_rate considers pitch 69 to be a note of no
             * transposition. In this we consider middle C to be a note of no
             * transposition, so we add 9 (middle C is note 60) */
            no.rate = MMCC_et12_rate(
                    noteParamSets[((NoteOnEvent*)event)->parameterSet].pitch + 9);
            MMTrapEnvedSamplePlayer_noteOn_Rate(
                    &spsps[(int)voiceNum], &no);
        }
    }
    MMDLList_remove((MMDLList*)((NoteOnEvent*)event)->parent);
    free(((NoteOnEvent*)event)->parent);
    free(event);
}

static void NoteSchedEvent_happen(MMEvent *event)
{
    if (((NoteSchedEvent*)event)->active == 1) {
        /* Schedule notes */
        int n;
        for (n = 0; n < NUM_NOTE_PARAM_SETS; n++) {
            if (noteOnEventCount[n]
                    >= noteParamSets[n].intermittency) {
                noteOnEventCount[n] = 0;
                schedule_noteOn_event(
                        noteParamSets[n].offsetBeats
                        * 0xffffffffULL,
                        NoteOnEvent_new(1,
                            n,
                            noteParamSets[n].numRepeats,
                            0,
                            1,
                            noteParamSets[n].startPoint));
            } else {
                noteOnEventCount[n] += 1;
            }
        }
        /* Schedule next noteSchedEvent using note 0's eventDeltaBeats */
        schedule_noteSched_event(noteParamSets[0].eventDeltaBeats
                * 0xffffffffULL,
                NoteSchedEvent_new(1));
        /* If scheduled recording enabled, stop the previous recording and start
         * a new one. When scheduled recording is disabled, it turns off
         * recording immediately and so it is okay that we don't turn off
         * recording from the scheduled event when schedule recording is unset
         * (== 0) */
        if (scheduleRecording == 1) {
            if (firstScheduledRecording == 0) {
                MIDI_synth_record_stop_helper((void*)&wtr);
            } else {
                /* If this is the first scheduled recording, don't stop any
                 * recording and swap the buffers because the last buffer might
                 * have garbage in it */
                firstScheduledRecording = 0;
            }
            MIDI_synth_record_start_helper((void*)&wtr);
        }
    }
    MMDLList_remove((MMDLList*)((NoteSchedEvent*)event)->parent);
    free(((NoteSchedEvent*)event)->parent);
    free(event);
}

void scheduler_incTimeAndDoEvents(void)
{
#ifdef DEBUG
    assert(sequence);
#endif
    MMSeq_incTime(sequence,(tempoBPM / 60.) 
            / ((MMSample)audio_hw_get_sample_rate(NULL) 
                / (MMSample)audio_hw_get_block_size(NULL)) * 0xffffffffULL);
    MMSeq_doAllCurrentEvents(sequence);
}

/* It could be that head is a sentinel and contains no child (as is the case
 * with noteOnEventListHead). In that case, be sure to pass the next in the
 * list, e.g., set_noteOnEvents_active(noteOnEventListHead.next) */
void set_noteOnEvents_active(NoteOnEventListNode *head)
{
    while (head) {
        head->child->active = 1;
        head = (NoteOnEventListNode*)((MMDLList*)head)->next;
    }
}

/* See set_noteOnEvents_active for tips. */
void set_noteOnEvents_inactive(NoteOnEventListNode *head)
{
    while (head) {
        head->child->active = 0;
        head = (NoteOnEventListNode*)((MMDLList*)head)->next;
    }
}

/* It could be that head is a sentinel and contains no child (as is the case
 * with noteSchedEventListHead). In that case, be sure to pass the next in the
 * list, e.g., set_noteSchedEvents_active(noteSchedEventListHead.next) */
void set_noteSchedEvents_active(NoteSchedEventListNode *head)
{
    while (head) {
        head->child->active = 1;
        head = (NoteSchedEventListNode*)((MMDLList*)head)->next;
    }
}

/* See set_noteSchedEvents_active for tips. */
void set_noteSchedEvents_inactive(NoteSchedEventListNode *head)
{
    while (head) {
        head->child->active = 0;
        head = (NoteSchedEventListNode*)((MMDLList*)head)->next;
    }
}
