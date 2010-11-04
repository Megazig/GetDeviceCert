#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <sys/dir.h>
#include <errno.h>

#include <sdcard/wiisd_io.h>
#include <fat.h>

#define		DEBUG			0

#define		EFATINIT		-1
#define		ENETINIT		-2

#define		EELMMOUNT		-3

// Colors
#define		BLACK_FG		30
#define		BLACK_BG		40
#define		RED_FG			31
#define		RED_BG			41
#define		GREEN_FG		32
#define		GREEN_BG		42
#define		YELLOW_FG		33
#define		YELLOW_BG		43
#define		BLUE_FG			34
#define		BLUE_BG			44
#define		MAGENTA_FG		35
#define		MAGENTA_BG		45
#define		CYAN_FG			36
#define		CYAN_BG			46
#define		WHITE_FG		37
#define		WHILTE_BG		47

// Text Styles
#define		RESET_TEXT		0
#define		BRIGHT_TEXT		1
#define		DIM_TEXT		2
#define		UNDERSCORE_TEXT	3
#define		BLINKING_TEXT	4
#define		REVERSE_TEXT	5
#define		HIDDEN_TEXT		6

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

s32 ExitRequested = 0;
s8 HWButton = -1;

//--------------------------------------------------------------------------------
void WiiResetPressed() {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Returns:	None
//
	HWButton = SYS_RETURNTOMENU;
}

//--------------------------------------------------------------------------------
void WiiPowerPressed() {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Returns:	None
//
	HWButton = SYS_POWEROFF_STANDBY;
}

//--------------------------------------------------------------------------------
void WiimotePowerPressed( int channel ) {
//--------------------------------------------------------------------------------
//
//	Params:		channel		Channel of wiimote whose power button was pressed
//	Returns:	None
//
	HWButton = SYS_POWEROFF_STANDBY;
}

//--------------------------------------------------------------------------------
void SetTextInfo( int color_fg , int color_bg , int attribute ) {
//--------------------------------------------------------------------------------
//
//	Params:		color_fg			Foreground Color For Text
//				color_bg			Background Color For Text
//				attribute			Text Style
//	Returns:	None
//

	printf( "\x1b[%dm" , attribute );
	printf( "\x1b[%dm" , color_fg );
	printf( "\x1b[%dm" , color_bg );
}

//--------------------------------------------------------------------------------
void ClearText( ) {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Returns:	None
//

	printf( "\x1b[2J" );
}

//--------------------------------------------------------------------------------
void PrintPositioned( int y , int x , const char * text ) {
//--------------------------------------------------------------------------------
//
//	Params:		y			Y Position for Text
//				x			X Position for Text
//				string		Strig to Print
//	Returns:	None
//

	printf("\x1b[%d;%dH", y , x );
	printf("%s" , text );
}

//--------------------------------------------------------------------------------
void ExitToLoader( int return_val ) {
//--------------------------------------------------------------------------------
//
//	Params:		return_val	Value to Pass to Exit
//	Return:		None		exits
//

	printf("\x1b[25;5HThanks for using DeviceCertDumper\n");
	exit( return_val );
}

//--------------------------------------------------------------------------------
void WaitForButtonPress() {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Return:		None
//

	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application when home is pressed
		if ( pressed & WPAD_BUTTON_HOME ) ExitToLoader(0);
		// We break the waiting loop with any other buttons
		if ( pressed & WPAD_BUTTON_A ) break;
		if ( pressed & WPAD_BUTTON_B ) break;
		if ( pressed & WPAD_BUTTON_1 ) break;
		if ( pressed & WPAD_BUTTON_2 ) break;
		if ( pressed & WPAD_BUTTON_PLUS ) break;
		if ( pressed & WPAD_BUTTON_MINUS ) break;
		if ( pressed & WPAD_BUTTON_UP ) break;
		if ( pressed & WPAD_BUTTON_DOWN ) break;
		if ( pressed & WPAD_BUTTON_LEFT ) break;
		if ( pressed & WPAD_BUTTON_RIGHT ) break;

		// Add check for Reset or Power button
		if ( HWButton != -1 )
			SYS_ResetSystem( HWButton , 0 , 0 );

		// Wait for the next frame
		VIDEO_WaitVSync();
	}
}

//--------------------------------------------------------------------------------
void ShowProgramInfo() {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Returns:	None
//

	printf("\x1b[2;0H");
	printf("DeviceCertDumper\n");
	printf("coded by\n");
	SetTextInfo( BLINKING_TEXT , GREEN_FG , BLACK_BG );
	printf("megazig\n");
	SetTextInfo( RESET_TEXT , WHITE_FG , BLACK_BG );
}

//--------------------------------------------------------------------------------
char ascii(char s) {
//--------------------------------------------------------------------------------
//
//	Params:		char s
//	Returns:	None
//
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

//--------------------------------------------------------------------------------
void hexdump(FILE *fp, void *d, int len) {
//--------------------------------------------------------------------------------
//
//	Params:		FILE *fp , void *d , int len
//	Returns:	None
//
	u8 *data;
	int i, off;
	data = (u8*)d;
	for (off=0; off<len; off += 16) {
		fprintf(fp, "%08x  ",off);
		for(i=0; i<16; i++)
			if((i+off)>=len) fprintf(fp, "  ");
			else fprintf(fp, "%02x ",data[off+i]);

		fprintf(fp, " ");
		for(i=0; i<16; i++)
			if((i+off)>=len) fprintf(fp," ");
			else fprintf(fp,"%c",ascii(data[off+i]));
		fprintf(fp,"\n");
	}
}

//--------------------------------------------------------------------------------
void Init() {
//--------------------------------------------------------------------------------
//
//	Params:		None
//	Returns:	None
//

	// Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	SYS_SetResetCallback( WiiResetPressed );
	SYS_SetPowerCallback( WiiPowerPressed );
	WPAD_SetPowerButtonCallback( WiimotePowerPressed );

	ClearText();
	printf("\x1b[2;0H");
	ShowProgramInfo();

	PrintPositioned( 15 , 0 , "Initializing FAT File System.........." );
	if(!fatInitDefault()) {
		printf("fatInit failed\n");
		WaitForButtonPress();
		ExitToLoader( EELMMOUNT );
	}
	int ret = fatMountSimple("sd", &__io_wiisd);
	if ( !ret ) {
		PrintPositioned( 15 , 45 , "fatMount failed :( \n" );
		printf("fatMount returned: %d\n", ret);
		WaitForButtonPress();
		ExitToLoader( EELMMOUNT );
	}
	PrintPositioned( 15 , 45 , "COMPLETE\n" );
}

//--------------------------------------------------------------------------------
int main(int argc, char **argv)
//--------------------------------------------------------------------------------
{
	Init();

	int i;
	FILE *fp = NULL;

	if(chdir("sd:/sys") != 0)
		mkdir("sd:/sys", 0);
	fp = fopen("sd:/sys/device.cert", "w");
	if ( fp == NULL ) {
		printf( "Error opening device.cert" );
		fatUnmount("sd:");
		WaitForButtonPress();
		ExitToLoader(1);
	}

	if (fp) {
		u8 * devcert = memalign( 32 , 0x200 );
		memset(devcert, 42, 0x200);
		i = ES_GetDeviceCert(devcert);
		if (i) printf("ES_GetDeviceCert returned %d\n", i);
		else {
			fwrite(devcert, 0x180, 1, fp);
		}
		fclose(fp);
		fatUnmount("sd:");
		free(devcert);
	}
	printf("File written, please wait for restart\n");
	sleep(5);
	return 0;
}

