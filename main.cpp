#include "computation_manager.h"
#include "ui.h"
#include "worker.h"

#include <signal.h>
#include <string.h>

volatile bool g_bGotSigInt = false; // if sigint is received, then ui can process user input
void Signal_Int(int)
{
	g_bGotSigInt = true;

	signal(SIGINT, Signal_Int);
}

int main(int argc, char** argv)
{
	// condition to run worker process
	if (argc > 1 && !strcmp(argv[1], "worker"))
	{
		// ignore signint, since parent process handles it in its own way and it's also passed to child processes
		signal(SIGINT, SIG_IGN);
		return Worker_Run();
	}

	// handle sigint signal to allow user input during IPC
	signal(SIGINT, Signal_Int);

	CM_Init();
	UI_Run();
	CM_Shutdown();

	return 0;
}
