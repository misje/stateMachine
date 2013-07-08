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

/**
 * \addtogroup stateMachine
 * @{
 *
 * \file
 * \example stateMachineExample.c Simple example of how to create a state
 * machine
 */

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * \brief Event
 *
 * Events trigger transitions from a state to another. Event types are defined
 * by the user. Any event may optionally contain a payload (\ref 
 * #event::data "data").
 *
 * \sa state
 * \sa transition
 */
struct event
{
   /** \brief Type of event. Defined by user. */
   int type;
   /** 
    * \brief Event payload.
    *
    * How this is used is entirely up to the user. This data
    * is always passed together with an eventType in order to make it possible
    * to always cast the data correctly.
    */
   void *data;
};

struct state;

/**
 * \brief Transition between a state and another state
 *
 * All states that are not final must have at least one transition.
 * Transitions are triggered by events. If a state has more than one
 * transition with the same type of event (and the same condition), the first
 * transition in the array will be run. A transition may optionally run a
 * function #action, which will have the triggering event passed to it as an
 * argument.
 *
 * It is perfectly valid for a transition to return to the state it belongs
 * to. Such a transition will not call the state's #state::entryAction or
 * #state::exitAction.
 *
 * ### Examples ###
 * - An ungarded transition to a state with no action performed:
 * ~~~{.c}
 * {
 *    .eventType = Event_timeout,
 *    .condition = NULL,
 *    .guard = NULL,
 *    .action = NULL,
 *    .nextState = &mainMenuState,
 * },
 * ~~~
 * - A guarded transition executing an action
 * ~~~{.c}
 * {
 *    .eventType = Event_keyboard,
 *    .condition = NULL,
 *    .guard = &ensureNumericInput,
 *    .action = &addToBuffer,
 *    .nextState = &awaitingInputState,
 * },
 * ~~~
 * - A guarded transition using a condition
 * ~~~{.c}
 * {
 *    .eventType = Event_mouse,
 *    .condition = boxLimits,
 *    .guard = &coordinateWithinLimits,
 * ~~~
 * By using \ref #condition "conditions" a more general guard function can be
 * used, operating on the supplied argument #condition. In this example,
 * `coordinateWithinLimits` checks whether the coordinate in the mouse event
 * is within the limits of the "box".
 *
 * \sa event
 * \sa state
 */
struct transition
{
   /** \brief The event that will trigger this transition. */
   int eventType;
   /**
    * \brief Condition that event must fulfil
    *
    * This variable will be passed to #guard (if #guard is non-NULL) and may
    * be used as a condition that the incoming event's data must fulfil in
    * order for the transition to be performed. By using this variable, the
    * number of #guard functions can be minimized by making them more general.
    */
   void *condition;
   /**
    * \brief Check if data passed with event fulfils a condition
    *
    * A transition may be conditional. If so, this function, if non-NULL, will
    * be called. Its first argument will be supplied with #condition, which
    * can be compared againts #event. The user may choose to use this argument
    * or not. Only if the result is true, the transition will take place.
    *
    * \param condition event (data) to compare the incoming event against.
    * \param event the event passed to the state machine.
    *
    * \retval true if the event's data fulfils the condition, otherwise false.
    */
   bool ( *guard )( void *condition, struct event *event );
   /** 
    * \brief Function containing tasks to be performed during the transition
    *
    * The transition may optionally do some work in this function before
    * entering the next state. May be NULL.
    *
    * \param event the event passed to the state machine.
    */
   void ( *action )( struct event *event );
   /**
    * \brief The next state
    *
    * This must point to the next state that will be entered. It cannot be
    * NULL, but if it is, the state machine will detect it and enter the \ref
    * stateMachine::errorState "error state".
    */
   struct state *nextState;
};

/**
 * \brief State
 *
 * The current state in a state machine moves to a new state when one of the
 * #transitions in the current state acts on an event. An optional \ref
 * #exitAction "exit action" is called when the state is left, and an \ref
 * #entryAction "entry action" is called when the state machine enters a new
 * state. If a state returns to itself, neither #exitAction nor #entryAction
 * will be called.
 *
 * States may be organised in a hierarchy by setting \ref #parentState
 * "parent states". When a group/parent state is entered, the state machine is
 * redirected to the group state's \ref #entryState "entry state". If an event
 * does not trigger a transition in a state and if the state has a defined
 * parent state, the event will be passed to the parent state. Thus all
 * children of a state have a set of common #transitions. A parent state's
 * #entryAction will not be called if an event is passed on from a child
 * state. 
 *
 * The following lists the different types of states that may be created, and
 * how to create them:
 *
 * ### Normal state ###
 * ~~~{.c}
 * struct state normalState = {
 *    .parentState = &groupState,
 *    .entryState = NULL,
 *    .transition = (struct transition[]){
 *       { Event_keyboard, (void *)(intptr_t)'\n', &compareKeyboardChar,
 *          NULL, &msgReceivedState },
 *    },
 *    .numTransitions = 1,
 *    .data = normalStateData,
 *    .entryAction = &doSomething,
 *    .exitAction = &cleanUp,
 * };
 * ~~~
 * In this example, `normalState` is a childe of `groupState`, but the
 * #parentState value may also be NULL to indicate that it is not a child of
 * any group state.
 *
 * ### Group/parent state ###
 * A state becomes a group/parent state when it is linked to by child states
 * by using #parentState. No members in the group state need to be set in a
 * particular way.
 * ~~~{.c}
 * struct state groupState = {
 *    .entryState = &normalState,
 *    .entryAction = NULL,
 * ~~~
 * If there are any transitions in the state machine that lead to a group
 * state, it makes sense to define an entry state in the group. This can be
 * done by using #entryState, but it is not mandatory.
 *
 * \note If #entryState is defined for a group state, the group state's
 * #entryAction will not be called (the state pointed to by #entryState,
 * however, will have its #entryAction called).
 *
 * ### Final state ###
 * A final state is a state that terminates the state machine. A state is
 * considered as a final state if its #numTransitions is 0:
 * ~~~{.c}
 * struct state finalState = {
 *    .transitions = NULL,
 *    .numTransitions = 0,
 * ~~~
 * The error state used by the state machine to indicate errors should be a
 * final state.
 *
 * Final states must have #numTransitions set to 0.
 *
 * \note A state can only be a parent or a child (only #parentState or
 * #entryState, but not both, can be defined at a time).
 *
 * \sa event
 * \sa transition
 */
struct state
{
   /**
    * \brief If the state has a parent state, this pointer must be non-NULL.
    */
   struct state *parentState;
   /**
    * \brief If this state is a parent state, this pointer may point to a
    * child state that serves as an entry point.
    */
   struct state *entryState;
   /** 
    * \brief An array of transitions for the state.
    */
   struct transition *transitions;
   /** 
    * \brief Number of transitions in the array.
    */
   size_t numTransitions;
   /**
    * \brief Data that will be available for the state in its #entryAction and
    * #exitAction.
    */
   void *data;
   /** 
    * \brief This function is called whenever the state is being entered. May
    * be NULL.
    *
    * \note If a state returns to itself through a transition (either directly
    * or through a parent/group sate), its #entryAction will not be called.
    *
    * \param stateData the state's #data will be passed.
    * \param event the event that triggered a transition will be passed.
    */
   void ( *entryAction )( void *stateData, struct event *event );
   /**
    * \brief This function is called whenever the state is being left. May be
    * NULL
    *
    * \note If a state returns to itself through a transition (either directly
    * or through a parent/group sate), its #exitAction will not be called.
    *
    * \param stateData the state's #data will be passed.
    * \param event the event that triggered a transition will be passed.
    */
   void ( *exitAction )( void *stateData, struct event *event );
};

/**
 * \brief State machine
 *
 * Should not be manipulated directly by user.
 */
struct stateMachine
{
   /** \brief Pointer to the current state */
   struct state *currentState;
   /** 
    * \brief Pointer to previous state
    *
    * The previous state is stored for convenience in case the user needs to
    * keep track of previous states.
    */
   struct state *previousState;
   /** 
    * \brief Pointer to the state which will be entered whenever an error
    * occurs in the state machine.
    */
   struct state *errorState;
};

/**
 * \brief Initialise the state machine
 *
 * This function initialises the supplied stateMachine and sets the current
 * state to \pn{initialState}. No actions are performed until
 * stateM_handleEvent() is called. It is safe to call this function numerous
 * times, for instance in order to reset/restart the state machine if a final
 * state has been reached.
 *
 * \note The \ref #state::entryAction "entry action" for \pn{initialState}
 * will not be called.
 *
 * \param stateMachine the state machine to initalise.
 * \param initialState the initial state of the state machine.
 * \param errorState pointer to a state that acts a final state and notifys
 * the system/user that an eror has occurred.
 */
void stateM_init( struct stateMachine *stateMachine,
      struct state *initialState, struct state *errorState );

/**
 * \brief stateM_handleEvent() return values
 */
enum stateM_handleEventRetVals
{
   /** \brief Erroneous arguments were passed */
   stateM_errArg = -2,
   /** \brief The error state was reached
    *
    * The state machine enters the state machine if any of the following
    * happends:
    * - The current state is NULL
    * - A transition for the current event did not define the next state
    */
   stateM_errorStateReached,
   /** \brief The current state changed into a non-final state */
   stateM_stateChanged,
   /** \brief The state changed back to itself */
   stateM_stateLoopSelf,
   /** \brief The current state did not change on the given event
    *
    * If any event passed to the state machine should result in a state
    * change, this return value should be considered as an error.
    */
   stateM_noStateChange,
   /** \brief A final state (any but the error state) was reached */
   stateM_finalStateReached,
};

/**
 * \brief Pass an event to the state machine
 *
 * The event will be passed to the current state, and possibly to the current
 * state's parent states (if any). If the event triggers a transition, a new
 * state will be entered. If the new state has an entryAction defined, it will
 * be called.
 *
 * \param stateMachine the state machine to pass an event to.
 * \param event the event to be handled.
 *
 * \return #stateM_handleEventRetVals
 */
int stateM_handleEvent( struct stateMachine *stateMachine,
      struct event *event );

/**
 * \brief The current state
 *
 * \param stateMachine the state machine to get the current state from.
 *
 * \retval a pointer to the current state.
 * \retval NULL if \pn{stateMachine} is NULL.
 */
struct state *stateM_currentState( struct stateMachine *stateMachine );

/**
 * \brief The previous state
 *
 * \param stateMachine the state machine to get the previous state from.
 *
 * \retval the previous state.
 * \retval NULL if \pn{stateMachine} is NULL.
 * \retval NULL if there has not yet been any transitions.
 */
struct state *stateM_previousState( struct stateMachine *stateMachine );

/**
 * \brief Check if the state machine has stopped
 *
 * \param stateMachine the state machine to test.
 *
 * \retval true if the state machine has reached a final state.
 * \retval false if \pn{stateMachine} is NULL or if the current state is not a
 * final state.
 */
bool stateM_stopped( struct stateMachine *stateMachine );

#endif // STATEMACHINE_H

/**
 * @}
 */
