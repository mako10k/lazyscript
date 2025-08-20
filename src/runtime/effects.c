#include "runtime/effects.h"

static int g_effects_strict = 0; // when true, guard side effects
static int g_effects_depth  = 0; // nesting counter for allowed effects

void ls_effects_set_strict(int on) { g_effects_strict = on ? 1 : 0; }
int  ls_effects_get_strict(void) { return g_effects_strict; }
void ls_effects_begin(void) {
  if (g_effects_strict)
    g_effects_depth++;
}
void ls_effects_end(void) {
  if (g_effects_strict && g_effects_depth > 0)
    g_effects_depth--;
}
int  ls_effects_allowed(void) { return !g_effects_strict || g_effects_depth > 0; }
