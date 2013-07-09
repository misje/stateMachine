/* 
 * Copyright (C) 2013 Andreas Misje
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include "stateMachine.h"

/* This simple example checks keyboad input against the two allowed strings
 * "ha\n" and "hi\n". If an unrecognised character is read, a group state will
 * handle this by printing a message and returning to the idle state. If the
 * character '!' is encountered, a "reset" message is printed, and the group
 * state's entry state will be entered (the idle state). 
 *
 *                   print 'reset'
 *       o      +---------------------+
 *       |      |                     | '!'
 *       |      v     group state     |
 * +-----v-----------------------------------+----+
 * |  +------+  'h'  +---+  'a'  +---+  '\n'      |
 * +->| idle | ----> | h | ----> | a | ---------+ |
 * |  +------+       +---+\      +---+          | |
 * |   ^ ^ ^               \'i'  +---+  '\n'    | |
 * |   | | |                \--> | i | ------+  | |
 * |   | | |                     +---+       |  | |
 * +---|-|-|----------------+----------------|--|-+
 *     | | |                |                |  |
 *     | | |                | '[^hai!\n]'    |  |
 *     | | | print unrecog. |                |  |
 *     | | +----------------+   print 'hi'   |  |
 *     | +-----------------------------------+  |
 *     |               print 'ha'               |
 *     +----------------------------------------+
 */

/* Types of events */
enum eventType {
   Event_keyboard,
};

/* Compare keyboard character from transition's condition variable against
 * data in event. */
static bool compareKeyboardChar( void *ch, struct event *event );

static void printRecognisedChar( void *stateData, struct event *event );
static void printUnrecognisedChar( void *stateData, struct event *event );
static void printReset( void *stateData, struct event *event );
static void printHiMsg( void *stateData, struct event *event );
static void printHaMsg( void *stateData, struct event *event );
static void printErrMsg( void *stateData, struct event *event );
static void printEnterMsg( void *stateData, struct event *event );
static void printExitMsg( void *stateData, struct event *event );

/* Forward declaration of states so that they can be defined in an logical
 * order: */
static struct state checkCharsGroupState, idleState, hState, iState, aState;

/* All the following states (apart from the error state) are children of this
 * group state. This way, any unrecognised character will be handled by this
 * state's transition, eliminating the need for adding the same transition to
 * all the children states. */
static struct state checkCharsGroupState = {
   .parentState = NULL,
   /* The entry state is defined in order to demontrate that the 'reset'
    * transtition, going to this group state, will be 'redirected' to the
    * 'idle' state (the transition could of course go directly to the 'idle'
    * state): */
   .entryState = &idleState,
   .transitions = (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'!', &compareKeyboardChar,
         &printReset, &idleState, },
      { Event_keyboard, NULL, NULL, &printUnrecognisedChar, &idleState, },
   },
   .numTransitions = 2,
   .data = "group",
   .entryAction = &printEnterMsg,
   .exitAction = &printExitMsg,
};

static struct state idleState = {
   .parentState = &checkCharsGroupState,
   .entryState = NULL,
   .transitions = (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'h', &compareKeyboardChar, NULL,
         &hState },
   },
   .numTransitions = 1,
   .data = "idle",
   .entryAction = &printEnterMsg,
   .exitAction = &printExitMsg,
};

static struct state hState = {
   .parentState = &checkCharsGroupState,
   .entryState = NULL,
   .transitions = (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'a', &compareKeyboardChar, NULL,
         &aState },
      { Event_keyboard, (void *)(intptr_t)'i', &compareKeyboardChar, NULL,
         &iState },
   },
   .numTransitions = 2,
   .data = "H",
   .entryAction = &printRecognisedChar,
   .exitAction = &printExitMsg,
};

static struct state iState = {
   .parentState = &checkCharsGroupState,
   .entryState = NULL,
   .transitions = (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar,
         &printHiMsg, &idleState }
   },
   .numTransitions = 1,
   .data = "I",
   .entryAction = &printRecognisedChar,
   .exitAction = &printExitMsg,
};

static struct state aState = {
   .parentState = &checkCharsGroupState,
   .entryState = NULL,
   .transitions = (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar,
         &printHaMsg, &idleState }
   },
   .numTransitions = 1,
   .data = "A",
   .entryAction = &printRecognisedChar,
   .exitAction = &printExitMsg
};

static struct state errorState = {
   .entryAction = &printErrMsg
};


int main()
{
   struct stateMachine m;
   stateM_init( &m, &idleState, &errorState );

   int ch;
   while ( ( ch = getc( stdin ) ) != EOF )
      stateM_handleEvent( &m, &(struct event){ Event_keyboard,
            (void *)(intptr_t)ch } );

   return 0;
}

static bool compareKeyboardChar( void *ch, struct event *event )
{
   if ( event->type != Event_keyboard )
      return false;

   return (intptr_t)ch == (intptr_t)event->data;
}

static void printRecognisedChar( void *stateData, struct event *event )
{
   printEnterMsg( stateData, event );
   printf( "parsed: %c\n", (char)(intptr_t)event->data );
}

static void printUnrecognisedChar( void *stateData, struct event *event )
{
   printf( "unrecognised character: %c\n",
         (char)(intptr_t)event->data );
}

static void printReset( void *stateData, struct event *event )
{
   puts( "Resetting" );
}

static void printHiMsg( void *stateData, struct event *event )
{
   puts( "Hi!" );
}

static void printHaMsg( void *stateData, struct event *event )
{
   puts( "Ha-ha" );
}

static void printErrMsg( void *stateData, struct event *event )
{
   puts( "ENTERED ERROR STATE!" );
}

static void printEnterMsg( void *stateData, struct event *event )
{
   printf( "Entering %s state\n", (char *)stateData );
}

static void printExitMsg( void *stateData, struct event *event )
{
   printf( "Exiting %s state\n", (char *)stateData );
}
