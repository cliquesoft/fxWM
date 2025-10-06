// config.h	Contains the configuration parameters for the software.
//		It's based on original work by Bill Spitzak.
//
// created	2015/06/30 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/06/30 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)




// [ Desktop Behavior ]

// Whether multiple desktops are allowed.
// On: enables multiple desktops
// Off: disables multiple desktops (reduces size ~20k)
#define MULTIPLE_DESKTOPS 0

// Whether the user is prompted for a new desktop name when MULTIPLE_DESKTOPS=1.
// On: prompts the user for a name
// Off: ignores assigning a name to the new desktop (reduces size ~12k)
#define ASK_FOR_NEW_DESKTOP_NAME 1




// [ Window Behavior ]

// Defines how many pixels from the edge of the window for resize handle.
#define RESIZE_EDGE 5

// must drag window this far off screen to snap the border off the screen.
#define EDGE_SNAP 50

// Must drag the window this far off screen before it moves off screen.
#define SCREEN_SNAP 100




// [ Keyboard Behavior ]

// Define the hotkeys for the window manager (see io.cpp)
#define TOGGLECODE_HOTKEYS 1		// allows the toggle code to enable/disable the window manager
#define FOCUS_HOTKEYS 1			// alt+esc, alt+tab, alt+shift+tab
#define WINDOW_HOTKEYS 1		// allows manipulation of the focused window via the keyboard
#define DESKTOP_HOTKEYS 0		// alt+F# goes to desktop #, meta-alt cycles desktops, meta-left|right traverses; requires "MULTIPLE_DESKTOPS 1" above!
#define EXTENDED_HOTKEYS 1		// enables things like meta for Start Menu, meta-d for Desktop, etc

#define MICROSOFT_HOTKEYS 0		// converts all the enabled hotkeys to Microsoft designations

#define TEST_HOTKEYS 0			// enables the defined hotkeys above, but prefaces each with the 'meta' key




// [ Operational Behavior ]

#define ALERTS 1			// whether alerts are available in the window manager
#define CONTROL_SOCK 1			// whether the fxwm-control socket should be used or not
#define MONITOR_PIPE 1			// whether the fxwm-monitor pipe should be used or not
#define SCREEN_CAPTURE 0		// whether the screen capturing functionality should be used or not	WARNING: this is NOT yet implemented

#define TOGGLE_MAXIMIZE 1		// whether the maximize/windowed state is a toggle or separate hotkeys

#define FXWM 1				// if not defined then portions not used by FXWM are included




// [ Developer Modifications ]

// Defines if a sample window should be created to test functionality.
// 0: operates in a normal way
// 1: creates the test window
#define TEST 0

// Defines if DEBUG output should be displayed during operation.
// 0: no
// 1: yes
#define DEBUG 0




// [ Mandatory ]

#define FL_INTERNALS 1			// this is required for the main.h file to compile correctly

