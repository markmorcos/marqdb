#include "page.h"
#include <string.h>

static inline Slot* slot_dir_base(Page* p) {
  return (Slot*)(p->data + p->hdr.free_end);
}

void page_init(Page* p, uint32_t page_id) {
  memset(p, 0, sizeof(Page));
  p->hdr.page_id = page_id;
  p->hdr.free_start = 0;
  p->hdr.free_end = sizeof(p->data);
  p->hdr.slot_count = 0;
  p->hdr.flags = 0;
}

bool page_has_space(Page* p, uint16_t record_len) {
  uint16_t needed = record_len + sizeof(Slot);
  return (p->hdr.free_start + needed) <= p->hdr.free_end;
}

int page_insert(Page* p, const uint8_t* rec, uint16_t len) {
  if (!page_has_space(p, len)) return -1;

  uint16_t off = p->hdr.free_start;
  memcpy(p->data + off, rec, len);
  p->hdr.free_start += len;

  p->hdr.free_end -= sizeof(Slot);
  Slot* s = (Slot*)(p->data + p->hdr.free_end);

  s->offset = off;
  s->len = len;
  s->deleted = 0;

  return p->hdr.slot_count++;
}

bool page_get(Page* p, int slot_id, uint8_t** out, uint16_t* len) {
  if (slot_id < 0 || slot_id >= p->hdr.slot_count) return false;

  Slot* first = slot_dir_base(p);
  Slot* s = &first[slot_id];

  if (s->deleted) return false;

  *out = p->data + s->offset;
  *len = s->len;
  return true;
}

bool page_delete(Page* p, int slot_id) {
  if (slot_id < 0 || slot_id >= p->hdr.slot_count) return false;
  Slot* first = slot_dir_base(p);
  first[slot_id].deleted = 1;
  return true;
}
