#include "disk.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

DiskManager* disk_open(const char* path) {
  DiskManager* dm = calloc(1, sizeof(*dm));
  dm->f = fopen(path, "r+b");
  if (!dm->f) dm->f = fopen(path, "w+b");
  return dm;
}

void disk_close(DiskManager* dm) {
  if (!dm) return;
  fclose(dm->f);
  free(dm);
}

void disk_read_page(DiskManager* dm, uint32_t pid, Page* out) {
  memset(out, 0, sizeof(Page));
  fseek(dm->f, (long)pid * PAGE_SIZE, SEEK_SET);
  fread(out, PAGE_SIZE, 1, dm->f);
}

void disk_write_page(DiskManager* dm, uint32_t pid, const Page* in) {
  fseek(dm->f, (long)pid * PAGE_SIZE, SEEK_SET);
  fwrite(in, PAGE_SIZE, 1, dm->f);
  fflush(dm->f);
}

uint32_t disk_alloc_page(DiskManager* dm) {
  fseek(dm->f, 0, SEEK_END);
  long size = ftell(dm->f);
  uint32_t pid = (uint32_t)(size / PAGE_SIZE);

  Page p;
  page_init(&p, pid);
  disk_write_page(dm, pid, &p);
  return pid;
}

long disk_file_size(DiskManager* dm) {
  long cur = ftell(dm->f);
  if (cur < 0) cur = 0;

  fseek(dm->f, 0, SEEK_END);
  long size = ftell(dm->f);

  fseek(dm->f, cur, SEEK_SET);
  return size;
}