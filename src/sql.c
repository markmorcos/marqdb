#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "catalog.h"
#include "heap.h"
#include "buffer.h"

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
  while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
    if (i + 1 >= out_sz) break;
    out[i++] = *p++;
  }
  out[i] = 0;
  return i > 0;
}

void repl(BufferPool* bp) {
  Catalog cat = catalog_open(bp);
  if (cat.catalog_heap_header_pid == INVALID_PID) return;

  char line[512];

  printf("marqdb> ");
  while (fgets(line, sizeof(line), stdin)) {
    trim(line);
    if (line[0] == 0) { printf("marqdb> "); continue; }

    if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
      break;
    }

    if (starts_with(line, "create table")) {
      char tname[TABLE_NAME_MAX];
      if (!parse_ident_after(line, "create table", tname, sizeof(tname))) {
        printf("Parse error.\nmarqdb> "); continue;
      }
      uint32_t heap_h;
      if (catalog_create_table(bp, &cat, tname, &heap_h)) {
        printf("OK\n");
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
      HeapFile hf = heap_open(bp, heap_h_pid);

      char* values = strcasestr(line, "values");
      if (!values) { printf("Parse error.\nmarqdb> "); continue; }

      char* lpar = strchr(values, '(');
      char* rpar = strrchr(values, ')');
      if (!lpar || !rpar || rpar <= lpar) { printf("Parse error.\nmarqdb> "); continue; }

      char inside[256];
      size_t len = (size_t)(rpar - lpar - 1);
      if (len >= sizeof(inside)) len = sizeof(inside)-1;
      memcpy(inside, lpar + 1, len);
      inside[len] = 0;
      trim(inside);

      if (inside[0] == '\'' || inside[0] == '"') {
        size_t m = strlen(inside);
        if (m >= 2) {
          memmove(inside, inside+1, m-2);
          inside[m-2] = 0;
        }
      }

      heap_insert(bp, &hf, (uint8_t*)inside, (uint16_t)(strlen(inside) + 1));
      printf("OK\n");
      printf("marqdb> ");
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

      HeapFile hf = heap_open(bp, heap_h_pid);
      RID cur = { .page_id = INVALID_PID, .slot_id = 0 };
      uint8_t* out;
      uint16_t len;
      int count = 0;

      while (heap_scan_next(bp, &hf, &cur, &out, &len)) {
        printf("%s\n", (char*)out);
        count++;
        bp_unpin_page(bp, cur.page_id, false);
      }

      printf("(%d rows)\n", count);
      printf("marqdb> ");
      continue;
    }

    printf("Unknown command.\nmarqdb> ");
  }
}
