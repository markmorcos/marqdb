#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "disk.h"
#include "buffer.h"
#include "heap.h"

int main() {
  const char* path = "test.db";
  DiskManager* dm = disk_open(path);

  BufferPool* bp = bp_create(dm, 8);
  HeapFile hf = heap_open(bp);

  const int N = 10000;

  for (int i = 0; i < N; i++) {
    char row[128];
    int len = snprintf(row, sizeof(row), "user-%d|name=Mark-%d", i, i);
    heap_insert(bp, &hf, (uint8_t*)row, (uint16_t)(len + 1));
  }

  bp_flush_all(bp);
  bp_destroy(bp);
  disk_close(dm);

  dm = disk_open(path);
  bp = bp_create(dm, 8);
  hf = heap_open(bp);

  RID cur = { .page_id = 0xFFFFFFFF, .slot_id = 0 };
  int count = 0;
  uint8_t* out;
  uint16_t len;

  while (heap_scan_next(bp, &hf, &cur, &out, &len)) {
    count++;
    if (count <= 3) printf("row %d -> %s\n", count, (char*)out);

    bp_unpin_page(bp, cur.page_id, false);
  }

  printf("Scanned %d rows.\n", count);

  bp_destroy(bp);
  disk_close(dm);
  return 0;
}
