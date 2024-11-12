cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("computation_manager")

add_executable(computation_manager "computation_manager.cpp" "computation_manager.h" "main.cpp" "task.cpp" "task.h" "ui.cpp" "ui.h" "worker.cpp" "worker.h"  "shared.h")

target_link_libraries(computation_manager wsock32 ws2_32)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET computation_manager PROPERTY CXX_STANDARD 20)
endif()
