#pragma once
#include <stdint.h>
#include "buffer.h"

#define CATALOG_PID 0
#define CATALOG_MAGIC "MARQDB1"
#define INVALID_PID 0xFFFFFFFF
#define TABLE_NAME_MAX 32
#define COL_NAME_MAX 32

/**
 * @brief Represents the catalog structure in the database.
 * 
 * The catalog maintains metadata about the database, including the PID of
 * the heap file header page.
 */
typedef struct {
  uint32_t catalog_heap_header_pid; ///< PID of the catalog's heap file header page
  uint32_t columns_heap_header_pid; ///< PID of the columns' heap file header page
} Catalog;


/**
 * @brief Represents an entry in the catalog for a specific table.
 * 
 * Each entry contains the table name and the PID of its heap file header page.
 */
typedef struct {
  char name[TABLE_NAME_MAX];
  uint32_t heap_header_pid;
} CatalogEntry;

/**
 * @brief Enum for column data types.
 * 
 * This enum defines the supported data types for table columns.
 */
typedef enum {
  COL_INT = 1, ///< Integer column type
  COL_TEXT = 2 ///< Text column type
} ColumnType;

/**
 * @brief Represents an entry for a column in the catalog.
 * 
 * Each entry contains the table name, column name, data type, and ordinal position.
 */
typedef struct {
  char table[TABLE_NAME_MAX]; ///< Name of the table the column belongs to
  char col[COL_NAME_MAX]; ///< Name of the column
  uint8_t type; ///< Data type of the column
  uint8_t ordinal; ///< Ordinal position of the column in the table
} ColumnEntry;

/**
 * @brief Defines a column in a table schema.
 * 
 * This structure represents a column definition with its name and data type.
 */
typedef struct {
  char col[COL_NAME_MAX]; ///< Name of the column
  ColumnType type; ///< Data type of the column
} ColumnDef;

/**
  * @brief Opens the catalog from the buffer pool.
  * 
  * This function reads the catalog information from the designated catalog page
  * in the buffer pool and returns a Catalog structure populated with the data.
  * 
  * @param bp Pointer to the BufferPool instance managing memory pages
  * @return Catalog The populated Catalog structure
  */
Catalog catalog_open(BufferPool* bp);

/**
 * @brief Writes the catalog data to the buffer pool.
 * 
 * This function serializes the Catalog structure and writes it to the
 * designated catalog page in the buffer pool for persistent storage.
 * 
 * @param bp Pointer to the BufferPool instance managing memory pages
 * @param c Pointer to the Catalog structure containing catalog data to write
 */
void catalog_write(BufferPool* bp, const Catalog* c);

/**
 * @brief Finds a table in the catalog by name.
 * 
 * This function searches the catalog for a table with the specified name
 * and retrieves the PID of its heap file header page if found.
 * 
 * @param bp Pointer to the BufferPool instance managing memory pages
 * @param c Pointer to the Catalog structure containing catalog data
 * @param name The name of the table to find
 * @param out_heap_header_pid Pointer to store the found heap header PID
 * @return true if the table is found, false otherwise
 */
int catalog_find_table(BufferPool* bp, Catalog* c, const char* name, uint32_t* out_heap_header_pid);

/**
 * @brief Creates a new table entry in the catalog.
 * 
 * This function adds a new table with the specified name and column definitions
 * to the catalog and allocates a new heap file header page for it.
 * 
 * @param bp Pointer to the BufferPool instance managing memory pages
 * @param c Pointer to the Catalog structure containing catalog data
 * @param name The name of the table to create
 * @param cols Array of column definitions for the table
 * @param ncols Number of columns in the table
 * @param out_heap_header_pid Pointer to store the allocated heap header PID
 * @return true if the table was successfully created, false otherwise
 */
int catalog_create_table(BufferPool* bp, Catalog* c,
                          const char* name,
                          const ColumnDef* cols, int ncols,
                          uint32_t* out_heap_header_pid);

/**
  * @brief Loads the schema of a specified table from the catalog.
  * 
  * This function retrieves the column definitions for a given table
  * from the catalog and populates the provided array with the schema information.
  * 
  * @param bp Pointer to the BufferPool instance managing memory pages
  * @param c Pointer to the Catalog structure containing catalog data
  * @param table The name of the table whose schema is to be loaded
  * @param out_cols Array to store the loaded column definitions
  * @param max_cols Maximum number of columns that can be stored in out_cols
  * @return int The number of columns loaded into out_cols
  */
int  catalog_load_schema(BufferPool* bp, const Catalog* c,
                         const char* table,
                         ColumnDef* out_cols, int max_cols);