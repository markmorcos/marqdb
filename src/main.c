#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "page.h"

int main() {
  Page p;
  page_init(&p, 0);

  int inserted = 0;
  for (int i = 0; i < 1000; i++) {
    char row[128];
    int len = snprintf(row, sizeof(row), "row-%d: hello hello hello", i);
    int slot = page_insert(&p, (uint8_t*)row, (uint16_t)(len + 1));
    if (slot < 0) break;
    inserted++;
  }

  printf("Inserted %d rows into one page.\n", inserted);
  printf("free_start=%u free_end=%u slot_count=%u\n",
         p.hdr.free_start, p.hdr.free_end, p.hdr.slot_count);

  for (int i = 0; i < inserted; i += 5) {
    page_delete(&p, i);
  }

  int alive = 0, dead = 0;
  for (int i = 0; i < p.hdr.slot_count; i++) {
    uint8_t* out;
    uint16_t len;
    if (page_get(&p, i, &out, &len)) {
      alive++;
      if (i < 3) printf("slot %d -> %s\n", i, (char*)out);
    } else {
      dead++;
    }
  }

  printf("Scan result: alive=%d dead=%d\n", alive, dead);

  return 0;
}
