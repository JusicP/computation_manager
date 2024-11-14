#include "computation_manager.h"
#include "shared.h"

#include <cstdio>
#include <time.h> 
#ifdef WIN32
#include <Windows.h>
#endif

GroupList* g_pGroups;
bool g_bRunning; // is the server running
bool g_bInitialized; // is the server initialized
SOCKET g_SockFd;

#define MAX_CONNECTIONS 128
SOCKET g_ClientSockets[MAX_CONNECTIONS];

void CM_Init()
{
    g_pGroups = NULL;
    g_bRunning = false;
    g_bInitialized = false;
    g_SockFd = INVALID_SOCKET;

    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        g_ClientSockets[i] = INVALID_SOCKET;
    }
}

void CM_Shutdown()
{
    CleanUpGroupList(g_pGroups);
}

Task* CM_NewTask(int groupIdx, double limit, char compSymbol)
{
    // find group and add to task list
    Group* group = GetGroupByIdx(g_pGroups, groupIdx);
    if (!group)
    {
        return NULL;
    }

	return NewTask(group->taskList, TASK_STATUS_READY_TO_RUN, limit, compSymbol);
}

Group* CM_NewGroup(int x, double limit)
{
    return NewGroup(g_pGroups, x, limit);
}

void CM_StartProcess(Task* task)
{
    char cmdLine[128];
    snprintf(cmdLine, sizeof(cmdLine), "%s %s", ::GetCommandLine(), "worker");

	// create child process
    // possible problem: SIGINT sent to parent process, is also sent to child, causing it to terminate
#ifdef WIN32
	BOOL bSuccess = FALSE;
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);

	bSuccess = CreateProcess(NULL,
        cmdLine,       // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	if (!bSuccess)
	{
		printf("CreateProcess failed with code: %d\n", GetLastError());
		ASSERT(false)
	}
	else
	{
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
#else
#error
#endif
}

void CM_CloseSocket(Task* task)
{
    if (task->sockFd != INVALID_SOCKET)
    {
        closesocket(task->sockFd);
        task->sockFd = INVALID_SOCKET;
    }
}

void CM_InitServer()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return;
    }

    g_SockFd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(g_SockFd != -1);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MGR_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_SockFd, (sockaddr*)&addr, sizeof(sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(g_SockFd, SOMAXCONN) == -1)
    {
        perror("listen");
        exit(1);
    }

    // change the socket into non-blocking state
    u_long iMode = 1;
    if (ioctlsocket(g_SockFd, FIONBIO, &iMode) == -1)
    {
        perror("ioctlsocket");
        exit(1);
    }
}

void CM_ShutdownServer()
{
    if (g_SockFd == INVALID_SOCKET)
    {
        // server is already down
        return;
    }

    closesocket(g_SockFd);
    g_SockFd = INVALID_SOCKET;

    WSACleanup();
}

void CM_FinishTask(Task* task)
{
    task->status = TASK_STATUS_FINISHED;
    task->elapsedTime = task->clientElapsedTime;

    printf("Task %d finished with result: %f\n", task->idx, task->result);
}

// The sequence of send messages mgr <-> worker
// [Worker] Send hello message
// [Mgr] Send task to the worker if there is one (task)
// [Worker] Send task result (or send nothing on error) and disconnect
void CM_RunServer()
{
    if (!g_bInitialized)
    {
        CM_InitServer();
        g_bInitialized = true;
    }

    SOCKET newFd = accept(g_SockFd, NULL, NULL);
    int error = WSAGetLastError();
    if (newFd >= 0)
    {
        // set non-blocking for new socket
        u_long iMode = 1;
        ioctlsocket(newFd, FIONBIO, &iMode);

        // add the new client socket to the array
        bool added = false;
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            if (g_ClientSockets[i] == INVALID_SOCKET)
            {
                g_ClientSockets[i] = newFd;
                added = true;
                break;
            }
        }
        if (!added)
        {
            printf("Max clients reached, closing connection.\n");
            closesocket(newFd);
        }
    }
    else if (error != WSAEWOULDBLOCK)
    {
        perror("accept");
        exit(1);
    }

    for (int i = 0; i <= MAX_CONNECTIONS; i++)
    {
        if (g_ClientSockets[i] == INVALID_SOCKET)
        {
            continue;
        }

        SOCKET socket = g_ClientSockets[i];
        char buf[128];
        int n = recv(socket, buf, sizeof(buf), 0);
        if (n > 0) // got message
        {
            // if no task by socket, then we got probably inital hello message
            Task* task = GetTaskBySocket(g_pGroups, socket);
            if (!task)
            {
                // read hello msg and find free task, assign socket to task
                if (strcmp(buf, "HELLO"))
                {
                    printf("Unknown message\n");
                    continue;
                }

                int x;
                task = FindFreeTask(g_pGroups, x);
                ASSERT(task); // could be zero if run the program manually with the worker argument
                task->sockFd = socket;

                // send task message
                TaskMsg msg;
                msg.x = x;
                msg.componentSymbol = task->componentSymbol;
                if (send(socket, (const char*)&msg, sizeof(msg), 0) == -1)
                {
                    perror("send");
                    CM_CloseSocket(task);
                    g_ClientSockets[i] = INVALID_SOCKET;
                    task->status = TASK_STATUS_ERROR;
                    printf("Task %d cancelled: got error\n", task->idx);
                    continue;
                }

                task->status = TASK_STATUS_CALCULATING;

                continue;
            }

            // read calculation results
            ResultMsg resultMsg = *(ResultMsg*)buf;
            task->result = resultMsg.result;
            task->clientElapsedTime = resultMsg.elapsedTime;

            if (!task->secondChance)
            {
                CM_FinishTask(task);
            }

            CM_CloseSocket(task);
            g_ClientSockets[i] = INVALID_SOCKET;
        }
        else if (n == 0 || WSAGetLastError() != WSAEWOULDBLOCK) // client disconnected or we got socket error
        {
            Task* task = GetTaskBySocket(g_pGroups, socket);
            if (task)
            {
                CM_CloseSocket(task);

                if (task->status == TASK_STATUS_CALCULATING || task->status == TASK_STATUS_WAITING_FOR_HELLO_MSG)
                {
                    // set error status for worker from which a message is expected
                    task->status = TASK_STATUS_ERROR;
                    printf("Task %d cancelled: got error\n", task->idx);
                }
            }
            else
            {
                closesocket(socket);
            }

            g_ClientSockets[i] = INVALID_SOCKET;
        }
	}
}

bool CM_ShouldRun()
{
    // CM should run when some task is ready to run or calculation component is active (calculating)
    GroupList* list = g_pGroups;
    while (list != NULL)
    {
        Group* group = list->group;
        ASSERT(group);

		TaskList* taskList = group->taskList;
		while (taskList != NULL)
		{
			Task* task = taskList->pTask;
			ASSERT(task);

            if (task->status == TASK_STATUS_READY_TO_RUN ||
                task->status == TASK_STATUS_WAITING_FOR_HELLO_MSG ||
                task->status == TASK_STATUS_CALCULATING)
            {
                return true;
            }

            taskList = taskList->pNext;
		}

        list = list->pNext;
    }

    return false;
}

void CM_Run()
{
    if (!CM_ShouldRun())
    {
        if (g_bInitialized)
        {
            CM_ShutdownServer();
            g_bInitialized = false;
        }

        CM_SetRunning(false);
        return;
    }

    // process task status
    GroupList* list = g_pGroups;
    while (list != NULL)
    {
        Group* group = list->group;
        ASSERT(group);

        double maxElapsedTime = 0.0;
        bool secondChance = false;
        TaskList* taskList = group->taskList;
        while (taskList != NULL)
        {
            Task* task = taskList->pTask;
            ASSERT(task);

            if (task->status == TASK_STATUS_READY_TO_RUN)
            {
                CM_StartProcess(task);
                task->status = TASK_STATUS_WAITING_FOR_HELLO_MSG;
            }
            else if (task->status == TASK_STATUS_CALCULATING)
            {
                // update task time counter
                if (task->startTime == 0)
                {
                    task->startTime = clock();
                }

                task->elapsedTime = (double)(clock() - task->startTime) / CLOCKS_PER_SEC;

                if (task->limit > 0 && task->limit <= task->elapsedTime)
                {
                    // Since there may be interruptions for user input, the task execution time is calculated incorrectly.
                    // Then, give the worker a second chance. 
                    // If there was i/o, it makes sense to check whether the result has arrived.
                    // Result contains client-side time of task execution, which can be used to check timeout
                    if (!task->secondChance)
                    {
                        task->secondChance = true;
                        secondChance = true;
                    }
                    else if (task->clientElapsedTime != -1.0 && task->limit > task->clientElapsedTime)
                    {
                        CM_FinishTask(task);
                    }
                    else
                    {
                        task->status = TASK_STATUS_LIMIT_EXCEEDED;

                        // don't receive message from this worker because he's late
                        CM_CloseSocket(task);

                        printf("Task %d cancelled: time limit exceeded\n", task->idx);
                    }
                }

                if (maxElapsedTime < task->elapsedTime)
                {
                    maxElapsedTime = task->elapsedTime;
                }
            }
            taskList = taskList->pNext;
        }

        // check limit for group
        if (maxElapsedTime > 0.0 && !secondChance)
        {
            group->elapsedTime = maxElapsedTime;

            if (group->limit > 0 && group->limit <= group->elapsedTime)
            {
                // finish all tasks
                TaskList* taskList = group->taskList;
                while (taskList != NULL)
                {
                    Task* task = taskList->pTask;
                    ASSERT(task);

                    if (task->status == TASK_STATUS_CALCULATING ||
                        task->status == TASK_STATUS_WAITING_FOR_HELLO_MSG)
                    {
                        task->status = TASK_STATUS_LIMIT_EXCEEDED;
                        CM_CloseSocket(task);

                        printf("Task %d cancelled: global group %d time limit exceeded\n", task->idx, group->idx);
                    }
                    taskList = taskList->pNext;
                }
            }
        }

        list = list->pNext;
    }

    CM_RunServer();
}

void CM_Cancel(int groupIdx, int componentIdx)
{
    GroupList* list = g_pGroups;
    while (list != NULL)
    {
        Group* group = list->group;
        ASSERT(group);

        if (groupIdx == -1 || group->idx == groupIdx)
        {
            TaskList* taskList = group->taskList;
            while (taskList != NULL)
            {
                Task* task = taskList->pTask;
                ASSERT(task);

                // set canceled status for running task
                if ((componentIdx == -1 || task->idx == componentIdx) &&
                    task->status == TASK_STATUS_CALCULATING &&
                    task->status == TASK_STATUS_WAITING_FOR_HELLO_MSG)
                {
                    CM_CloseSocket(task);
                    task->status = TASK_STATUS_CANCELED_BY_USER;
                }

                taskList = taskList->pNext;
            }
        }

        list = list->pNext;
    }
}

void CM_SetRunning(bool running)
{
    g_bRunning = running;
}

bool CM_IsRunning()
{
	return g_bRunning;
}

GroupList* CM_GetGroups()
{
    return g_pGroups;
}