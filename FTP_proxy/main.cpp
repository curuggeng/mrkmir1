
#include <stdio.h>
#include <conio.h>

#include "const.h"
#include "fsmsystem.h"
#include "logfile.h"
#include "ProxyControlAuto.h"

#define AUTOMAT_COUNT 1
#define MSGBOX_COUNT 1


/* FSM system instance. */
static FSMSystem sys(AUTOMAT_COUNT, MSGBOX_COUNT);

DWORD WINAPI SystemThread(void *data) {
	
	ProxyControlAuto Control;


	/* Kernel buffer description block */
	/* number of buffer types */
	const uint8 buffClassNo =  4; 
	/* number of buffers of each buffer type */
	uint32 buffsCount[buffClassNo] = { 50, 50, 5000, 10 }; 
	/* buffer size for each buffer type */
 	uint32 buffsLength[buffClassNo] = { 128, 256, 512, 1024}; 
	
	/* Logging setting - to a file in this case */
	LogFile lf("log.log" /*log file name*/, "./log.ini" /* message translator file */);
	LogAutomateNew::SetLogInterface(&lf);

	/* Mandatory kernel initialization */
	printf("[*] Initializing system...\n");
	sys.InitKernel(buffClassNo, buffsCount, buffsLength, 3, Timer1s);

	/* Add automates to the system */
	sys.Add(&Control, ProxyControlAuto_TYPE_ID, 1, true);
	
	Control.Start();

	sys.Start();

	
	return 0;
}

void main(int argc, char* argv[]) {
	
	DWORD thread_id;
	HANDLE thread_handle;

	/* Start operating thread. */
	thread_handle = CreateThread(NULL, 0, SystemThread, NULL, 0, &thread_id);

	/* Wait for end. */
	getch();
	
	/* Notify the system to stop - this causes the thread to finish */
	printf("[*] Stopping system...\n");
	sys.StopSystem();

	/* Free the thread handle */
	CloseHandle(thread_handle);


}