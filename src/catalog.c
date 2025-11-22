#include "catalog.h"
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
  memcpy(p->data + 8, &c->heap_header_pid, sizeof(uint32_t));
  bp_unpin_page(bp, CATALOG_PID, true);
}

Catalog catalog_open(BufferPool* bp) {
  Catalog c = {0};

  long size = disk_file_size(bp->dm);
  if (size == 0) {
    uint32_t cat_pid = disk_alloc_page(bp->dm); (void)cat_pid;

    uint32_t heap_header_pid = disk_alloc_page(bp->dm);

    uint32_t first_data_pid = disk_alloc_page(bp->dm);

    heap_bootstrap(bp, heap_header_pid, first_data_pid);

    c.heap_header_pid = heap_header_pid;
    catalog_write(bp, &c);
    return c;
  }

  Page* p = bp_fetch_page(bp, CATALOG_PID);

  memcpy(&c.heap_header_pid, p->data + 8, sizeof(uint32_t));
  bp_unpin_page(bp, CATALOG_PID, false);

  return c;
}
