#include <stdio.h>

#include "m_argv.h"

#include "vroom.h"

pixel_t* DG_ScreenBuffer = NULL;

void M_FindResponseFile(void);
void D_DoomMain (void);


void vroom_Create(int argc, char **argv)
{
	// save arguments
    myargc = argc;
    myargv = argv;

	M_FindResponseFile();

	DG_ScreenBuffer = malloc(VROOM_RESX * VROOM_RESY * 4);

	DG_Init();

	D_DoomMain ();
}

