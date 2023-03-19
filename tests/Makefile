
.PHONY: tests clean run

tests: list parser translator

clean:
	@rm -rf list
	@rm -rf parser
	@rm -rf translator

list: list_test.c utest.c ../list.c ../list.h
	$(CC) -g -O0 list_test.c utest.c ../list.c -o $@

parser: parser_test.c utest.c ../parser.c ../parser.h
	$(CC) -g -O0 parser_test.c utest.c ../parser.c ../lib/mpc.c -o $@

translator: translator_test.c utest.c ../translator.c ../translator.h ../list.c ../list.h ../parser.c ../parser.h
	$(CC) -g -O0 translator_test.c utest.c ../translator.c ../list.c ../parser.c ../lib/mpc.c -o $@

run: list parser translator
	valgrind --leak-check=yes --error-exitcode=1 ./list
	valgrind --leak-check=yes --error-exitcode=1 ./parser
	valgrind --leak-check=yes --error-exitcode=1 ./translator

