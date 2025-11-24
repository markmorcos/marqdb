#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "catalog.h"
#include "heap.h"
#include "buffer.h"
#include "row.h"
#include "sql.h"

// ============================================================================
// String Utility Functions
// ============================================================================

void sql_trim(char* s) {
  char* p = s;
  while (isspace((unsigned char)*p)) p++;
  if (p != s) memmove(s, p, strlen(p)+1);
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = 0;
}

int sql_starts_with(const char* s, const char* pref) {
  return strncasecmp(s, pref, strlen(pref)) == 0;
}

// ============================================================================
// SQL Parsing Functions
// ============================================================================

int sql_parse_ident_after(const char* line, const char* kw, char* out, size_t out_sz) {
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

int sql_parse_create_columns(const char* line, ColumnDef* cols, int max_cols) {
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
    sql_trim(tok);
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

int sql_parse_insert_values(const char* line, char values[][128], int max_vals) {
  char* vals_kw = strcasestr((char*)line, "values");
  if (!vals_kw) return 0;

  char* lpar = strchr(vals_kw, '(');
  char* rpar = strrchr(vals_kw, ')');
  if (!lpar || !rpar || rpar <= lpar) return 0;

  char inside[256];
  size_t len = (size_t)(rpar - lpar - 1);
  if (len >= sizeof(inside)) len = sizeof(inside) - 1;
  memcpy(inside, lpar + 1, len);
  inside[len] = 0;

  int n = 0;
  char* tok = strtok(inside, ",");
  while (tok && n < max_vals) {
    sql_trim(tok);

    if (tok[0] == '\'' || tok[0] == '"') {
      size_t m = strlen(tok);
      if (m >= 2) {
        tok[m - 1] = 0;
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

int sql_parse_where_clause(const char* line, Filter* f) {
  const char* p = strcasestr(line, "where");
  if (!p) return 0;

  p += 5;
  while (*p && isspace((unsigned char)*p)) p++;

  char col[COL_NAME_MAX], op[4], val[128];
  if (sscanf(p, "%31s %3s %127[^\n]", col, op, val) != 3) return -1;

  memset(f, 0, sizeof(Filter));
  strncpy(f->col, col, COL_NAME_MAX - 1);

  if (strcmp(op, "=") == 0) f->op = OP_EQ;
  else if (strcmp(op, ">") == 0) f->op = OP_GT;
  else if (strcmp(op, "<") == 0) f->op = OP_LT;
  else return -1;

  char* v = val;
  sql_trim(v);
  size_t L = strlen(v);
  if (L > 0 && v[L-1] == ';') v[L-1] = 0;
  
  L = strlen(v);
  if ((v[0] == '\'' || v[0] == '"') && L >= 2) {
    v[L - 1] = 0;
    v++;
  }
  strncpy(f->value, v, sizeof(f->value) - 1);

  return 1;
}

int sql_filter_match(const Filter* f, const char linebuf[]) {
  char target[128];
  snprintf(target, sizeof(target), "%s=", f->col);

  const char* p = strstr(linebuf, target);
  if (!p) return 0;

  p += strlen(target);

  char actual[128];
  int i = 0;
  while (*p && *p != '|' && *p != ' ' && i < 127) actual[i++] = *p++;
  actual[i] = 0;

  switch (f->op) {
    case OP_EQ: return strcmp(actual, f->value) == 0;
    case OP_GT: return atoi(actual) > atoi(f->value);
    case OP_LT: return atoi(actual) < atoi(f->value);
  }
  
  return 0;
}

// ============================================================================
// SQL Command Execution Functions
// ============================================================================

int sql_exec_create_table(BufferPool* bp, Catalog* cat, const char* line) {
  char tname[TABLE_NAME_MAX];
  if (!sql_parse_ident_after(line, "create table", tname, sizeof(tname))) {
    printf("Parse error.\n");
    return 0;
  }

  ColumnDef cols[16];
  int ncols = sql_parse_create_columns(line, cols, 16);
  if (ncols <= 0) {
    printf("Parse error. Example: CREATE TABLE t (id INT, name TEXT);\n");
    return 0;
  }

  uint32_t heap_h;
  if (catalog_create_table(bp, cat, tname, cols, ncols, &heap_h)) {
    printf("Table '%s' created successfully.\n", tname);
    return 1;
  } else {
    printf("Table '%s' already exists.\n", tname);
    return 0;
  }
}

int sql_exec_insert(BufferPool* bp, Catalog* cat, const char* line) {
  char tname[TABLE_NAME_MAX];
  if (!sql_parse_ident_after(line, "insert into", tname, sizeof(tname))) {
    printf("Parse error.\n");
    return 0;
  }

  uint32_t heap_h_pid;
  if (!catalog_find_table(bp, cat, tname, &heap_h_pid)) {
    printf("Table '%s' does not exist.\n", tname);
    return 0;
  }

  ColumnDef cols[16];
  int ncols = catalog_load_schema(bp, cat, tname, cols, 16);
  if (ncols <= 0) {
    printf("Schema missing for table '%s'.\n", tname);
    return 0;
  }

  char vals_raw[16][128];
  int nvals = sql_parse_insert_values(line, vals_raw, 16);
  if (nvals != ncols) {
    printf("Value count mismatch (expected %d, got %d).\n", ncols, nvals);
    return 0;
  }

  const char* vals[16];
  for (int i = 0; i < nvals; i++) vals[i] = vals_raw[i];

  uint8_t enc[512];
  int enc_len = row_encode(cols, ncols, vals, nvals, enc, sizeof(enc));
  if (enc_len < 0) {
    printf("Failed to encode row.\n");
    return 0;
  }

  HeapFile hf = heap_open(bp, heap_h_pid);
  heap_insert(bp, &hf, enc, (uint16_t)enc_len);

  printf("1 row inserted.\n");
  return 1;
}

int sql_exec_select(BufferPool* bp, Catalog* cat, const char* line) {
  char tname[TABLE_NAME_MAX];
  if (!sql_parse_ident_after(line, "from", tname, sizeof(tname))) {
    printf("Parse error.\n");
    return -1;
  }

  uint32_t heap_h_pid;
  if (!catalog_find_table(bp, cat, tname, &heap_h_pid)) {
    printf("Table '%s' does not exist.\n", tname);
    return -1;
  }

  ColumnDef cols[16];
  int ncols = catalog_load_schema(bp, cat, tname, cols, 16);
  if (ncols <= 0) {
    printf("Schema missing for table '%s'.\n", tname);
    return -1;
  }

  Filter flt;
  int has_filter = sql_parse_where_clause(line, &flt);
  if (has_filter < 0) {
    printf("WHERE clause parse error.\n");
    return -1;
  }

  HeapFile hf = heap_open(bp, heap_h_pid);
  RID cur = { .page_id = INVALID_PID, .slot_id = 0 };
  uint8_t* out;
  uint16_t len;
  int count = 0;

  while (heap_scan_next(bp, &hf, &cur, &out, &len)) {
    char linebuf[512];
    if (row_decode(cols, ncols, out, len, linebuf, sizeof(linebuf)) >= 0) {
      if (has_filter == 1 && !sql_filter_match(&flt, linebuf)) {
        bp_unpin_page(bp, cur.page_id, false);
        continue;
      }

      puts(linebuf);
      count++;
    }

    bp_unpin_page(bp, cur.page_id, false);
  }

  printf("(%d row%s)\n", count, count == 1 ? "" : "s");
  return count;
}

// ============================================================================
// REPL
// ============================================================================

void repl(BufferPool* bp) {
  Catalog cat = catalog_open(bp);
  if (cat.catalog_heap_header_pid == INVALID_PID) {
    fprintf(stderr, "Failed to open catalog.\n");
    return;
  }

  printf("MarqDB - Type .help for commands\n");
  char line[512];

  while (1) {
    printf("marqdb> ");
    
    if (!fgets(line, sizeof(line), stdin)) break;
    
    sql_trim(line);
    if (line[0] == 0) continue;

    // Meta commands
    if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
      printf("Goodbye!\n");
      break;
    }
    
    if (strcmp(line, ".help") == 0) {
      printf("Commands:\n");
      printf("  CREATE TABLE name (col1 TYPE1, col2 TYPE2, ...);\n");
      printf("  INSERT INTO name VALUES (val1, val2, ...);\n");
      printf("  SELECT * FROM name [WHERE col = value];\n");
      printf("  .exit / .quit  - Exit the database\n");
      printf("  .help          - Show this help message\n");
      continue;
    }

    // SQL commands
    if (sql_starts_with(line, "create table")) {
      sql_exec_create_table(bp, &cat, line);
    } else if (sql_starts_with(line, "insert into")) {
      sql_exec_insert(bp, &cat, line);
    } else if (sql_starts_with(line, "select *")) {
      sql_exec_select(bp, &cat, line);
    } else {
      printf("Unknown command. Type .help for available commands.\n");
    }
  }
}