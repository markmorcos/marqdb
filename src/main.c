#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "heap.h"

int main() {
  const char* path = "test.db";
  DiskManager* dm = disk_open(path);
  HeapFile hf = heap_open(dm);

  // insert a lot of rows
  const int N = 10000;
  for (int i = 0; i < N; i++) {
    char row[128];
    int len = snprintf(row, sizeof(row), "user-%d|name=Mark-%d", i, i);
    heap_insert(dm, &hf, (uint8_t*)row, (uint16_t)(len + 1));
  }
  disk_close(dm);

  // reopen to prove persistence
  dm = disk_open(path);
  hf = heap_open(dm);

  // scan all rows
  RID cur = { .page_id = 0xFFFFFFFF, .slot_id = 0 };
  int count = 0;
  uint8_t* out;
  uint16_t len;

  while (heap_scan_next(dm, &hf, &cur, &out, &len)) {
    count++;
    if (count <= 3) {
      printf("row %d -> %s\n", count, (char*)out);
    }
  }

  printf("Scanned %d rows.\n", count);

  disk_close(dm);
  return 0;
}
