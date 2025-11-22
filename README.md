# MarqDB

MarqDB is an experimental relational database engine written in C.  
It is designed as a learning project exploring how modern database systems work internally, with a focus on clarity, correctness, and simple architecture.

The project aims to provide a minimal but realistic foundation for:

- page-based storage
- on-disk data management
- indexing structures
- SQL interpretation
- transactional guarantees

MarqDB is not intended as a production database.  
Its primary goal is to serve as a clean, approachable codebase for understanding the fundamentals of storage engines and query execution.

---

## Features (Current & Planned)

MarqDB follows a modular structure that allows independent development of core components. The high-level features include:

### Storage Engine

- Fixed-size pages
- Page header metadata
- Variable-length record storage
- Slot directory for row management
- Page-level insert, delete, and scan operations

### File & Buffer Management

- On-disk page allocation and I/O abstraction
- Buffer management (planned)
- Basic free-space tracking (planned)

### Table & Index Structures

- Heap files (planned)
- B+ tree indexing (planned)
- Catalog metadata (planned)

### SQL Layer (Long-Term)

- Lexer and parser for a SQL subset
- AST construction
- Logical and physical plan generation
- Query execution operators

### Durability & Concurrency (Future)

- Write-ahead logging (WAL)
- Crash recovery
- Concurrency control mechanisms

---

## Project Structure

```
marqdb/
├── include/ # public headers
├── src/ # implementation files
├── build/ # compiled binaries
└── Makefile
```

---

## Building

MarqDB requires a C11-compatible compiler.  
To build:

```bash
make
```

To clean and rebuild:

```bash
make clean && make
```

---

## Running

The default executable is:

```bash
./build/marqdb
```

This may run a test harness or REPL depending on current development.

---

## Goals

MarqDB was created to explore and understand:

- how data is physically organized on disk
- how database pages are structured and managed
- how indexing structures work
- how SQL queries are transformed into execution plans
- how durability and concurrency are implemented

The project focuses on incremental, well-documented development rather than large monolithic components.

---

## Why This Exists

Databases are often treated as black boxes.  
MarqDB aims to:

- demystify their internal design
- provide a clear and educational reference implementation
- help others (and the author) deepen understanding of low-level systems

---

## Contributing

Contributions, suggestions, and discussions are welcome.  
The project is early-stage and evolving, so feedback and ideas are appreciated.

---

## License

MIT License.

Feel free to use, modify, or learn from this project.
