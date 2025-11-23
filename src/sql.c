#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "catalog.h"
#include "heap.h"
#include "buffer.h"
#include "row.h"

static void trim(char* s) {
  char* p = s;
  while (isspace((unsigned char)*p)) p++;
  if (p != s) memmove(s, p, strlen(p)+1);
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = 0;
}
static int starts_with(const char* s, const char* pref) {
  return strncasecmp(s, pref, strlen(pref)) == 0;
}
static int parse_ident_after(const char* line, const char* kw, char* out, size_t out_sz) {
  const char* p = strcasestr(line, kw);
  if (!p) return 0;
  p += strlen(kw);
  while (*p && isspace((unsigned char)*p)) p++;
  size_t i = 0;
  while (*p && (isalnum((unsigned char)*p) || *p == '_' )) {
    if (i + 1 >= out_sz) break;
    out[i++] = *p++;
  }
  out[i] = 0;
  return i > 0;
}

static ColumnType parse_type(const char* t) {
  if (strcasecmp(t, "int") == 0) return COL_INT;
  if (strcasecmp(t, "text") == 0) return COL_TEXT;
  return 0;
}

// Parse: CREATE TABLE t (a INT, b TEXT);
static int parse_create_columns(const char* line, ColumnDef* cols, int max_cols) {
  const char* lpar = strchr(line, '(');
  const char* rpar = strrchr(line, ')');
  if (!lpar || !rpar || rpar <= lpar) return 0;

  char inside[256];
  size_t len = (size_t)(rpar - lpar - 1);
  if (len >= sizeof(inside)) len = sizeof(inside)-1;
  memcpy(inside, lpar+1, len);
  inside[len] = 0;

  int n = 0;
  char* tok = strtok(inside, ",");
  while (tok && n < max_cols) {
    trim(tok);
    // split by space
    char col[COL_NAME_MAX], typ[16];
    if (sscanf(tok, "%31s %15s", col, typ) != 2) return 0;

    ColumnType ct = parse_type(typ);
    if (!ct) return 0;

    memset(&cols[n], 0, sizeof(ColumnDef));
    strncpy(cols[n].col, col, COL_NAME_MAX-1);
    cols[n].type = ct;
    n++;

    tok = strtok(NULL, ",");
  }
  return n;
}

// Parse: INSERT INTO t VALUES (1, 'alice');
static int parse_insert_values(const char* line, char values[][128], int max_vals) {
  char* vals_kw = strcasestr((char*)line, "values");
  if (!vals_kw) return 0;

  char* lpar = strchr(vals_kw, '(');
  char* rpar = strrchr(vals_kw, ')');
  if (!lpar || !rpar || rpar <= lpar) return 0;

  char inside[256];
  size_t len = (size_t)(rpar - lpar - 1);
  if (len >= sizeof(inside)) len = sizeof(inside)-1;
  memcpy(inside, lpar+1, len);
  inside[len] = 0;

  int n = 0;
  char* tok = strtok(inside, ",");
  while (tok && n < max_vals) {
    trim(tok);

    // strip quotes for text
    if (tok[0] == '\'' || tok[0] == '"') {
      size_t m = strlen(tok);
      if (m >= 2) {
        tok[m-1] = 0;
        tok++;
      }
    }

    strncpy(values[n], tok, 127);
    values[n][127] = 0;
    n++;
    tok = strtok(NULL, ",");
  }
  return n;
}

void repl(BufferPool* bp) {
  Catalog cat = catalog_open(bp);
  if (cat.catalog_heap_header_pid == INVALID_PID) return;

  char line[512];
  printf("marqdb> ");

  while (fgets(line, sizeof(line), stdin)) {
    trim(line);
    if (line[0] == 0) { printf("marqdb> "); continue; }

    if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) break;

    if (starts_with(line, "create table")) {
      char tname[TABLE_NAME_MAX];
      if (!parse_ident_after(line, "create table", tname, sizeof(tname))) {
        printf("Parse error.\nmarqdb> "); continue;
      }

      ColumnDef cols[16];
      int ncols = parse_create_columns(line, cols, 16);
      if (ncols <= 0) {
        printf("Parse error. Example: CREATE TABLE t (id INT, name TEXT);\nmarqdb> ");
        continue;
      }

      uint32_t heap_h;
      if (catalog_create_table(bp, &cat, tname, cols, ncols, &heap_h)) {
        printf("OK\n");
      } else {
        printf("Table '%s' already exists in catalog\n", tname);
      }

      printf("marqdb> ");
      continue;
    }

    if (starts_with(line, "insert into")) {
      char tname[TABLE_NAME_MAX];
      if (!parse_ident_after(line, "insert into", tname, sizeof(tname))) {
        printf("Parse error.\nmarqdb> "); continue;
      }

      uint32_t heap_h_pid;
      if (!catalog_find_table(bp, &cat, tname, &heap_h_pid)) {
        printf("No such table.\nmarqdb> "); continue;
      }

      ColumnDef cols[16];
      int ncols = catalog_load_schema(bp, &cat, tname, cols, 16);
      if (ncols <= 0) {
        printf("Schema missing.\nmarqdb> "); continue;
      }

      char vals_raw[16][128];
      int nvals = parse_insert_values(line, vals_raw, 16);
      if (nvals != ncols) {
        printf("Value count mismatch (expected %d).\nmarqdb> ", ncols);
        continue;
      }

      const char* vals[16];
      for (int i = 0; i < nvals; i++) vals[i] = vals_raw[i];

      uint8_t enc[512];
      int enc_len = row_encode(cols, ncols, vals, nvals, enc, sizeof(enc));
      if (enc_len < 0) {
        printf("Encode error.\nmarqdb> "); continue;
      }

      HeapFile hf = heap_open(bp, heap_h_pid);
      heap_insert(bp, &hf, enc, (uint16_t)enc_len);

      printf("OK\nmarqdb> ");
      continue;
    }

    if (starts_with(line, "select *")) {
      char tname[TABLE_NAME_MAX];
      if (!parse_ident_after(line, "from", tname, sizeof(tname))) {
        printf("Parse error.\nmarqdb> "); continue;
      }

      uint32_t heap_h_pid;
      if (!catalog_find_table(bp, &cat, tname, &heap_h_pid)) {
        printf("No such table.\nmarqdb> "); continue;
      }

      ColumnDef cols[16];
      int ncols = catalog_load_schema(bp, &cat, tname, cols, 16);
      if (ncols <= 0) {
        printf("Schema missing.\nmarqdb> "); continue;
      }

      HeapFile hf = heap_open(bp, heap_h_pid);
      RID cur = { .page_id = INVALID_PID, .slot_id = 0 };
      uint8_t* out;
      uint16_t len;
      int count = 0;

      while (heap_scan_next(bp, &hf, &cur, &out, &len)) {
        char linebuf[512];
        if (row_decode(cols, ncols, out, len, linebuf, sizeof(linebuf)) >= 0) {
          puts(linebuf);
        } else {
          puts("<decode error>");
        }
        count++;
        bp_unpin_page(bp, cur.page_id, false);
      }

      printf("(%d rows)\nmarqdb> ", count);
      continue;
    }

    printf("Unknown command.\nmarqdb> ");
  }
}
