
.PHONY: tests clean run

tests: list

clean:
	@rm -rf list

list: list_test.c ../list.c ../list.h
	$(CC) list_test.c ../list.c -o $@

run: list
	@./list

