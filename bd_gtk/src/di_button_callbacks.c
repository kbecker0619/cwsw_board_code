/** @file
 *	@brief	One-sentence short description of file.
 *
 *	\copyright
 *	Copyright (c) 2020 Kevin L. Becker. All rights reserved.
 *
 *	Created on: Jun 29, 2020
 *	@author Kevin L. Becker
 */

// ============================================================================
// ----	Include Files ---------------------------------------------------------
// ============================================================================

// ----	System Headers --------------------------
#include <stdbool.h>

// ----	Project Headers -------------------------

// ----	Module Headers --------------------------
#include "cwsw_board.h"	/* pull in the GTK info */


// ============================================================================
// ----	Constants -------------------------------------------------------------
// ============================================================================

//  consecutive 1s: 	    8          7        6       5      4     3   2  1 (meaningless noise to fill 64 bits)
//	noisy input:		1111 1111 0111 1111 0111 1110 1111 1011 1101 1101 1010 0000 0011 1111 0000 0001
#define noisypatterna	0xFF7F7EFBDDA03F01

//	consecutive 0s:          8          7        6        5     4    3    2  1
// noisy input:			1000 0000 0000 1000 0000 1000 0001 0000 0100 0010 0010 01010
//						0x1 0010 1020 844A
#define noisypatternb	0x100101020844A

//						1111 1111 1001
#define cleanpatterna	0xFF9
#define cleanpatternb	~cleanpatterna


// ============================================================================
// ----	Type Definitions ------------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Global Variables ------------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Module-level Variables ------------------------------------------------
// ============================================================================

static uint64_t buttoninputbits[kBoardNumButtons]	= {0};
static uint32_t buttonstatus = 0;	// bitmapped image of current button state. 32 bits mostly to avoid compiler warnings.

GObject *btn0		= NULL;
GObject *btn1		= NULL;
GObject *btn2		= NULL;
GObject *btn3		= NULL;
GObject *btn4		= NULL;
GObject *btn5		= NULL;
GObject *btn6		= NULL;
GObject *btn7		= NULL;

/** Event queue to which button events will be posted.
 *	Nothing (at the moment) in this module directly uses this variable, but this button component
 *	"owns" the queue, and it is shared to the components (such as the SME) that need to post.
 */
tEvQ_QueueCtrlEx BtnQ = NULL;


// ============================================================================
// ----	Private Functions -----------------------------------------------------
// ============================================================================

// ============================================================================
// ----	Public Functions ------------------------------------------------------
// ============================================================================

/* i know very well that this is more complicated than it need be, however, this UI is there to use
 * and show the CWSW BSP, and as such, i need to translate GTK events into things the BSP under-
 * stands.
 *
 * for simpleness, each event and each button has its own callback, which then set flags as
 * appropriate for the CWSW code to process.
 */

/* on a physical board, my DI task will read all inputs at once, and will iterate through the
 * handlers for the individual assignments. to kinda-sorta replicate that behavior, we'll have one
 * button handler, and will detect which button was pressed by the widget address.
 *
 * note that on a real board, the DI action would be descended from a task, not a dispatched event
 * from the GUI framework.
 */
void
cbButtonClicked(GtkWidget *widget, gpointer data)
{
	UNUSED(widget);
	UNUSED(data);
	return;
	// no recognized objects, just leave
}


void
cbUiButtonPressed(GtkWidget *widget, gpointer data)
{
	uint32_t idx = kBoardButtonNone;
	UNUSED(data);

	// this layer knows about button assignments. for now, only the 1st 4 buttons are assigned.
	//	we're a bit inverted here, in that the GUI is hooked to this layer, but in a real board, the
	//	lower layers know which button is pressed and feed that info back up here.
	// 'twould rather use a switch statement, but can't b/c GCC complains about a pointer not being a scalar type
	if(widget == (GtkWidget *)btn0) { idx = kBoardButton0; } else
	if(widget == (GtkWidget *)btn1) { idx = kBoardButton1; } else
	if(widget == (GtkWidget *)btn2) { idx = kBoardButton2; } else
	if(widget == (GtkWidget *)btn3) { idx = kBoardButton3; } else
	if(widget == (GtkWidget *)btn4) { idx = kBoardButton4; } else
	if(widget == (GtkWidget *)btn5) { idx = kBoardButton5; } else
	if(widget == (GtkWidget *)btn6) { idx = kBoardButton6; } else
	if(widget == (GtkWidget *)btn7) { idx = kBoardButton7; } else
	{
		// empty final else clause
	}

	// toggle back and forth between two noisy sets of inputs.
	// - the 1st is 64 bits of noise, the tail of which is 8 "on" bits (hence a debounced button press).
	// - the 2nd intentionally fails qualification for a button press; remember that this function
	//	"sets" the pressed indicator, we we have to book-end the embedded 0 bits with "1" markers to
	// make the detection algorithms work.

	// call into the next layer down (arch)
	// for exploration, we'll use 8 consecutive bits of the same value to detect a state change
#if 0
	static bool noisypattern = false;	// location here in this function requires C99, prohibits Visual Studio
	buttoninputbits[idx] = noisypattern ? noisypatterna : noisypatternb; noisypattern = !noisypattern;
#else
	if(buttoninputbits[idx])
	{
		buttoninputbits[idx] += cleanpatterna * 4096; // clean pattern is 12 bits
	}
	else
	{
		buttoninputbits[idx] = cleanpatterna;
	}
#endif
	BIT_SET(buttonstatus, idx);
}

void
cbUiButtonReleased(GtkWidget *widget, gpointer data)
{
	uint32_t idx = 0;
	UNUSED(data);
	// 'twould rather use a switch statement, but can't b/c GCC complains about a pointer not be a scalar type
	if(widget == (GtkWidget *)btn0) { idx = kBoardButton0; } else
	if(widget == (GtkWidget *)btn1) { idx = kBoardButton1; } else
	if(widget == (GtkWidget *)btn2) { idx = kBoardButton2; } else
	if(widget == (GtkWidget *)btn3) { idx = kBoardButton3; } else
	if(widget == (GtkWidget *)btn4) { idx = kBoardButton4; } else
	if(widget == (GtkWidget *)btn5) { idx = kBoardButton5; } else
	if(widget == (GtkWidget *)btn6) { idx = kBoardButton6; } else
	if(widget == (GtkWidget *)btn7) { idx = kBoardButton7; } else
	{
		// empty final else clause
	}

	// call into the next layer down (arch)
#if 0
	static bool noisypattern = false;	// location here in this function requires C99, prohibits Visual Studio
	buttoninputbits[idx] = noisypattern ? noisypatternb : noisypatterna; noisypattern = !noisypattern;
#else
	if(buttoninputbits[idx])
	{
		buttoninputbits[idx] += (uint64_t)((cleanpatternb & 0xFFF) * 4096);
	}
	else
	{
		buttoninputbits[idx] = (uint64_t)((cleanpatternb & 0xFFF));
	}
#endif
	BIT_CLR(buttonstatus, idx);

	/* running commentaire, to be moved to more formal documentation.
	 * - if the DI button SM is in the "released" state, the 1st 1 bit will provoke a transition to
	 *   the debounce-press state; after the bit stream settles down to all 0s, it'll return to the
	 *   released state.
	 */
	return;
}


bool
di_read_next_button_input_bit(uint32_t idx)
{
	bool retval = ((buttoninputbits[idx] & 1) != 0);
	buttoninputbits[idx] /= 2;
	if((!retval) && (!buttoninputbits[idx]))	// if this bit is clear, it could be because the input stream is depleted; if the input stream is also depleted...
	{	// ... then check whether the "button pressed" flag is true
		// toggle buttons didn't work as i wanted - each press also gets a release, but it's a press-release sandwich where the "pressed" status is the meat
//		retval = gtk_toggle_button_get_active((GtkToggleButton *)btn0);	// todo: return state of last button pressed
		retval = BIT_TEST(buttonstatus, idx);
	}
	return retval;
}

bool
di_button_init(GtkBuilder *pUiPanel, ptEvQ_QueueCtrlEx pEvQX)
{
	bool bad_init = false;

	/* we want button-press and button-release events. for convenience and exploration, we'll also
	 * capture the click event.
	 */
	btn0 = gtk_builder_get_object(pUiPanel, "btn0");// run-time association w/ "ID" field in UI
	if(!btn0)	{ bad_init = true; }

	if(!bad_init)		// connect btn0, get handle for button 1
	{
		g_signal_connect(btn0, "clicked",	G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn0, "pressed",	G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn0, "released",	G_CALLBACK(cbUiButtonReleased), NULL);

		btn1 = gtk_builder_get_object(pUiPanel, "btn1");
		if(!btn1)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn1, get handle for button 2
	{
		g_signal_connect(btn1, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn1, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn1, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn2 = gtk_builder_get_object(pUiPanel, "btn2");
		if(!btn2)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn2, get handle for button 3
	{
		g_signal_connect(btn2, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn2, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn2, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn3 = gtk_builder_get_object(pUiPanel, "btn3");
		if(!btn3)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn3, get handle for button 4
	{
		g_signal_connect(btn3, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn3, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn3, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn4 = gtk_builder_get_object(pUiPanel, "btn4");
		if(!btn4)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn4, get handle for button 5
	{
		g_signal_connect(btn4, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn4, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn4, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn5 = gtk_builder_get_object(pUiPanel, "btn5");
		if(!btn5)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn5, get handle for button 6
	{
		g_signal_connect(btn5, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn5, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn5, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn6 = gtk_builder_get_object(pUiPanel, "btn6");
		if(!btn6)	{ bad_init = true; }
	}

	if(!bad_init)		// connect btn6, get handle for button 7
	{
		g_signal_connect(btn6, "clicked", G_CALLBACK(cbButtonClicked), NULL);
		g_signal_connect(btn6, "clicked", G_CALLBACK(cbUiButtonPressed), NULL);
		g_signal_connect(btn6, "clicked", G_CALLBACK(cbUiButtonReleased), NULL);

		btn7 = gtk_builder_get_object(pUiPanel, "btn7");
		if(!btn7)	{ bad_init = true; }
	}

	BtnQ = pEvQX;

	return bad_init;
}
