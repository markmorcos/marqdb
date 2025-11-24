#pragma once
#include "catalog.h"

/**
 * @brief Enumeration of filter operations for WHERE clauses
 * 
 * This enumeration defines the types of comparison operations
 * that can be used in SQL WHERE clauses for filtering data.
 */
typedef enum {
  OP_EQ, ///< Equal operation
  OP_LT, ///< Less than operation
  OP_GT ///< Greater than operation
} FilterOp;

/**
 * @brief Filter structure for WHERE clause operations
 * 
 * This structure represents a filter condition used in WHERE clauses
 * for querying data with comparison operations.
 */
typedef struct {
  char col[COL_NAME_MAX]; ///< Column name to filter on
  FilterOp op; ///< Filter operation (=, <, >)
  char value[128]; ///< Filter value to compare against
} Filter;

// String utility functions

/**
 * @brief Trims leading and trailing whitespace from a string in-place
 * 
 * @param s Pointer to the null-terminated string to trim
 */
void sql_trim(char* s);

/**
 * @brief Checks if a string starts with a given prefix (case-insensitive)
 * 
 * @param s The string to check
 * @param pref The prefix to search for
 * @return int Non-zero if s starts with pref, 0 otherwise
 */
int sql_starts_with(const char* s, const char* pref);

// SQL parsing functions

/**
 * @brief Extracts an identifier that appears after a keyword in a SQL statement
 * 
 * @param line The SQL command line to parse
 * @param kw The keyword to search for (e.g., "from", "create table")
 * @param out Buffer to store the extracted identifier
 * @param out_sz Size of the output buffer
 * @return int Number of characters extracted, or 0 if parsing failed
 */
int sql_parse_ident_after(const char* line, const char* kw, char* out, size_t out_sz);

/**
 * @brief Parses column definitions from a CREATE TABLE statement
 * 
 * @param line The CREATE TABLE command line
 * @param cols Array to store parsed column definitions
 * @param max_cols Maximum number of columns that can be stored
 * @return int Number of columns successfully parsed, or 0 on error
 */
int sql_parse_create_columns(const char* line, ColumnDef* cols, int max_cols);

/**
 * @brief Parses values from an INSERT statement
 * 
 * @param line The INSERT command line
 * @param values Array to store parsed values
 * @param max_vals Maximum number of values that can be stored
 * @return int Number of values successfully parsed, or 0 on error
 */
int sql_parse_insert_values(const char* line, char values[][128], int max_vals);

/**
 * @brief Parses a WHERE clause from a SELECT statement
 * 
 * @param line The SELECT command line containing the WHERE clause
 * @param f Pointer to Filter structure to populate
 * @return int 1 if WHERE clause parsed successfully, 0 if no WHERE clause, -1 on error
 */
int sql_parse_where_clause(const char* line, Filter* f);

/**
 * @brief Tests if a row matches a filter condition
 * 
 * @param f Pointer to the Filter to apply
 * @param linebuf The formatted row string to test
 * @return int Non-zero if row matches filter, 0 otherwise
 */
int sql_filter_match(const Filter* f, const char linebuf[]);

// SQL command execution functions

/**
 * @brief Executes a CREATE TABLE command
 * 
 * @param bp Pointer to the BufferPool
 * @param cat Pointer to the Catalog
 * @param line The complete CREATE TABLE command line
 * @return int 1 on success, 0 on failure
 */
int sql_exec_create_table(BufferPool* bp, Catalog* cat, const char* line);

/**
 * @brief Executes an INSERT INTO command
 * 
 * @param bp Pointer to the BufferPool
 * @param cat Pointer to the Catalog
 * @param line The complete INSERT INTO command line
 * @return int 1 on success, 0 on failure
 */
int sql_exec_insert(BufferPool* bp, Catalog* cat, const char* line);

/**
 * @brief Executes a SELECT * command with optional WHERE clause
 * 
 * @param bp Pointer to the BufferPool
 * @param cat Pointer to the Catalog
 * @param line The complete SELECT command line
 * @return int Number of rows returned, or -1 on error
 */
int sql_exec_select(BufferPool* bp, Catalog* cat, const char* line);

/**
 * @brief REPL function for SQL commands
 * 
 * This function implements a Read-Eval-Print Loop (REPL) for processing SQL commands.
 * It interacts with the buffer pool to execute commands such as creating tables,
 * inserting data, and querying data.
 * 
 * @param bp Pointer to the BufferPool structure
 */
void repl(BufferPool* bp);

/**
 * @brief Update statement structure for SQL UPDATE commands
 * 
 * This structure represents an UPDATE SQL command, including the target table,
 * the column to set, the new value, and an optional WHERE clause for filtering.
 */
typedef struct {
  char table[TABLE_NAME_MAX]; ///< Name of the table to update
  char set_col[COL_NAME_MAX]; ///< Column name to set
  char set_value[128]; ///< New value to set
  int has_where; ///< Flag indicating if a WHERE clause is present
  Filter where; ///< WHERE clause filter
} UpdateStmt;

/**
 * @brief Parses an UPDATE SQL statement into an UpdateStmt structure
 * 
 * @param line The complete UPDATE command line
 * @param st Pointer to UpdateStmt structure to populate
 * @return int 1 on successful parse, 0 on failure
 */
int sql_parse_update(const char* line, UpdateStmt* st);
