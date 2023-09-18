# RW combined API or RW separated API

Common funcionality regarding an elf file can be divided into read APIs and write APIs. There are 2 design strategies:
1. have a data structure that can handle both read and write
2. have separated data structures for read and write.

Since in most use cases we care, like linker, ar, nm, etc, we never need the ability to read an elf file and update the same one, having separating data structures for read/write seems reasonable.
