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

#include "stateMachine.h"

static void goToErrorState( struct stateMachine *stateMachine,
      struct event *const event );
static struct state *goToNextState( struct stateMachine *stateMachine,
      struct state *state, struct event *const event );

void stateM_init( struct stateMachine *fsm,
      struct state *initialState, struct state *errorState )
{
   if ( !fsm )
      return;

   fsm->currentState = initialState;
   fsm->previousState = NULL;
   fsm->errorState = errorState;
}

int stateM_handleEvent( struct stateMachine *fsm,
      struct event *event )
{
   if ( !fsm || !event )
      return stateM_errArg;

   if ( !fsm->currentState )
   {
      goToErrorState( fsm, event );
      return stateM_errorStateReached;
   }

   struct state *parentState, *nextState = fsm->currentState;
   do {
      parentState = nextState->parentState;
      nextState = goToNextState( fsm, nextState, event );
      if ( fsm->currentState == fsm->errorState )
         return stateM_errorStateReached;

      /* If the state returned to itself, exit and do not call state's entry
       * action: */
      if ( nextState == fsm->currentState )
         return stateM_stateLoopSelf;

      /* If there were no transitions for the given event for the current
       * state, check if there are any transitions for any of the parent
       * states (if any): */
      if ( !nextState )
         nextState = parentState;
      else
      {
         fsm->previousState = fsm->currentState;
         fsm->currentState = nextState;

         /* If the new state is a parent state, enter its entry state (if it
          * has one): */
         if ( fsm->currentState->entryState )
         {
            fsm->previousState = fsm->currentState;
            fsm->currentState = fsm->currentState->entryState;
         }

         /* Call the new state's entry action if it has any: */
         if ( fsm->currentState->entryAction )
            fsm->currentState->entryAction( fsm->currentState->data, event );

         if ( fsm->currentState == fsm->errorState )
            return stateM_errorStateReached;

         /* If the new state is a final state, notify user that the state
          * machine has stopped: */
         if ( !fsm->currentState->numTransitions )
            return stateM_finalStateReached;

         return stateM_stateChanged;
      }
   } while ( nextState );

   return stateM_noStateChange;
}

struct state *stateM_currentState( struct stateMachine *fsm )
{
   if ( !fsm )
      return NULL;

   return fsm->currentState;
}

struct state *stateM_previousState( struct stateMachine *fsm )
{
   if ( !fsm )
      return NULL;

   return fsm->previousState;
}


static void goToErrorState( struct stateMachine *fsm,
      struct event *const event )
{
   fsm->previousState = fsm->currentState;
   fsm->currentState = fsm->errorState;

   if ( fsm->currentState && fsm->currentState->entryAction )
      fsm->currentState->entryAction( fsm->currentState->data, event );
}

static struct state *goToNextState( struct stateMachine *fsm,
      struct state *state, struct event *const event )
{
   size_t i;

   for ( i = 0; i < state->numTransitions; ++i )
   {
      struct transition *t = &state->transitions[ i ];

      /* A transition for the given event has been found: */
      if ( t->eventType == event->type )
      {
         /* If the transition is guarded, skip if condition is not held: */
         if ( t->guard && !t->guard( t->condition, event ) )
            continue;
         else
         {
            /* Run exit action only if the current state is left (only if it
             * does not return to itself): */
            if ( t->nextState != state && state->exitAction )
               state->exitAction( state->data, event );

            /* Run transition action (if any): */
            if ( t->action )
               t->action( state->data, event );

            state = t->nextState;

            /* A transition must have defined a next state. If the user has
             * not defined the next state, go to error state: */
            if ( !state )
               goToErrorState( fsm, event );

            return state;
         }
      }
   }

   /* No transitions found for given event for given state: */
   return NULL;
}

bool stateM_stopped( struct stateMachine *stateMachine )
{
   if ( !stateMachine )
      return true;

   return stateMachine->currentState->numTransitions == 0;
}
