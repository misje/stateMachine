#include <stdint.h>
#include <stdio.h>
#include "stateMachine.h"

/* This simple example checks keyboad input against the two allowed strings
 * "ha\n" and "hi\n". If an unrecognised character is read, a group state will
 * handle this by printing a message and returning to the idle state.
 *
 *                    group state
 * +----------------------------------------------+
 * |  +------+  'h'  +---+  'a'  +---+  '\n'      |
 * |  | idle | ----> | h | ----> | a | ---------+ |
 * |  +------+       +---+\      +---+          | |
 * |   ^ ^ ^               \'i'  +---+  '\n'    | |
 * |   | | |                \--> | i | ------+  | |
 * |   | | |                     +---+       |  | |
 * +---|-|-|----------------+----------------|--|-+
 *     | | |                |                |  |
 *     | | |                | '[^hai\n]'     |  |
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
static void printUnrecognisedChar( struct event *event );
static void printErrMsg( void *stateData, struct event *event );
static void printEnterMsg( void *stateData, struct event *event );
static void printExitMsg( void *stateData, struct event *event );
static void printHiMsg( void *stateData, struct event *event );
static void printHaMsg( void *stateData, struct event *event );

/* Forward declaration of states so that they can be defined in an logical
 * order: */
static struct state checkCharsGroupState,
                    idleState,
                    hState,
                    iState,
                    aState;

/* All the following states (apart from the error state) are children of this
 * group state. This way, any unrecognised character will be handled by this
 * state's transition, eliminating the need for adding the same transition to
 * all the children states. */
static struct state checkCharsGroupState = {
   .parentState = NULL,
   /* This group state will never be entered (only exited from), so there is
    * no need for an entry state: */
   .entryState = NULL,
   .transitions = (struct transition[]){
      { Event_keyboard, NULL, NULL, &printUnrecognisedChar, &idleState, },
   },
   .numTransitions = 1,
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

static struct state iState = { &checkCharsGroupState, NULL,
   (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, NULL,
         &idleState } }, 1, "I", &printHiMsg , &printExitMsg };

static struct state aState = { &checkCharsGroupState, NULL,
   (struct transition[]){
      { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar, NULL,
         &idleState } }, 1, "A", &printHaMsg, &printExitMsg };

static struct state errorState = { NULL, NULL, NULL, 0, NULL, &printErrMsg,
   NULL };


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

static void printUnrecognisedChar( struct event *event )
{
   printf( "unrecognised character: %c\n",
         (char)(intptr_t)event->data );
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

static void printHiMsg( void *stateData, struct event *event )
{
   puts( "Hi!" );
}

static void printHaMsg( void *stateData, struct event *event )
{
   puts( "Ha-ha" );
}
