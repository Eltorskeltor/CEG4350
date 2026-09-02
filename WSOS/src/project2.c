#include "project2.h"

// Define a keymap to convert keyboard scancodes to ASCII
static char keymap[128] = {
	[0x1E] = 'a',
    [0x30] = 'b',
    [0x2E] = 'c',
    // Add remaining ascii characters for each scan code
};

#if PROJECT == 2
// This function runs when compiled with the #define PROJECT 2 flag for make
int kernel()
{
	// Ask the user to type in stuff forever
	
	return 0;
}
#endif

// Gets the character from the keyboard
// This is a blocking function that does not use interrupts
// Only I/O ports and polling are used
char getchar()
{
    (void)keymap; // Remove this when implementing your function
	// Convert the scancode to an ASCII character using the key map
	return 0;
}

// Read characters from the keyboard until the user hits the enter key
// Accepts a character array to fill with characters
// Terminates string with NULL terminator when done
void scan(char string[])
{
	(void)string; // Remove this when implementing your function
}

void scroll(int rows)
{
    (void)rows; // Remove this when implementing your function
}