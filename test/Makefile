CFLAGS := -I../include -m32 -g

first: test_elf_reader

test_dict:
	gcc test_dict.c $(CFLAGS)
	./a.out

test_str:
	gcc test_str.c $(CFLAGS)
	./a.out

test_vec:
	gcc test_vec.c $(CFLAGS)
	./a.out

test_util:
	gcc test_util.c $(CFLAGS)
	./a.out

test_elf_reader:
	gcc -m32 -c sum.c -o /tmp/sum.o
	gcc test_elf_reader.c $(CFLAGS)
	./a.out /tmp/sum.o

test_elf_writer:
	gcc test_elf_writer.c $(CFLAGS)
	./a.out
