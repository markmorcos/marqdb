#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

static void die(const char* msg) {
  perror(msg);
  exit(1);
}

int main() {
  const char* path = "test.db";

  // -------- Phase 1: allocate + write 100 pages --------
  DiskManager* dm = disk_open(path);
  if (!dm) die("disk_open");

  const int N = 100;
  uint32_t pids[N];

  for (int i = 0; i < N; i++) {
    uint32_t pid = disk_alloc_page(dm);
    pids[i] = pid;

    Page p;
    disk_read_page(dm, pid, &p);

    char msg[64];
    snprintf(msg, sizeof(msg), "page-%u says hi", pid);

    memcpy(p.data, msg, strlen(msg) + 1);
    disk_write_page(dm, pid, &p);
  }

  disk_close(dm);

  // -------- Phase 2: reopen + verify all pages --------
  dm = disk_open(path);
  if (!dm) die("disk_open (reopen)");

  for (int i = 0; i < N; i++) {
    uint32_t pid = pids[i];

    Page q;
    disk_read_page(dm, pid, &q);

    char expected[64];
    snprintf(expected, sizeof(expected), "page-%u says hi", pid);

    if (strcmp((char*)q.data, expected) != 0) {
      fprintf(stderr, "Mismatch on page %u!\n", pid);
      fprintf(stderr, "Expected: %s\n", expected);
      fprintf(stderr, "Got:      %s\n", (char*)q.data);
      return 1;
    }
  }

  printf("✅ All %d pages written and verified correctly.\n", N);

  disk_close(dm);
  return 0;
}
