#pragma once

#include "common/str.h"

// Simple module load cache used by prelude.require
int  ls_modules_is_loaded(const char* path_cstr);
void ls_modules_mark_loaded(const char* path_cstr);
