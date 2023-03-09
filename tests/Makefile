
.PHONY: tests clean run

tests: list

clean:
	@rm -rf list

list: list_test.c utest.c ../list.c ../list.h
	$(CC) -g -O0 list_test.c utest.c ../list.c -o $@

run: list
	valgrind --leak-check=yes --error-exitcode=1 ./list

