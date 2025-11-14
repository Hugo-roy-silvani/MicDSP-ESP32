#pragma once
#include <stdbool.h>

extern volatile bool filter_enabled;

void switch_monitor_task(void *arg);
