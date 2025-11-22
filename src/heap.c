#include "heap.h"
#include <string.h>
#include <stdio.h>

#define INVALID_PID 0xFFFFFFFF

static void write_header(DiskManager* dm, HeapFile* hf) {
  Page p;
  disk_read_page(dm, hf->header_page_id, &p);

  memcpy(p.data + 0, &hf->first_data_pid, sizeof(uint32_t));
  memcpy(p.data + 4, &hf->last_data_pid,  sizeof(uint32_t));

  disk_write_page(dm, hf->header_page_id, &p);
}

HeapFile heap_open(DiskManager* dm) {
  HeapFile hf = {0};
  hf.header_page_id = 0;

  fseek(dm->f, 0, SEEK_END);
  long size = ftell(dm->f);

  if (size == 0) {
    uint32_t header_pid = disk_alloc_page(dm);

    uint32_t data_pid = disk_alloc_page(dm);

    hf.header_page_id = header_pid;
    hf.first_data_pid = data_pid;
    hf.last_data_pid  = data_pid;

    write_header(dm, &hf);
    return hf;
  }

  Page p;
  disk_read_page(dm, hf.header_page_id, &p);

  memcpy(&hf.first_data_pid, p.data + 0, sizeof(uint32_t));
  memcpy(&hf.last_data_pid,  p.data + 4, sizeof(uint32_t));

  if (hf.first_data_pid == 0 && hf.last_data_pid == 0) {
    uint32_t data_pid = disk_alloc_page(dm);
    hf.first_data_pid = data_pid;
    hf.last_data_pid  = data_pid;
    write_header(dm, &hf);
  }

  return hf;
}

RID heap_insert(DiskManager* dm, HeapFile* hf, const uint8_t* rec, uint16_t len) {
  uint32_t pid = hf->last_data_pid;

  while (1) {
    Page p;
    disk_read_page(dm, pid, &p);

    int slot = page_insert(&p, rec, len);
    if (slot >= 0) {
      disk_write_page(dm, pid, &p);
      return (RID){ .page_id = pid, .slot_id = (uint16_t)slot };
    }

    if (p.hdr.next_page_id != INVALID_PID) {
      pid = p.hdr.next_page_id;
      continue;
    }

    uint32_t new_pid = disk_alloc_page(dm);
    p.hdr.next_page_id = new_pid;
    disk_write_page(dm, pid, &p);

    hf->last_data_pid = new_pid;
    write_header(dm, hf);

    pid = new_pid;
  }
}

bool heap_get(DiskManager* dm, RID rid, uint8_t** out, uint16_t* len) {
  Page p;
  disk_read_page(dm, rid.page_id, &p);
  return page_get(&p, rid.slot_id, out, len);
}

bool heap_scan_next(DiskManager* dm, HeapFile* hf, RID* cursor, uint8_t** out, uint16_t* len) {
  uint32_t pid;
  uint16_t slot;

  if (cursor->page_id == INVALID_PID) {
    pid = hf->first_data_pid;
    slot = 0;
  } else {
    pid = cursor->page_id;
    slot = cursor->slot_id + 1;
  }

  while (pid != INVALID_PID) {
    Page p;
    disk_read_page(dm, pid, &p);

    for (; slot < p.hdr.slot_count; slot++) {
      if (page_get(&p, slot, out, len)) {
        cursor->page_id = pid;
        cursor->slot_id = slot;
        return true;
      }
    }

    pid = p.hdr.next_page_id;
    slot = 0;
  }

  return false;
}
