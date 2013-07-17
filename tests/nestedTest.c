/* 
 * Copyright (c) 2013 Andreas Misje
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "stateMachine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This simple test uses a multiple-nested state machine to test that
 * traversing parents anc children work correctly.
 *
 *     +--+         o
 *     |  v         |
 * +---|------[9]---|----------+
 * |   |            v          |
 * |   |   o      +---+   (b)  |  +---+
 * |   |   |      | 1 |<----------| 2 |<---+
 * |   |   |      +---+        |  +---+<-+ |
 * |   |   |        |(d)       |         | |
 * | +-|---|--[10]--|--------+ |         | |
 * | | |   |        v        | |      (f)| |(g)
 * | | o   |      +---+<----------+      | |
 * | |     |      | 3 |-+    | |  |(a)   | |
 * | |     |      +---+ |(e) | +--+      | |
 * | |     |            v    | |         | |
 * | | +---|----[11]-------+ +-----------+ |
 * | | |   v             o +---------------+
 * | | | +---+ (h)+---+  | | | |
 * | | | | 4 |--->| 5 |<-+ | | |  +---+
 * | | | +---+    +---+    | | |  | 6 |
 * | | |   |(j)     |      | | |  +---+
 * | | |   |        |      | | |    ^
 * | | +---|--------|------+ +------+(i)
 * | +-----|--------|--------+ |
 * +-------|-----^--|----------+           +---+
 *      ^  |     |  |                      | E |
 *      +--+     +--+                      +---+
 */

enum eventTypes
{
   Event_dummy,
};

/* Use this struct as event payload, containing both the event data (a single
 * character) and the name of the expected new state (used to ensure correct
 * behaviour).  */
struct eventPayload
{
   char data;
   const char *expectedState;
};

static void entryAction( void *stateData, struct event *event );
static void exitAction( void *stateData, struct event *event );
static void transAction( void *oldStateData, struct event *event,
      void *newStateData );
static bool guard( void *condition, struct event *event );

static struct state s1, s2, s3, s4, s5, s6, s9, s10, s11, sE;

static struct state

s1 =
{
   .data = "1",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'d', &guard, &transAction, &s3 },
   },
   .numTransitions = 1,
   .parentState = &s9,
},

   s2 =
{
   .data = "2",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'b', &guard, &transAction, &s1 },
   },
   .numTransitions = 1,
},

   s3 =
{
   .data = "3",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'e', &guard, &transAction, &s11 },
   },
   .numTransitions = 1,
   .parentState = &s10,
},

   s4 =
{
   .data = "4",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'h', &guard, &transAction, &s5 },
      { Event_dummy, (void *)(intptr_t)'j', &guard, &transAction, &s9 },
   },
   .numTransitions = 2,
   .parentState = &s11,
},

   s5 =
{
   .data = "5",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .transitions = (struct transition[]) {
      /* Use an conditionless transition: */
      { Event_dummy, NULL, NULL, &transAction, &s10 },
   },
   .numTransitions = 1,
   .parentState = &s11,
},

   s6 =
{
   .data = "6",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
},

   s9 =
{
   .data = "9",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .entryState = &s4,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'a', &guard, &transAction, &s3 },
   },
   .numTransitions = 1,
},

   s10 =
{
   .data = "10",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .entryState = &s9,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'f', &guard, &transAction, &s2 },
      { Event_dummy, (void *)(intptr_t)'i', &guard, &transAction, &s6 },
   },
   .numTransitions = 2,
   .parentState = &s9,
},

   s11 =
{
   .data = "11",
   .entryAction = &entryAction,
   .exitAction = &exitAction,
   .entryState = &s5,
   .transitions = (struct transition[]) {
      { Event_dummy, (void *)(intptr_t)'g', &guard, &transAction, &s2 },
   },
   .numTransitions = 1,
   .parentState = &s10,
},

   sE =
{
   .data = "ERROR",
   .entryAction = &entryAction,
};

int main()
{
   struct stateMachine fsm;
   stateM_init( &fsm, &s1, &sE );

   struct event events[] = {
      /* Create transitions, with the single character as triggering event
       * data, and the expected new state name as the following string. '*' is
       * used when the unconditional transition will be followed. */
      { Event_dummy, &(struct eventPayload){ 'd', "3" } },
      { Event_dummy, &(struct eventPayload){ 'e', "5" } },
      { Event_dummy, &(struct eventPayload){ '*', "4" } },
      { Event_dummy, &(struct eventPayload){ 'j', "4" } },
      { Event_dummy, &(struct eventPayload){ 'g', "2" } },
      { Event_dummy, &(struct eventPayload){ 'b', "1" } },
      { Event_dummy, &(struct eventPayload){ 'd', "3" } },
      { Event_dummy, &(struct eventPayload){ 'e', "5" } },
      { Event_dummy, &(struct eventPayload){ 'k', "4" } },
      { Event_dummy, &(struct eventPayload){ 'h', "5" } },
      { Event_dummy, &(struct eventPayload){ '*', "4" } },
      { Event_dummy, &(struct eventPayload){ 'f', "2" } },
      { Event_dummy, &(struct eventPayload){ 'b', "1" } },
      { Event_dummy, &(struct eventPayload){ 'a', "3" } },
      { Event_dummy, &(struct eventPayload){ 'f', "2" } },
      { Event_dummy, &(struct eventPayload){ 'b', "1" } },
      { Event_dummy, &(struct eventPayload){ 'd', "3" } },
      { Event_dummy, &(struct eventPayload){ 'i', "6" } },
   };

   int res;
   size_t i;

   /* Hand all but the last event to the state machine: */
   for ( i = 0; i < sizeof( events ) / sizeof( events[ 0 ] ) - 1; ++i )
   {
      res = stateM_handleEvent( &fsm, &events[ i ] );
      if ( res == stateM_stateLoopSelf )
      {
         /* Prevent segmentation faults (due to the following comparison)
          * (loops will not be tested in the first transition): */
         if ( i == 0 )
         {
            fputs( "Internal error. This should not happen.\n", stderr );
            exit( 4 );
         }

         /* Ensure that the reported state loop is indeed a state loop (check
          * that the expected state is the same as the previous expected
          * state): */
         if ( !strcmp( ( (struct eventPayload *)events[ i ].data
                     )->expectedState, ((struct eventPayload *)events[ i - 1 ]
                        .data )->expectedState ) )
            puts( "State changed back to itself" );
         else
         {
            fputs( "State unexpectedly changed back to itself", stderr );
            exit( 5 );
         }
      }
      /* Apart from an occasional state loop, all other events handed to the
       * state machine should result in 'stateM_stateChanged': */
      else if ( res != stateM_stateChanged )
      {
         fprintf( stderr, "Unexpected return value from stateM_handleEvent:"
               " %d\n", res );
         exit( 2 );
      }
   }

   /* The last state change is expected to result in a transition to a final
    * state: */
   res = stateM_handleEvent( &fsm, &events[ i ] );
   if ( res != stateM_finalStateReached )
   {
      fprintf( stderr, "Unexpected return value from stateM_handleEvent:"
            " %d\n", res );
      exit( 3 );
   }
   puts( "A final state was reached (as expected)" );

   return 0;
}

static void entryAction( void *stateData, struct event *event )
{
   const char *stateName = (const char *)stateData;

   printf( "Entering %s\n", stateName );
}

static void exitAction( void *stateData, struct event *event )
{
   const char *stateName = (const char *)stateData;

   printf( "Exiting %s\n", stateName );
}

static void transAction( void *oldStateData, struct event *event,
      void *newStateData )
{
   struct eventPayload *eventData = (struct eventPayload *)event->data;

   printf( "Event '%c'\n", eventData->data );

   if ( strcmp( ( (const char *)newStateData ), eventData->expectedState ) )
   {
      fprintf( stderr, "Unexpected state transition (to %s)\n",
            (const char *)newStateData );
      exit( 1 );
   }
}

static bool guard( void *condition, struct event *event )
{
   struct eventPayload *eventData = (struct eventPayload *)event->data;

   return (intptr_t )condition == (intptr_t)eventData->data;
}
