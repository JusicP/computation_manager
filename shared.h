#pragma once

#define ASSERT(expr) \
    if (!(expr)) { \
        printf("Assertaion failed in %s at %d\n", __FILE__, __LINE__); \
        *(int*)0 = NULL; \
    }

#define MGR_PORT 7777

// task message sent to workers
struct TaskMsg
{
    int x;
    char componentSymbol;
};

struct ResultMsg
{
    double result;
    double elapsedTime;
};