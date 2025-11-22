#pragma once
#include <stdint.h>

#define PAGE_SIZE 8192

/**
 * @brief Page header structure containing metadata for database pages
 * 
 * This structure represents the header of a database page, containing essential
 * metadata used for page management, recovery, and space allocation.
 */
typedef struct {
  uint32_t page_id;     ///< Unique identifier for this page
  uint32_t lsn;         ///< Log sequence number for recovery and consistency
  uint16_t free_start;  ///< Offset to the start of free space in the page
  uint16_t free_end;    ///< Offset to the end of free space in the page
  uint16_t slot_count;  ///< Number of slots (records) currently in the page
  uint16_t flags;       ///< Page flags for various page states and properties
} PageHeader;

/**
 * @brief Represents a database page structure containing header and data sections.
 * 
 * A Page is the fundamental unit of storage in the database system. It consists of
 * a fixed-size header containing metadata about the page, followed by a data section
 * that utilizes the remaining space within the page boundary.
 * 
 * The total size of a Page is exactly PAGE_SIZE bytes, with the data section
 * automatically sized to fill the remaining space after accounting for the header.
 */
typedef struct {
  PageHeader hdr;
  uint8_t data[PAGE_SIZE - sizeof(PageHeader)];
} Page;
