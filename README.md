# Computation manager

## Overview
Computation manager (CM) is a program that manages computation components. These components are combined into groups.
Each component is a separate process (worker), which is created by the manager as a child.
Communication between the CM and worker processes (IPC) occurs via network sockets.
All this happens in a non-blocking manner, allowing you to do other things in addition to communication.

## UI
Interaction between the user and the CM occurs through the console.

### Console commands
- `group <x> <limit time (optional, 0 for no limit)>`.
Creates a new group with the specified integer argument `x` and time limit.

- `new <groupIdx> <component symbol> <limit time (optional)>`.
Creates a new computation component with the specified component symbol (function) and time limit.

- `run`.
Runs CM. During running, messages about completion, errors and time limits exceeded are output to the console.
At this time, user input is only available via interruption (CTRL+C).

- `cancel <group idx (-1 for all groups, component idx will be ignored)> <component idx (-1 for all components in group)>`.
Cancels computations for a group/component. You can also cancel computations for all groups or for all components in a group.

- `status`.
Displays the current computation status.
This is the result, task status, and running time.

During computations, the user is given the ability to view the current computation status, add new groups, calculations (as well as cancel them).
To do this, interrupt the process with the CTRL+C key combination and enter the desired command.

Examples of available functions for calculation:
- `h`. Calculates the sum from 1 to `x` of the product of the square root of k by the logarithm of k.
- `g`. Calculates the sum from 1 to `x` of the division of the square root of k by the logarithm of k.

## Calculation component statuses
They can be found in the `task.h` file.
- `TASK_STATUS_ERROR`. An error occurred while the computation component was running.
- `TASK_STATUS_READY_TO_RUN`. The computation component is ready to run.
- `TASK_STATUS_WAITING_FOR_HELLO_MSG`. CM is waiting for an initial message from the computation component that indicates that the component is waiting for a task.
- `TASK_STATUS_CALCULATING`. The component is performing computations, the CM is waiting for the result.
- `TASK_STATUS_LIMIT_EXCEEDED`. The component has exceeded the time limit specified by the user.
- `TASK_STATUS_CANCELED_BY_USER`. The computation has been canceled by the user.
- `TASK_STATUS_FINISHED`. The component returned the result.

If at least one task is in one of these states:
- `TASK_STATUS_READY_TO_RUN`
- `TASK_STATUS_WAITING_FOR_HELLO_MSG`
- `TASK_STATUS_CALCULATING`

then the CM continues its work. Otherwise, control is transferred to the UI.

## Calculating the work time
The component (work process) and the CM keep their own work time counters (global and local). Why?
Because the CM can be interrupted by the user, which will cause the time to be calculated incorrectly.
Exceeding the global counter does not guarantee that the component is late.
Therefore, the CM tries to get the result from components. If there is none, then the component is still performing computations.
To synchronize time, the component sends its work time along with the result.

## Screenshots
Group creation:

![group](/screenshots/group.jpg)

Component creation:

![new](/screenshots/new.jpg)

Running:

![run](/screenshots/run.jpg)

Cancellation:

![cancel](/screenshots/cancel.jpg)

Group time limit:

![group_time_limit](/screenshots/group_time_limit.jpg)

Multiple groups:

![multiple_groups](/screenshots/multiple_groups.jpg)
