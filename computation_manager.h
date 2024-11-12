#pragma once

#include "task.h"

void CM_Init();
void CM_Shutdown();

Task* CM_NewTask(int x, int limit, char compSymbol);
void CM_Run();
void CM_Cancel(int groupIdx, int componentIdx);
void CM_SetRunning(bool running);
bool CM_IsRunning();

TaskList* CM_GetTasks();