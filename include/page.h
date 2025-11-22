#pragma once
#include <stdint.h>
#include <stdbool.h>

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

  uint32_t next_page_id; ///< ID of the next page in the linked list (0xFFFFFFFF if none)
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

/**
 * @brief Slot structure representing a record entry within a database page
 * 
 * A Slot contains metadata about a record (row) stored in the page's data section.
 * Slots provide indirect access to records, allowing for efficient space management
 * and record operations such as updates and deletions without moving data.
 */
typedef struct {
  uint16_t offset;     ///< Offset where the row starts in the data[] array
  uint16_t len;        ///< Length of the row in bytes
  uint8_t deleted;     ///< Record status: 0 = alive, 1 = deleted
  uint8_t _pad;        ///< Padding byte for alignment
} Slot;

/**
 * @brief Initializes a Page structure with default values.
 * 
 * This function sets up a Page by initializing its header fields to default values,
 * preparing it for use in the database system.
 * 
 * @param p Pointer to the Page to be initialized
 */
void page_init(Page* p, uint32_t page_id);

/**
 * @brief Checks if a Page has enough free space to accommodate a new record.
 * 
 * This function calculates the available free space in the Page and determines
 * if it can fit a record of the specified length.
 * 
 * @param p Pointer to the Page to check
 * @param record_len Length of the record to be inserted
 * @return true if there is enough space, false otherwise
 */
bool page_has_space(Page* p, uint16_t record_len);

/**
 * @brief Inserts a new record into the Page.
 * 
 * This function adds a new record to the Page's data section and updates the
 * corresponding Slot metadata. It assumes that there is enough space available.
 * 
 * @param p Pointer to the Page where the record will be inserted
 * @param rec Pointer to the record data to be inserted
 * @param len Length of the record in bytes
 * @return The slot ID where the record was inserted, or -1 on failure
 */
int page_insert(Page* p, const uint8_t* rec, uint16_t len);

/**
 * @brief Retrieves a record from the Page by its slot ID.
 * 
 * This function fetches the record data associated with the specified slot ID
 * and provides a pointer to the record along with its length.
 * 
 * @param p Pointer to the Page containing the record
 * @param slot_id The slot ID of the record to retrieve
 * @param out Output parameter that will point to the record data
 * @param len Output parameter that will hold the length of the record
 * @return true if the record was successfully retrieved, false otherwise
 */
bool page_get(Page* p, int slot_id, uint8_t** out, uint16_t* len);

/**
 * @brief Marks a record in the Page as deleted by its slot ID.
 * 
 * This function updates the Slot metadata to indicate that the record is deleted.
 * The actual data remains in the Page, but it is considered inactive.
 * 
 * @param p Pointer to the Page containing the record
 * @param slot_id The slot ID of the record to delete
 * @return true if the record was successfully marked as deleted, false otherwise
 */
bool page_delete(Page* p, int slot_id);
