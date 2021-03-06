/** @file
 *	@brief	Board Support Package Header File for board that uses GTK for the simulated board - Windows or Linux desktop.
 *
 *	\copyright
 *	Copyright (c) 2020 Kevin L. Becker. All rights reserved.
 *
 *	Created on: Jun 26, 2020
 *	@author Kevin L. Becker
 */

#ifndef BOARD_BD_GTK_H
#define BOARD_BD_GTK_H

// ============================================================================
// ----	Include Files ---------------------------------------------------------
// ============================================================================

// ----	System Headers --------------------------
#include <stdint.h>			/* uint16_t */
#include <stdbool.h>		/* bool */

// ----	Project Headers -------------------------
//#include "tedlos.h"
#include <gtk/gtk.h>

// ----	Module Headers --------------------------
#include "../cwsw_board_common.h"


#ifdef	__cplusplus
extern "C" {
#endif


// ============================================================================
// ----	Constants -------------------------------------------------------------
// ============================================================================

/** Button IDs for this board. */
enum eBoardButtons
{
	kBoardButtonNone,
	kBoardButton0,
	kBoardButton1,
	kBoardButton2,
	kBoardButton3,
	kBoardButton4,
	kBoardButton5,
	kBoardButton6,
	kBoardButton7,
	kBoardNumButtons
};

enum eBoardLeds
{
	kBoardLed1,
	kBoardLed2,
	kBoardLed3,
	kBoardLed4,
	kBoardNumLeds
};


// ============================================================================
// ----	Type Definitions ------------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Public Variables ------------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Public API ------------------------------------------------------------
// ============================================================================

// ---- Discrete Functions -------------------------------------------------- {
extern uint16_t	bd_gtk__Init(void);

// ---- /Discrete Functions ------------------------------------------------- }

// ---- Targets for Get/Set APIs -------------------------------------------- {

/** "Chapter Designator" for Get/Set API.
 *	Intentionally unused symbol, designed to get you to the correct starting
 *	point, when you want to find macros for the Get/Set API; simply highlight
 *	the Module argument in your IDE (e.g, Eclipse, NetBeans, etc.), and select
 *	Go To Definition.
 */
enum { bd_gtk };		/* Component ID for Event Queue */

/** Target for `Get(bd_gtk, xxx)` interface */
#define bd_gtk__Get(resource)		bd_gtk__Get_ ## resource()


/** Target for `SET(kBoardLed1, kLogicalOff);`
 *	This function is equivalent to an arch-layer function that directly writes to the ports that
 *	drive the output "LEDn".
 *	@param [in]	onoff	On or off indication. In this implementation, there is no notion of "tristate" or degree of "on-ness".
 *	@note This is a "local" API, not designed to work across components.
 *	@{
 */
#define SET_kBoardLed1(onoff)				Set(Cwsw_Board, kBoardLed1, onoff)
#define SET_kBoardLed2(onoff)				Set(Cwsw_Board, kBoardLed2, onoff)
#define SET_kBoardLed3(onoff)				Set(Cwsw_Board, kBoardLed3, onoff)
#define SET_kBoardLed4(onoff)				Set(Cwsw_Board, kBoardLed4, onoff)
/**	@} */

/** Target for `Set(Cwsw_Board, kBoardLed, on_off)`
 * 	@{
 */
extern void Cwsw_Board__Set_kBoardLed1(bool value);
extern void Cwsw_Board__Set_kBoardLed2(bool value);
extern void Cwsw_Board__Set_kBoardLed3(bool value);
extern void Cwsw_Board__Set_kBoardLed4(bool value);
/**	@} */

/** Target 1 for TM(tmr) */
#define GET_tmrdebounce()	Cwsw_GetTimeLeft(tmrdebounce)	/* timer local to one SM state */
#define GET_tmrPressed()	Cwsw_GetTimeLeft(tmrPressed)	/* timer local to one SM state */

// ---- /Targets for Get/Set APIs ------------------------------------------- }


#ifdef	__cplusplus
}
#endif

#endif /* BOARD_BD_GTK_H */
