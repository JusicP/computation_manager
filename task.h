#pragma once

struct Task
{
	int idx;
	int x; // function argument
	char componentSymbol; // component symbol - defines calculations
	int status; // task status
	int limit; // maximum amount of time can be allocated to calculations

	// for summary
	int elapsedTime;
	double result;

	// connection info
	int sockFd;
};

// Task statuses
// They determine computation manager behavior.
// Before CM launch, all tasks are assigned the TASK_STATUS_READY_TO_RUN status to create processes.
// Tasks for which a worker process has been created are assigned the TASK_STATUS_WAITING_FOR_HELLO_MSG status.
// If all tasks have one of these statuses: 
// TASK_STATUS_ERROR, TASK_STATUS_LIMIT_EXCEEDED, TASK_STATUS_CANCELED_BY_USER, TASK_STATUS_FINISHED,
// then CM closes the server and calls SetRunning(false)
#define TASK_STATUS_ERROR -1 // task failed with error
#define TASK_STATUS_READY_TO_RUN 0 // task is ready to run (CM must create worker process)
#define TASK_STATUS_CALCULATING 1 // assigned after sending task info to worker
#define TASK_STATUS_LIMIT_EXCEEDED 2 // task execution time exceeded the limit
#define TASK_STATUS_CANCELED_BY_USER 3 // task cancelled by executing "cancel" command
#define TASK_STATUS_FINISHED 4 // worker returned result of task
#define TASK_STATUS_WAITING_FOR_HELLO_MSG 5 // CM is waiting for worker hello message

struct TaskList
{
	Task* pTask;
	TaskList* pNext;
};

TaskList* NewNode(Task* task);
Task* NewTask(TaskList*& list, int x, int status, int limit, char compSymbol);
void CleanUpTaskList(TaskList* head);
Task* GetTaskBySocket(TaskList* head, int sockFd);
Task* FindFreeTask(TaskList* head);