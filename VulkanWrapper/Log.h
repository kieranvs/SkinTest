#pragma once

#include <stdio.h>
#include <stdlib.h>

inline void log_warning(const char* message)
{
    printf(message);
}

inline void log_error(const char* message)
{
    printf(message);
    exit(-1);
}
