#include "catalog.h"
#include "disk.h"
#include "heap.h"
#include <string.h>
#include <stdio.h>

static void write_magic(Page* p) {
  memset(p->data, 0, 8);
  memcpy(p->data, CATALOG_MAGIC, strlen(CATALOG_MAGIC));
}

void catalog_write(BufferPool* bp, const Catalog* c) {
  Page* p = bp_fetch_page(bp, CATALOG_PID);
  write_magic(p);
  memcpy(p->data + 8, &c->catalog_heap_header_pid, sizeof(uint32_t));
  memcpy(p->data + 12, &c->columns_heap_header_pid, sizeof(uint32_t));
  bp_unpin_page(bp, CATALOG_PID, true);
}

Catalog catalog_open(BufferPool* bp) {
  Catalog c = {0};
  long size = disk_file_size(bp->dm);

  if (size == 0) {
    uint32_t root_pid = disk_alloc_page(bp->dm); (void)root_pid;

    uint32_t t_h = disk_alloc_page(bp->dm);
    uint32_t t_d = disk_alloc_page(bp->dm);
    heap_bootstrap(bp, t_h, t_d);

    uint32_t c_h = disk_alloc_page(bp->dm);
    uint32_t c_d = disk_alloc_page(bp->dm);
    heap_bootstrap(bp, c_h, c_d);

    c.catalog_heap_header_pid = t_h;
    c.columns_heap_header_pid = c_h;
    catalog_write(bp, &c);
    return c;
  }

  Page* p = bp_fetch_page(bp, CATALOG_PID);

  memcpy(&c.catalog_heap_header_pid, p->data + 8, sizeof(uint32_t));
  memcpy(&c.columns_heap_header_pid, p->data + 12, sizeof(uint32_t));
  bp_unpin_page(bp, CATALOG_PID, false);
  return c;
}

int catalog_find_table(BufferPool* bp, Catalog* c, const char* name, uint32_t* out_heap_header_pid) {
  if (c->catalog_heap_header_pid == INVALID_PID) return 0;

  HeapFile cat_hf = heap_open(bp, c->catalog_heap_header_pid);
  
  RID cur = { .page_id = INVALID_PID, .slot_id = 0 };
  uint8_t* out;
  uint16_t len;

  while (heap_scan_next(bp, &cat_hf, &cur, &out, &len)) {
    if (len < sizeof(CatalogEntry)) {
      bp_unpin_page(bp, cur.page_id, false);
      continue;
    }

    CatalogEntry e;
    memcpy(&e, out, sizeof(CatalogEntry));
    bp_unpin_page(bp, cur.page_id, false);

    e.name[TABLE_NAME_MAX - 1] = 0;
    if (strncmp(e.name, name, TABLE_NAME_MAX) == 0) {
      *out_heap_header_pid = e.heap_header_pid;
      return 1;
    }
  }

  return 0;
}

static int insert_table_entry(BufferPool* bp, const Catalog* c,
                              const char* name, uint32_t heap_h_pid) {
  CatalogEntry e;
  memset(&e, 0, sizeof(e));
  strncpy(e.name, name, TABLE_NAME_MAX - 1);
  e.heap_header_pid = heap_h_pid;

  HeapFile cat_hf = heap_open(bp, c->catalog_heap_header_pid);
  heap_insert(bp, &cat_hf, (uint8_t*)&e, (uint16_t)sizeof(CatalogEntry));
  return 1;
}

static int insert_column_entries(BufferPool* bp, const Catalog* c,
                                 const char* table,
                                 const ColumnDef* cols, int ncols) {
  HeapFile col_hf = heap_open(bp, c->columns_heap_header_pid);

  for (int i = 0; i < ncols; i++) {
    ColumnEntry ce;
    memset(&ce, 0, sizeof(ce));
    strncpy(ce.table, table, TABLE_NAME_MAX - 1);
    strncpy(ce.col, cols[i].col, COL_NAME_MAX - 1);
    ce.type = (uint8_t)cols[i].type;
    ce.ordinal = (uint8_t)i;

    heap_insert(bp, &col_hf, (uint8_t*)&ce, (uint16_t)sizeof(ColumnEntry));
  }
  return 1;
}

int catalog_create_table(BufferPool* bp, Catalog* c,
                          const char* name,
                          const ColumnDef* cols, int ncols,
                          uint32_t* out_heap_header_pid) {
  if (strlen(name) >= TABLE_NAME_MAX) {
    fprintf(stderr, "Table name too long (max %d)\n", TABLE_NAME_MAX - 1);
    return 0;
  }
  if (ncols <= 0) {
    fprintf(stderr, "Table must have at least one column\n");
    return 0;
  }

  uint32_t existing;
  if (catalog_find_table(bp, c, name, &existing)) {
    fprintf(stderr, "Table '%s' already exists in catalog\n", name);
    return 0;
  }

  uint32_t heap_h_pid = disk_alloc_page(bp->dm);
  uint32_t heap_d_pid = disk_alloc_page(bp->dm);
  heap_bootstrap(bp, heap_h_pid, heap_d_pid);

  insert_table_entry(bp, c, name, heap_h_pid);
  insert_column_entries(bp, c, name, cols, ncols);

  *out_heap_header_pid = heap_h_pid;
  return 1;
}

int catalog_load_schema(BufferPool* bp, const Catalog* c,
                        const char* table,
                        ColumnDef* out_cols, int max_cols) {
  if (c->columns_heap_header_pid == INVALID_PID) return 0;

  HeapFile col_hf = heap_open(bp, c->columns_heap_header_pid);
  RID cur = { .page_id = INVALID_PID, .slot_id = 0 };
  uint8_t* out;
  uint16_t len;

  int found = 0;
  ColumnDef tmp[64];
  int tmpn = 0;

  while (heap_scan_next(bp, &col_hf, &cur, &out, &len)) {
    if (len < sizeof(ColumnEntry)) {
      bp_unpin_page(bp, cur.page_id, false);
      continue;
    }

    ColumnEntry ce;
    memcpy(&ce, out, sizeof(ce));
    bp_unpin_page(bp, cur.page_id, false);

    ce.table[TABLE_NAME_MAX - 1] = 0;
    ce.col[COL_NAME_MAX - 1] = 0;

    if (strncmp(ce.table, table, TABLE_NAME_MAX) != 0) continue;

    ColumnDef cd;
    memset(&cd, 0, sizeof(cd));
    strncpy(cd.col, ce.col, COL_NAME_MAX - 1);
    cd.type = (ColumnType)ce.type;

    if (tmpn < (int)(sizeof(tmp)/sizeof(tmp[0]))) {
      tmp[ce.ordinal] = cd;
      if (ce.ordinal + 1 > tmpn) tmpn = ce.ordinal + 1;
    }
  }

    for (int i = 0; i < tmpn && found < max_cols; i++) {
    if (tmp[i].type == 0) continue;
    out_cols[found++] = tmp[i];
  }
  return found;
}
