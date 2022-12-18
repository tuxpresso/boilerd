all: boilerd
boilerd: main.c
	$(CC) $^ -lpidc -lzmq -o $@
.phony: clean test
clean:
	rm -f boilerd
