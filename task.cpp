#include "task.h"
#include <cstdio>

int g_nTaskIdCounter = 0;

TaskList* NewNode(Task* task)
{
	TaskList* node = new TaskList;
	node->pTask = task;
	node->pNext = NULL;
	return node;
}

void PushTask(TaskList*& list, Task* task)
{
	if (list == NULL)
	{
		list = NewNode(task);
		return;
	}

	TaskList* current = list;
	while (current->pNext != NULL)
	{
		current = current->pNext;
	}

	current->pNext = NewNode(task);
}

Task* NewTask(TaskList*& list, int status, double limit, char compSymbol)
{
	Task* pTask = new Task;
	pTask->status = status;
	pTask->limit = limit;
	pTask->idx = g_nTaskIdCounter++;
	pTask->componentSymbol = compSymbol;
	pTask->elapsedTime = 0.0;
	pTask->startTime = 0.0;
	pTask->clientElapsedTime = -1.0;
	pTask->secondChance = false;

	PushTask(list, pTask);

	return pTask;
}

void CleanUpTaskList(TaskList* head)
{
	while (head != NULL)
	{
		TaskList* temp = head->pNext;

		delete head;

		head = temp;
	}
}

Task* GetTaskBySocket(TaskList* head, int sockFd)
{
	while (head != NULL)
	{
		if (head->pTask->sockFd == sockFd)
			return head->pTask;

		head = head->pNext;
	}

	return NULL;
}

Task* FindFreeTask(TaskList* head)
{
	while (head != NULL)
	{
		if (head->pTask->status == TASK_STATUS_WAITING_FOR_HELLO_MSG)
			return head->pTask;

		head = head->pNext;
	}

	return NULL;
}