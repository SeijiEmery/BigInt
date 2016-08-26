CC = clang++
CFLAGS = -std=gnu++14 -Wall -Werror

bigint_test: BigInt/main.cpp BigInt/unittest.hpp
	$(CC) $(CFLAGS) BigInt/main.cpp -O2 -o $@

all: bigint_test

run: bigint_test
	./bigint_test
clean:
	rm -f ./bigint_test
