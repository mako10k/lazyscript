#pragma once

// Effect discipline helpers used by builtins and runtime

void ls_effects_set_strict(int on);
int  ls_effects_get_strict(void);
void ls_effects_begin(void);
void ls_effects_end(void);
int  ls_effects_allowed(void);
