#include "group.h"
#include "task.h"
#include <cstdio>

int g_nGroupIdCounter = 0;
GroupList* NewNode(Group* group)
{
	GroupList* node = new GroupList;
	node->group = group;
	node->pNext = NULL;
	return node;
}

void PushGroup(GroupList*& list, Group* group)
{
	if (list == NULL)
	{
		list = NewNode(group);
		return;
	}

	GroupList* current = list;
	while (current->pNext != NULL)
	{
		current = current->pNext;
	}

	current->pNext = NewNode(group);
}

Group* NewGroup(GroupList*& list, int x, double limit)
{
	Group* pGroup = new Group;
	pGroup->idx = g_nGroupIdCounter++;
	pGroup->x = x;
	pGroup->limit = limit;
	pGroup->taskList = NULL;

	PushGroup(list, pGroup);

	return pGroup;
}

// TODO: fix cleanup: delete not only list, but group and task
void CleanUpGroupList(GroupList* list)
{
	while (list != NULL)
	{
		GroupList* temp = list->pNext;

		CleanUpTaskList(list->group->taskList);

		delete list;

		list = temp;
	}
}

Group* GetGroupByIdx(GroupList* list, int idx)
{
	while (list != NULL)
	{
		if (list->group && list->group->idx == idx)
		{
			return list->group;
		}

		list = list->pNext;
	}

	return NULL;
}

Task* GetTaskBySocket(GroupList* list, int sockFd)
{
	while (list != NULL)
	{
		if (list->group)
		{
			Task* task = GetTaskBySocket(list->group->taskList, sockFd);
			if (task)
			{
				return task;
			}
		}

		list = list->pNext;
	}

	return NULL;
}

Task* FindFreeTask(GroupList* list, int& x)
{
	while (list != NULL)
	{
		if (list->group)
		{
			Task* task = FindFreeTask(list->group->taskList);
			if (task)
			{
				x = list->group->x;
				return task;
			}
		}

		list = list->pNext;
	}

	return NULL;
}