#pragma once

struct Group
{
	int idx;
	int x; // function argument
	int limit;
	struct TaskList* taskList;
};

struct GroupList
{
	Group* group;
	GroupList* pNext;
};

struct Task;

Group* NewGroup(GroupList*& list, int x, int limit);
void CleanUpGroupList(GroupList* list);
Group* GetGroupByIdx(GroupList* list, int idx);
Task* GetTaskBySocket(GroupList* list, int sockFd);
Task* FindFreeTask(GroupList* list, int& x);