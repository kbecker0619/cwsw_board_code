/** @file
 *	@brief	Implementation of Button-handling state machine common to all boards.
 *
 *	\copyright
 *	Copyright (c) 2020 Kevin L. Becker. All rights reserved.
 *
 *	Created on: Jul 31, 2020
 *	@author Kevin L. Becker
 */

// ============================================================================
// ----	Include Files ---------------------------------------------------------
// ============================================================================

// ----	System Headers --------------------------
#include <stdbool.h>

// ----	Project Headers -------------------------
#include "cwsw_board.h"				// this module builds on top of the BSP

// ----	Module Headers --------------------------
#include "cwsw_bsp_buttons.h"		// public API for this module
#include "cwsw_bsp_buttons_cfg.h"	// project-specific configuration for this module


// ============================================================================
// ----	Constants -------------------------------------------------------------
// ============================================================================

/// "Reason3" reasons for exiting a state.
enum { kReasonNone, kReasonTwitchNoted, kReasonDebounced, kReasonTimeout, kReasonButtonUnstuck };

/// @todo Move this to a board-specific calibration.
enum eButtonCalibrationValues {
	/// Stuck button timeout value.
	kButtonStuckTimeoutValue = tmr1000ms * 30,

	/// Debounce time for board-level button-handling state machine.
	/* this timeout is just "comfortably" shy of enough time to read the full defined input
	 *	value of "noisy" input bits. the expected behavior then, is that this should transition
	 *	back to the released state, and immediately transition back here because the input stream
	 *	will have a few more "1" bits left in it. this 2nd invocation will result in a "solid"
	 *	debounced "press" reading.
	 * we know that this will take more overall time but for demonstration purposes will have
	 *	the same end result. timing: 1 cycle for our exit action, 1 cycle for "released"'s
	 *	entry action, 1 cycle for "released" to read a "1" bit, 1 cycle for "released"'s exit
	 *	action, 1 cycle for our entry action, and then we can begin accumulating debouncing bits.
	 */
	// 750 ms is long enough to read 64 bits of input stream, w/ ~100+ ms margin
	kTmButtonDebounceTime = tmr500ms + tmr100ms
};


// ============================================================================
// ----	Type Definitions ------------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Global Variables ------------------------------------------------------
// ============================================================================

tCwswSwAlarm	Btn_tmr_ButtonRead = {
	/* .tm			= */tmr10ms,
	/* .reloadtm	= */tmr10ms,
	/* .pEvQX		= */NULL,
	/* .evid		= */0,
	/* .tmrstate	= */kTmrState_Enabled
};


// ============================================================================
// ----	Module-level Variables ------------------------------------------------
// ============================================================================

static ptEvQ_QueueCtrlEx pBtnEvqx = NULL;


// ============================================================================
// ----	State Functions -------------------------------------------------------
// ============================================================================

/** @todo Move this prototype to a legitimate header - probably board level */
extern bool di_read_next_button_input_bit(uint32_t idx);

/** Button-debounce state.
 *	This routine is common for states when you're detecting a button push, and a release.
 */
static tStateReturnCodes
stDebounceButton(ptEvQ_Event pev, uint32_t *pextra)
{
	static tStateReturnCodes statephase[kBoardNumButtons] = {kStateUninit};
	static tCwswClockTics tmrMyStateTimer[kBoardNumButtons] = {0}, tmrdebounce;
	static tEvQ_EventID evId[kBoardNumButtons] = {0};
	static uint32_t reason3[kBoardNumButtons] = {kReasonNone};
	static uint8_t read_bits[kBoardNumButtons] = {0};
	uint32_t thisbutton;

	if(!pev)	{return 0;}
	if(!pextra)	{return 0;}

	thisbutton = pev->evData;
	switch(statephase[thisbutton]++)
	{
	case kStateUninit:	/* on 1ste entry, execute on-entry action */
	case kStateFinished:	/* upon return to this state after previous normal exit, execute on-entry action */
	default:			/* for any unexpected value, restart this state. */
		evId[thisbutton] = pev->evId;				// save exit Reason1
		statephase[thisbutton] = kStateOperational;	// reinitialize state's phase marker unilaterally

		/* for this task, we assume the transition was provoked by a non-zero bit on the most recent
		 * DI bit read. seed our debounce var with that 1st bit.
		 * NOTE: his line is legacy from early development. its effect is that it allows recognition
		 * of a a switch press after 8 consecutive bits, but requires 10 consecutive bits to
		 * recognize a switch release (the 1st 0 read is thrown away, then it needs another one to
		 * "clear" this seeding of the initial 1).
		 */
		read_bits[thisbutton] = 1;

		// start my state timer. remember, our call rate is 10 ms. 100ms == 10 bit readings, 640ms is 64 bit reads
		Set(Cwsw_Clock, tmrMyStateTimer[thisbutton], kTmButtonDebounceTime);
		break;

	case kStateOperational:
		// TM() API doesn't work w/ array syntax; copy to local scalar timer
		tmrdebounce = tmrMyStateTimer[thisbutton];
		// read next bit
		read_bits[thisbutton] <<= 1;				// shift current bits left one position
		read_bits[thisbutton] = (uint8_t)(read_bits[thisbutton] | di_read_next_button_input_bit(thisbutton));
		if(read_bits[thisbutton] == 0)
		{
			// debounce done, recognized as an open (released) button
			evId[thisbutton] = evBtnReleased;
			reason3[thisbutton] = kReasonDebounced;
		}
		else if(read_bits[thisbutton] == 0xFF)
		{
			// debounce done, recognized as button press, advance to next state
			evId[thisbutton] = evBntPressed;
			reason3[thisbutton] = kReasonDebounced;
		}
		else if(TM(tmrdebounce))
		{
			reason3[thisbutton] = kReasonTimeout;
		}
		else
		{
			--statephase[thisbutton];		// nothing of note happened, stay in this state
		}
		break;

	case kStateExit:
		// for this edition of this state, no state-specific exit action is required.
		//	let the caller (normally the SME) know what event and what guard provoked the change.
		pev->evId = evId[thisbutton];	// save exit reason 1 (event that provoked the exit)
		pev->evData = thisbutton;		// save exit reason 2 (button recognized)
		*pextra = reason3[thisbutton];	// save exit reason 3 (reason for exit (no button, button, timeout)
		break;
	}

	// the next line is part of the template and should not be touched.
	return statephase[thisbutton];
}


static tStateReturnCodes
stStart(ptEvQ_Event pev, uint32_t *pextra)
{
	static tStateReturnCodes statephase[kBoardNumButtons] = {kStateUninit};
	static tEvQ_EventID evId[kBoardNumButtons];
	uint32_t thisbutton;

	if(!pev)	{ return 0; }
	if(!pextra)	{ return 0; }

	thisbutton = pev->evData;
	switch(statephase[thisbutton]++)
	{
	case kStateUninit:	/* on 1st entry, execute on-entry action */
	case kStateFinished:	/* upon return to this state after previous normal exit, execute on-entry action */
	default:			/* for any unexpected value, restart this state. */
		// generic state management, common to all states
		statephase[thisbutton]	= kStateOperational;
		evId[thisbutton]		= pev->evId;			// save default exit reason 1.

		// ---- state-specific behavior ---------
		// no state-specific behavior for this state
//		printf("Entering %s\n", __FUNCTION__);
		break;

	case kStateOperational:
		// we'll basically leave as soon as we start.
		break;

	case kStateExit:
		// manage the state machine: set exit reasons
		pev->evId = evId[thisbutton];	// exit reason 1: event that provoked the exit.
		pev->evData = thisbutton;		// exit reason 2
		*pextra = kReasonNone;			// exit reason 3 (for debugging use in transition)

		// state-specific behavior
		/* (no state-specific exit actions here) */
//		printf("Leaving %s\n", __FUNCTION__);
		break;
	}

	// the next line is part of the template and should not be touched.
	return statephase[thisbutton];
}

/**	Implement the Button Released state of the SM.

 *	@note If return here is due to a stuck-button event, this state will continue to read a stream
 *		  of "1" bits from the DI layer. In that case, there will be a continual cycle between this
 *		  state, debounce-press, and pressed, then back here. \n
 *		  The default SM has a special "stuck" state, which is intended to eliminate this cycle, but
 *		  we wanted to describe the behavior if something should go wrong.
 *	@{
 */
static tStateReturnCodes
stButtonReleased(ptEvQ_Event pev, uint32_t *pextra)
{
	static tStateReturnCodes statephase[kBoardNumButtons] = {kStateUninit};
	uint32_t thisbutton;

	if(!pev)	{return 0;}
	if(!pextra)	{return 0;}

	thisbutton = pev->evData;
	switch(statephase[thisbutton]++)
	{
	case kStateUninit:		/* on 1st entry, execute on-entry action */
	case kStateFinished:	/* upon return to this state after previous normal exit, execute on-entry action */
	default:				/* for any unexpected value, restart this state. */
		// generic state management, common to all states
		statephase[thisbutton] = kStateOperational;	// reinitialize state's phase marker unilaterally

		// ---- state-specific behavior ---------
		// no state-specific behavior for this state
//		printf("Entering %s\n", __FUNCTION__);
		break;

	case kStateOperational:
		do {
			// use local var so i can override it during debugging.
			bool thisbit = di_read_next_button_input_bit(thisbutton);	// issue #3: pass the current button
			if(!thisbit)
			{
				// stay in this state until we see a twitch on one of the button inputs.
				//	note: in this iteration of this implementation, we're only reading "button" 0
				--statephase[thisbutton];
			}
		} while(0);
		break;

	case kStateExit:
		// manage the state machine: set exit reasons
		// evId field (reason1) is OK as passed in.
		pev->evData = thisbutton;		// reason2
		*pextra = kReasonTwitchNoted;	// reason3

		// state-specific behavior
		/* (no state-specific exit actions here) */
//		printf("Leaving %s\n", __FUNCTION__);
		break;
	}

	// the next line is part of the template and should not be touched.
	return statephase[thisbutton];
}
/** @} */

static tStateReturnCodes
stDebouncePress(ptEvQ_Event pev, uint32_t *pextra)
{
	return stDebounceButton(pev, pextra);
}

static tStateReturnCodes
stButtonPressed(ptEvQ_Event pev, uint32_t *pextra)
{
	static tStateReturnCodes statephase[kBoardNumButtons] = {kStateUninit};
	static tCwswClockTics tmrPressedStateTimer[kBoardNumButtons] = {0}, tmrPressed;
	static tEvQ_EventID evId[kBoardNumButtons] = {0};
	static uint32_t reason2[kBoardNumButtons] = {0};
	static uint32_t reason3[kBoardNumButtons] = {kReasonNone};
	uint32_t thisbutton;

	if(!pev)	{return 0;}
	if(!pextra)	{return 0;}

	thisbutton = pev->evData;
	switch(statephase[thisbutton]++)
	{
	case kStateUninit:
	case kStateFinished:
	default:
		evId[thisbutton] = pev->evId;				// save exit Reason1
		reason3[thisbutton] = kReasonNone;			// save default exit Reason3
		statephase[thisbutton] = kStateOperational;	// reinitialize state's phase marker unilaterally

		/* for this task, we stay here as long as the button remains pressed, or until the timeout
		 * period expires. a "release" is seen as a zero bit on the bit input stream.
		 */
		Set(Cwsw_Clock, tmrPressedStateTimer[thisbutton], kButtonStuckTimeoutValue);
//		printf("Entering %s\n", __FUNCTION__);
		break;

	case kStateOperational:
		do {
			bool thisbit;
			tmrPressed = tmrPressedStateTimer[thisbutton];
			// use local var so i can override it during debugging.
			thisbit = di_read_next_button_input_bit(thisbutton);
			if(!thisbit)
			{
				// button might have been released, go to debounce-release state to confirm
				reason2[thisbutton] = thisbutton;
				reason3[thisbutton] = kReasonTwitchNoted;
			}
			else if(TM(tmrPressed))
			{
				// we've been too long in the pressed-button state, there might be a stuck button
				reason2[thisbutton] = 0;
				reason3[thisbutton] = kReasonTimeout;
			}
			else
			{
				--statephase[thisbutton];	// nothing of note happened, stay in this state
			}
		} while(0);
		break;

	case kStateExit:
		// for this edition of this state, no state-specific exit action is required.
		//	let the caller (normally the SME) know what event and what guard provoked the change.
		pev->evId = evId[thisbutton];		// save exit reason 1 (event that provoked the exit)
		pev->evData = reason2[thisbutton];	// save exit reason 2 (button recognized)
		*pextra = reason3[thisbutton];		// save exit reason 3 (reason for exit (no button, button, timeout)
//		printf("Leaving %s\n", __FUNCTION__);
		break;
	}

	// the next line is part of the template and should not be touched.
	return statephase[thisbutton];
}

static tStateReturnCodes
stDebounceRelease(ptEvQ_Event pev, uint32_t *pextra)
{
	return stDebounceButton(pev, pextra);
}

static tStateReturnCodes
stButtonStuck(ptEvQ_Event pev, uint32_t *pextra)
{
	static tStateReturnCodes statephase[kBoardNumButtons] = {kStateUninit};
	static tEvQ_EventID evId[kBoardNumButtons] = {0};
	uint32_t thisbutton;

	if(!pev)	{return 0;}
	if(!pextra)	{return 0;}

	thisbutton = pev->evData;
	switch(statephase[thisbutton]++)
	{
	case kStateUninit:	/* on 1st entry, execute on-entry action */
	case kStateFinished:	/* upon return to this state after previous normal exit, execute on-entry action */
	default:			/* for any unexpected value, restart this state. */
		evId[thisbutton] = pev->evId;	// save exit Reason1
//		printf("Entering %s\n", __FUNCTION__);
		break;

	case kStateOperational:
		do {
			bool thisbit = di_read_next_button_input_bit(0);
			if(thisbit)
			{
				// stay in this state as long as we read a "1" bit
				--statephase[thisbutton];
			}
		} while(0);
		break;

	case kStateExit:
		// for this edition of this state, no state-specific exit action is required.
		//	let the caller (normally the SME) know what event and what guard provoked the change.
		pev->evId = evId[thisbutton];	// save exit reason 1 (event that provoked the exit)

		// there's one and only one reason we leave this state, no reason for supplying reasons.
		//	However, to allow the transition action to make an informed decision, indicate a unique
		//	exit code.
		pev->evData = thisbutton;
		*pextra = kReasonButtonUnstuck;
//		printf("Leaving %s\n", __FUNCTION__);
		break;
	}

	// the next line is part of the template and should not be touched.
	return statephase[thisbutton];
}


// ============================================================================
// ----	Transition Functions --------------------------------------------------
// ============================================================================

/** Do-nothing transition function.
 *	This is entirely a debugging aid, it is not required by the transition table.
 */
static void
NullTransition(tEvQ_Event ev, uint32_t extra)
{
	UNUSED(ev);
	UNUSED(extra);
//	printf("Transition: ev: %i, Button: %i, Transition ID: %i\n", ev.evId, ev.evData, extra);
}

/** Transition Function.
 * 	This function notifies the world of a state change in our button-reading SM.
 * 	Not sure it really matters to our design, if we post this event in the exit function vs. the
 * 	transition; philosophically, it makes sense for the state to notify the world of its own status
 * 	change, but actually (as realized in this implementation) it does not affect the timing of the
 * 	event posting, in that both the exit action and the transition function are executed in the same
 * 	instance of the SME. the one (small) factor that tilts me towards the transition function as the
 * 	poster of note, is that philosophically, the exit action is supposed to be atomic and robust (as
 * 	in, cannot fail), but in our system, the posting of an event can fail.
 *
 * 	we rely on the state providing the event detail to be posted.
 */
static void
NotifyBtnStateChg(tEvQ_Event ev, uint32_t extra)
{
	switch(extra)
	{
	case kReasonDebounced:
		// evId indicates button press or release
		// evData indicates which button
		// not much to do here except validate
		switch(ev.evId)
		{
		case evBtnReleased:	// button has been released
		case evBntPressed:	// state change to Pressed state
			break;

		default:
			ev.evId = 0;
			break;
		}
		break;

	case kReasonTimeout:
		ev.evId = evButton_BtnStuck;
		break;

	case kReasonButtonUnstuck:
		ev.evId = evButton_BtnUnstuck;
		break;

	default:
		ev.evId = 0;
		break;
	}
	if(ev.evId)
	{
		(void)Cwsw_EvQX__PostEvent(pBtnEvqx, ev);
	}
}


/* the button reads use the following state machine:
 * start -> released -> debounce-press -> pressed -> debounce-release
 * 				^				|			|               |
 * 				+-------- timeout (stuck)  -/               |
 * 				\-------------------------------------------/ (unimportant if debounce achieved, or if timeout)
 *
 *	~these transitions are in the same order as listed in the design document.~ (not anymore)
 */
static tTransitionTable tblTransitions[] = {
	// current				Reason1					Reason2		Reason3					Next State			Transition Func
	{ stStart,				evButton_Task,	0xFF,	kReasonNone,			stButtonReleased,	NullTransition		},	// normal termination

	{ stButtonReleased,		evButton_Task,	0xFF,	kReasonTwitchNoted,		stDebouncePress,	NullTransition		},	// normal termination: non-0 bit seen @ button

	{ stDebouncePress,		evBntPressed,	0xFF,	kReasonDebounced,		stButtonPressed,	NotifyBtnStateChg	},	// normal termination (debounced input is 0xFF)
	{ stDebouncePress,		evBtnReleased,	0xFF,	kReasonDebounced,		stButtonReleased,	NullTransition		},	// debounced input is 0. no need to post event, since debounced state hasn't changed.
	{ stDebouncePress,		evButton_Task,	0xFF,	kReasonTimeout,			stButtonReleased,	NullTransition		},	// debounce timeout

	{ stButtonPressed,		evButton_Task,	0xFF,	kReasonTwitchNoted,		stDebounceRelease,	NullTransition		},
	{ stButtonPressed,		evButton_Task,	0xFF,	kReasonTimeout,			stButtonStuck,		NotifyBtnStateChg	},	// button stuck, go directly back to "stuck" state

	{ stDebounceRelease,	evBtnReleased,	0xFF,	kReasonDebounced,		stButtonReleased,	NotifyBtnStateChg	},
	{ stDebounceRelease,	evBntPressed,	0xFF,	kReasonDebounced,		stButtonPressed,	NullTransition		},

	// in the interests of simplicity (MVP), we'll jump directly back to the Released state.
	//	we could insert another instance of the debouncer, but except for transition time, the end effect will be the same.
	{ stButtonStuck,		evButton_Task,	0xFF,	kReasonButtonUnstuck,	stButtonReleased,	NotifyBtnStateChg	},
};


// ============================================================================
// ----	Public Functions ------------------------------------------------------
// ============================================================================

/** Button handler.
 *	This version is specifically tied to the GTK board. It is a prime candidate for refactoring once
 *	we add support for more boards.
 *
 *	This handler needs to make the adaptation between our array of button state machines (one SM for
 *	each button), and the single-instance SME.
 */
void
Btn_tsk_ButtonRead(tEvQ_Event ev, uint32_t extra)	// uses DI lower layers
{
	static pfStateHandler currentstate[kBoardNumButtons] = {NULL};
	uint32_t idxbutton = TABLE_SIZE(currentstate);

	while(idxbutton--)
	{
		if(!currentstate[idxbutton])	{ currentstate[idxbutton] = stStart; }

		ev.evData = idxbutton;
		currentstate[idxbutton] = Cwsw_Sme__SME(
				tblTransitions, TABLE_SIZE(tblTransitions),
				currentstate[idxbutton], ev, extra);

		if(!currentstate[idxbutton])
		{
			// disable alarm that launches this SME via its event.
			//	if restarted, we'll resume in the current state
			//	need a way to restart w/ the init state.
			Btn_tmr_ButtonRead.tmrstate = kTmrState_Disabled;
		}
	}	// idxbutton
}


/** Set button event parameters.
 */
void
Btn_SetQueue(tEvQ_EventID const evId, const ptEvQ_QueueCtrlEx pEvqx)
{
	// set queue for button activity
	pBtnEvqx = pEvqx;
	// set parameters for timer expiration notifications
	Btn_tmr_ButtonRead.pEvQX = pEvqx;
	Btn_tmr_ButtonRead.evid = evId;
}
