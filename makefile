CC=g++

build/%.o: src/%.cpp
	$(CC) -c -o $@ -w $<

build/vcc: $(patsubst src/%.cpp,build/%.o,$(wildcard src/*.cpp))
