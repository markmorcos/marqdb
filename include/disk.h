#pragma once
#include <stdint.h>
#include <stdio.h>
#include "page.h"

/**
 * @brief Disk manager structure for handling file operations
 * 
 * The DiskManager structure encapsulates file I/O operations for persistent storage.
 * It provides a wrapper around standard file operations for database storage management.
 */
typedef struct {
  FILE* f; ///< File pointer for disk I/O operations */
} DiskManager;

/**
 * @brief Opens a disk manager for the specified file path.
 * 
 * Creates and initializes a new DiskManager instance that manages disk I/O
 * operations for the database file at the given path. If the file doesn't exist,
 * it may be created depending on the implementation.
 * 
 * @param path The file system path to the database file to open or create
 * @return DiskManager* Pointer to the newly created DiskManager instance,
 *                      or nullptr if the operation fails
 */
DiskManager* disk_open(const char* path);

/**
 * @brief Closes the disk manager and releases associated resources.
 * 
 * This function properly shuts down the disk manager, ensuring all pending
 * operations are completed and all resources are released. It should be called
 * when the disk manager is no longer needed to prevent resource leaks.
 * 
 * @param dm Pointer to the DiskManager instance to close. Must not be NULL.
 */
void disk_close(DiskManager* dm);


/**
 * @brief Reads a page from disk into memory
 * 
 * This function reads a specific page identified by page_id from the disk
 * managed by the DiskManager and loads it into the provided Page buffer.
 * 
 * @param dm Pointer to the DiskManager instance that manages disk operations
 * @param page_id The unique identifier of the page to be read from disk
 * @param out Pointer to the Page structure where the read data will be stored
 */
void disk_read_page(DiskManager* dm, uint32_t page_id, Page* out);

 /**
 * @brief Writes a page to disk storage
 * 
 * This function writes the contents of a page from memory to the disk at the
 * specified page location. The page data is persisted to the storage device
 * managed by the disk manager.
 * 
 * @param dm Pointer to the disk manager that handles disk operations
 * @param page_id The unique identifier of the page to write to disk
 * @param in Pointer to the page containing the data to be written
 */
void disk_write_page(DiskManager* dm, uint32_t page_id, const Page* in);

/**
 * @brief Allocates a new page on disk and returns its page ID.
 * 
 * This function allocates a new page in the disk storage managed by the 
 * DiskManager. The allocated page can be used to store data and will be
 * assigned a unique page identifier.
 * 
 * @param dm Pointer to the DiskManager instance that manages the disk storage
 * @return uint32_t The page ID of the newly allocated page, or an error code
 *                  if allocation fails
 */
uint32_t disk_alloc_page(DiskManager* dm);
