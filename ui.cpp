#include "ui.h"
#include "computation_manager.h"
#include "assert.h"

#include <cstdio>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_LEN 128

extern volatile bool g_bGotSigInt;

const char* StatusToString(int status)
{
	switch (status)
	{
	case TASK_STATUS_READY_TO_RUN:
		return "TASK_STATUS_READY_TO_RUN";
	case TASK_STATUS_ERROR:
		return "TASK_STATUS_ERROR";
	case TASK_STATUS_FINISHED:
		return "TASK_STATUS_FINISHED";
	case TASK_STATUS_CALCULATING:
		return "TASK_STATUS_CALCULATING";
	case TASK_STATUS_LIMIT_EXCEEDED:
		return "TASK_STATUS_LIMIT_EXCEEDED";
	case TASK_STATUS_CANCELED_BY_USER:
		return "TASK_STATUS_CANCELED_BY_USER";
	case TASK_STATUS_WAITING_FOR_HELLO_MSG:
		return "TASK_STATUS_WAITING_FOR_HELLO_MSG";
	}

	return "Unknown status";
}


void UI_NewComputation(int argc, char** argv)
{
	char compSymbol;
	if (argc > 1)
	{
		compSymbol = argv[1][0];
	}
	else
	{
		printf("new usage: new <component symbol> <limit time (optional)>\n");
		return;
	}
	
	int limit = 0;
	if (argc > 2)
	{
		limit = atoi(argv[2]);
	}

	Task* task = CM_NewTask(5, limit, compSymbol);

	printf("Computation component %c with idx %d added to group TODO: group\n", compSymbol, task->idx);
}

void UI_NewGroup(int argc, char** argv)
{
	int x;
	if (argc > 1)
	{
		x = atoi(argv[2]);
	}
	else
	{
		printf("group usage: group <x> <limit time (optional)>\n");
		return;
	}

	int limit = 0;
	if (argc > 2)
	{
		limit = atoi(argv[3]);
	}

	// TODO: groups
	printf("New group %d with x = %d\n", 1, x);
}

void UI_RunComputating()
{
	// set ready to run status for all tasks
	TaskList* list = CM_GetTasks();
	while (list != NULL)
	{
		Task* task = list->pTask;
		task->status = TASK_STATUS_READY_TO_RUN;

		list = list->pNext;
	}

	CM_SetRunning(true);
}

void UI_Summary()
{
	TaskList* taskList = CM_GetTasks();

	printf("Summary:\n");
	while (taskList != NULL)
	{
		Task* task = taskList->pTask;

		printf("[Task %d] result: %f, elapsed time: %d, status: %s\n", task->idx, task->result, task->elapsedTime, StatusToString(task->status));

		taskList = taskList->pNext;
	}
}

void UI_Status()
{
	TaskList* taskList = CM_GetTasks();

	printf("Computation status for each task:\n");
	while (taskList != NULL)
	{
		Task* task = taskList->pTask;

		printf("[Task %d] ", task->idx);
		if (task->status == TASK_STATUS_FINISHED)
			printf("result: %f, ", task->result);
		else
			printf("result: not defined, ");

		printf("elapsed time: %d, status: %s\n", task->elapsedTime, StatusToString(task->status));

		taskList = taskList->pNext;
	}
}

void UI_Cancel(int argc, char** argv)
{
	int groupIdx;
	if (argc > 1)
	{
		groupIdx = atoi(argv[1]);
	}
	else
	{
		printf("cancel: <group idx (-1 for all groups, component idx will be ignored)> <component idx (-1 for all components in group)>\n");
		return;
	}

	int componentIdx;
	if (argc > 2 && groupIdx != -1)
	{
		componentIdx = atoi(argv[2]);
	}
	else
	{
		printf("cancel: <group idx (-1 for all groups, component idx will be ignored)> <component idx (-1 for all components in group)>\n");
		return;
	}

	CM_Cancel(groupIdx, componentIdx);
}

void UI_ProcessCommand(char* cmd)
{
	const int MAX_ARGS = 5;
	char* argv[MAX_ARGS];
	int argc = 0;

	// remove newline character
	size_t len = strlen(cmd);
	if (cmd[len - 1] == '\n')
	{
		cmd[len - 1] = '\0';
	}

	// parse command arguments
	char* token = strtok(cmd, " ");
	while (token != NULL && argc < MAX_ARGS)
	{
		argv[argc] = new char[strlen(token) + 1];
		strcpy(argv[argc], token);

		// Move to the next token
		token = strtok(NULL, " ");
		argc++;
	}

	if (argc < 1)
	{
		return;
	}

	if (!strcmp(argv[0], "new"))
	{
		UI_NewComputation(argc, argv);
	}
	else if (!strcmp(argv[0], "group"))
	{
		UI_NewGroup(argc, argv);
	}
	else if (!strcmp(argv[0], "run"))
	{
		UI_RunComputating();
	}
	else if (!strcmp(argv[0], "status"))
	{
		UI_Status();
	}
	else if (!strcmp(argv[0], "summary"))
	{
		UI_Summary();
	}
	else if (!strcmp(argv[0], "cancel"))
	{
		UI_Cancel(argc, argv);
	}
	else
	{
		printf("Unknown command\n");
	}

	// free tokens
	for (int i = 0; i < argc; i++)
	{
		delete[] argv[i];
	}
}

void UI_Run()
{
	// main UI loop

	// Sequence:
	// read user input (blocking)
	// do IPC (non-blocking)
	// Catch SIGINT during IPC and process user input, then continue running

	// Another sequence variant (no need for signal handling):
	// read user input in non-blocking way and process it
	// do IPC using TCP/IP sockets

	printf("Computation manager. Available commands: group, new, run, cancel, summary, status\n");

	while (true)
	{
		// read user input if CM is not running or when received sigint
		if (!CM_IsRunning() || g_bGotSigInt)
		{
			char cmd[MAX_CMD_LEN];
			if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL)
			{
				// fgets fail on sigint
				continue;
			}

			UI_ProcessCommand(cmd);

			g_bGotSigInt = false;
		}

		if (CM_IsRunning())
		{
			CM_Run();
		}
	}
}