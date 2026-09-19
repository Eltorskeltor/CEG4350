#include "project2.h"

// Define a keymap to convert keyboard scancodes to ASCII
volatile static char keymap[128] = {
	[0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',

    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',

    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',

    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',

    [0x34] = '.',
    [0x35] = '/',
    [0x39] = ' ',
    [0x1C] = '\n'
};

#if PROJECT == 2
// This function runs when compiled with the #define PROJECT 2 flag for make
int kernel()
{
    char string[101];

    while (1)
    {
        print("Enter a string: ");

        scan(string);

        print("You entered: ");
        print(string);
        print("\n");
    }

    return 0;
}
#endif

// Gets the character from the keyboard
// This is a blocking function that does not use interrupts
// Only I/O ports and polling are used
char getchar()
{
    uint8 scancode;

    while (1)
    {
        // Wait until the keyboard has data available
        while ((inb(0x64) & 0x01) == 0)
        {
        }

        // Read the scancode
        scancode = inb(0x60);

        // Ignore key release scancodes
        if (scancode & 0x80)
        {
            continue;
        }

        // Convert scancode to ASCII
        return keymap[scancode];
    }
}

// Read characters from the keyboard until the user hits the enter key
// Accepts a character array to fill with characters
// Terminates string with NULL terminator when done
void scan(char string[])
{
	int i = 0;
    char character;

    while (1)
    {
        character = getchar();

        // Stop when Enter is pressed
        if (character == '\n')
        {
            string[i] = '\0';
            putchar('\n');
            return;
        }

        // Store character in string
        string[i] = character;

        // Display character
        putchar(character);

        i++;
    }
}

void scroll(int rows)
{
    volatile char *video = (volatile char *)0xB8000;

    int offset = rows * 80 * 2;
    int i;

    // Move the existing screen contents upward
    for (i = 0; i < (25 - rows) * 80 * 2; i++)
    {
        video[i] = video[i + offset];
    }

    // Clear the rows at the bottom
    for (i = (25 - rows) * 80 * 2; i < 25 * 80 * 2; i += 2){
		video[i] = ' '; video[i + 1] = 0x07;
	}
}
