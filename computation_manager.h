﻿#pragma once

#include "task.h"
#include "group.h"

void CM_Init();
void CM_Shutdown();

Group* CM_NewGroup(int x, int limit);
Task* CM_NewTask(int groupIdx, int limit, char compSymbol);
void CM_Run();
void CM_Cancel(int groupIdx, int componentIdx);
void CM_SetRunning(bool running);
bool CM_IsRunning();

GroupList* CM_GetGroups();
