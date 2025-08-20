#include "runtime/trace.h"
#include "common/io.h"
#include "common/malloc.h"
#include <stdlib.h>
#include <string.h>

lstrace_table_t* g_lstrace_table = NULL;
// Simple global stack for current trace id (process-wide for now)
static int   g_trace_stack[256];
static int   g_trace_top = 0;
// Optional JSONL dump state
static FILE* g_trace_dump_fp = NULL;

static void free_table(lstrace_table_t* t) {
  if (!t) return;
  if (t->spans) {
    // filenames are pointers into allocated copies; free each unique? Here we strdup per line.
    for (int i = 0; i < t->count; i++) {
      // filenames were strdup'ed
      if (t->spans[i].filename) free((void*)t->spans[i].filename);
    }
    free(t->spans);
  }
  free(t);
}

void lstrace_free(void) {
  free_table(g_lstrace_table);
  g_lstrace_table = NULL;
  g_trace_top = 0;
  if (g_trace_dump_fp) { fclose(g_trace_dump_fp); g_trace_dump_fp = NULL; }
}

static int parse_int(const char* s, int* out) {
  if (!s || !*s) return -1;
  char* end = NULL;
  long  v   = strtol(s, &end, 10);
  if (end == s) return -1;
  *out = (int)v;
  return 0;
}

// very small JSONL parser tailored to our schema to avoid adding deps
int lstrace_load_jsonl(const char* path) {
  FILE* fp = fopen(path, "r");
  if (!fp) return -1;
  lstrace_free();
  // First pass: count lines
  int  count = 0;
  int  ch;
  while ((ch = fgetc(fp)) != EOF) {
    if (ch == '\n') count++;
  }
  if (count == 0) {
    fclose(fp);
    return 0;
  }
  rewind(fp);
  lstrace_table_t* t = (lstrace_table_t*)calloc(1, sizeof(lstrace_table_t));
  t->spans           = (lstrace_span_t*)calloc(count, sizeof(lstrace_span_t));
  t->count           = count;
  char*  line        = NULL;
  size_t n           = 0;
  int    idx         = 0;
  while (getline(&line, &n, fp) != -1) {
    // simplistic field extraction
    // find "src":"..."
    char *p = strstr(line, "\"src\"");
    char *q = p ? strchr(p, '"') : NULL; // first quote
    if (q) q = strchr(q + 1, '"'); // second quote after key
    const char* src_start = NULL;
    const char* src_end   = NULL;
    if (q) {
      char* colon = strchr(q + 1, ':');
      if (colon) {
        char* first = strchr(colon + 1, '"');
        if (first) {
          char* second = strchr(first + 1, '"');
          if (second) {
            src_start = first + 1;
            src_end   = second;
          }
        }
      }
    }
    // extract numbers for sl,sc,el,ec (order-agnostic search)
    int sl = 1, sc = 1, el = 1, ec = 1;
    char* k = line;
    while ((k = strstr(k, "\"sl\"")) != NULL) {
      char* colon = strchr(k, ':');
      if (colon) { parse_int(colon + 1, &sl); }
      break;
    }
    k = line;
    while ((k = strstr(k, "\"sc\"")) != NULL) {
      char* colon = strchr(k, ':');
      if (colon) { parse_int(colon + 1, &sc); }
      break;
    }
    k = line;
    while ((k = strstr(k, "\"el\"")) != NULL) {
      char* colon = strchr(k, ':');
      if (colon) { parse_int(colon + 1, &el); }
      break;
    }
    k = line;
    while ((k = strstr(k, "\"ec\"")) != NULL) {
      char* colon = strchr(k, ':');
      if (colon) { parse_int(colon + 1, &ec); }
      break;
    }
    if (src_start && src_end && src_end > src_start) {
      size_t len = (size_t)(src_end - src_start);
      char*  fn  = (char*)malloc(len + 1);
      memcpy(fn, src_start, len);
      fn[len] = '\0';
      t->spans[idx].filename     = fn;
      t->spans[idx].first_line   = sl;
      t->spans[idx].first_column = sc;
      t->spans[idx].last_line    = el;
      t->spans[idx].last_column  = ec;
    } else {
      t->spans[idx].filename     = strdup("<unknown>");
      t->spans[idx].first_line   = sl;
      t->spans[idx].first_column = sc;
      t->spans[idx].last_line    = el;
      t->spans[idx].last_column  = ec;
    }
    idx++;
  }
  if (line) free(line);
  fclose(fp);
  g_lstrace_table = t;
  return 0;
}

lstrace_span_t lstrace_lookup(int index) {
  lstrace_span_t d = { .filename = "<unknown>", .first_line = 1, .first_column = 1, .last_line = 1, .last_column = 1 };
  if (!g_lstrace_table) return d;
  if (index < 0 || index >= g_lstrace_table->count) return d;
  return g_lstrace_table->spans[index];
}

void lstrace_print_frame(FILE* fp, lstrace_span_t s) {
  fprintf(fp, "%s:%d:%d", s.filename ? s.filename : "<unknown>", s.first_line, s.first_column);
}

int lstrace_current(void) {
  if (g_trace_top <= 0) return -1;
  return g_trace_stack[g_trace_top - 1];
}

void lstrace_push(int id) {
  if (g_trace_top < (int)(sizeof(g_trace_stack) / sizeof(g_trace_stack[0])))
    g_trace_stack[g_trace_top++] = id;
}

void lstrace_pop(void) {
  if (g_trace_top > 0) g_trace_top--;
}

void lstrace_print_stack(FILE* fp, int max_depth) {
  if (!fp) return;
  if (max_depth <= 0) return;
  if (!g_lstrace_table) return;
  int printed = 0;
  for (int i = g_trace_top - 1; i >= 0 && printed < max_depth; --i) {
    int id = g_trace_stack[i];
    fprintf(fp, "\n at ");
    lstrace_print_frame(fp, lstrace_lookup(id));
    printed++;
  }
}

int lstrace_begin_dump(const char* path) {
  if (g_trace_dump_fp) { fclose(g_trace_dump_fp); g_trace_dump_fp = NULL; }
  if (!path || !*path) return -1;
  g_trace_dump_fp = fopen(path, "w");
  return g_trace_dump_fp ? 0 : -1;
}

// minimal JSON string escaper for file names (escape \ and ")
static void fprint_json_string(FILE* fp, const char* s) {
  fputc('"', fp);
  for (const unsigned char* p = (const unsigned char*)s; p && *p; ++p) {
    if (*p == '"' || *p == '\\') { fputc('\\', fp); fputc(*p, fp); }
    else if (*p == '\n') { fputs("\\n", fp); }
    else { fputc(*p, fp); }
  }
  fputc('"', fp);
}

void lstrace_emit_loc(lsloc_t loc) {
  if (!g_trace_dump_fp) return;
  fputc('{', g_trace_dump_fp);
  fputs("\"src\":", g_trace_dump_fp);
  fprint_json_string(g_trace_dump_fp, loc.filename ? loc.filename : "<unknown>");
  fprintf(g_trace_dump_fp, ",\"sl\":%d,\"sc\":%d,\"el\":%d,\"ec\":%d}",
          loc.first_line, loc.first_column, loc.last_line, loc.last_column);
  fputc('\n', g_trace_dump_fp);
  fflush(g_trace_dump_fp);
}

void lstrace_end_dump(void) {
  if (g_trace_dump_fp) { fclose(g_trace_dump_fp); g_trace_dump_fp = NULL; }
}
