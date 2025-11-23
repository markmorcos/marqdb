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
  bp_unpin_page(bp, CATALOG_PID, true);
}

Catalog catalog_open(BufferPool* bp) {
  Catalog c = {0};

  long size = disk_file_size(bp->dm);
  if (size == 0) {
    uint32_t root_pid = disk_alloc_page(bp->dm); (void)root_pid;
    uint32_t cat_h_pid = disk_alloc_page(bp->dm);
    uint32_t cat_d_pid = disk_alloc_page(bp->dm);

    heap_bootstrap(bp, cat_h_pid, cat_d_pid);

    c.catalog_heap_header_pid = cat_h_pid;
    catalog_write(bp, &c);
    return c;
  }

  Page* p = bp_fetch_page(bp, CATALOG_PID);

  memcpy(&c.catalog_heap_header_pid, p->data + 8, sizeof(uint32_t));
  bp_unpin_page(bp, CATALOG_PID, false);

  return c;
}

bool catalog_find_table(BufferPool* bp, Catalog* c, const char* name, uint32_t* out_heap_header_pid) {
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

bool catalog_create_table(BufferPool* bp, Catalog* c, const char* name, uint32_t* out_heap_header_pid) {
  if (strlen(name) >= TABLE_NAME_MAX) {
    fprintf(stderr, "Table name too long (max %d)\n", TABLE_NAME_MAX - 1);
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

  CatalogEntry e;
  memset(&e, 0, sizeof(e));
  strncpy(e.name, name, TABLE_NAME_MAX - 1);
  e.heap_header_pid = heap_h_pid;

  HeapFile cat_hf = heap_open(bp, c->catalog_heap_header_pid);
  heap_insert(bp, &cat_hf, (uint8_t*)&e, sizeof(CatalogEntry));

  *out_heap_header_pid = heap_h_pid;
  return 1;
}