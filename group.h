#pragma once

struct Group
{
	int idx;
	int x; // function argument
	double limit;
	struct TaskList* taskList;

	double startTime;
	double elapsedTime;
};

struct GroupList
{
	Group* group;
	GroupList* pNext;
};

struct Task;

Group* NewGroup(GroupList*& list, int x, double limit);
void CleanUpGroupList(GroupList* list);
Group* GetGroupByIdx(GroupList* list, int idx);
Task* GetTaskBySocket(GroupList* list, int sockFd);
Task* FindFreeTask(GroupList* list, int& x);