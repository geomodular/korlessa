
.PHONY: tests clean run

tests: list parser

clean:
	@rm -rf list
	@rm -rf parser

list: list_test.c utest.c ../list.c ../list.h
	$(CC) -g -O0 list_test.c utest.c ../list.c -o $@

parser: parser_test.c utest.c ../parser.c ../parser.h
	$(CC) -g -O0 parser_test.c utest.c ../parser.c ../lib/mpc.c -o $@

run: list parser
	valgrind --leak-check=yes --error-exitcode=1 ./list
	valgrind --leak-check=yes --error-exitcode=1 ./parser

