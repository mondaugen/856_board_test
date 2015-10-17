#ifndef SCHEDULING_H
#define SCHEDULING_H 
#include "mm_seq.h"
#include "mm_dllist.h" 
#include "mm_sample.h" 

/* Amplitude below which playback is not triggered */
#define SCHEDULING_AMP_FLOOR 3.05E-5 /* ~ 2^-15 */

typedef struct __NoteOnEvent NoteOnEvent;
typedef struct __NoteSchedEvent NoteSchedEvent;
typedef struct __NoteOnEventListNode NoteOnEventListNode;
typedef struct __NoteSchedEventListNode NoteSchedEventListNode;

struct __NoteOnEventListNode {
    MMDLList head;
    NoteOnEvent *child;
};

struct __NoteSchedEventListNode {
    MMDLList head;
    NoteSchedEvent *child;
};

extern MMSeq *sequence;
extern NoteOnEventListNode noteOnEventListHead[];
extern NoteSchedEventListNode noteSchedEventListHead;
extern int noteOnEventCount[];
void scheduler_setup(void);
void schedule_noteOn_event(MMTime timeFromNow, NoteOnEvent *ev);
void scheduler_incTimeAndDoEvents(void);
void set_noteOnEvents_active(NoteOnEventListNode *head);
void set_noteOnEvents_inactive(NoteOnEventListNode *head);
void set_noteSchedEvents_active(NoteSchedEventListNode *head);
void set_noteSchedEvents_inactive(NoteSchedEventListNode *head);
void schedule_noteSched_event(uint64_t timeFromNow, NoteSchedEvent *ev);
NoteSchedEvent *NoteSchedEvent_new(int active);
NoteOnEvent *NoteOnEvent_new(int active,
        int parameterSet,
        int numRepeats,
        int repeatIndex,
        MMSample currentFade,
        MMSample currentPosition);

#endif /* SCHEDULING_H */
