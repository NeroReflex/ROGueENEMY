#pragma once

#include "input_dev.h"
#include "settings.h"
#include "xbox360.h"

size_t rog_ally_device_def_count(void);

input_dev_t **rog_ally_device_def(const controller_settings_t *const settings);
