CC=g++
MAKEFLAGS=-j

.PHONY: rebuild

rebuild:
	$(MAKE) clean
	$(MAKE) build/vcc

clean:
	rm -rf build
	mkdir build

build/%.o: src/%.cpp
	$(CC) -c -o $@ -Wall $<

build/vcc: $(patsubst src/%.cpp,build/%.o,$(wildcard src/*.cpp))
