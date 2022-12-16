all: boilerd
boilerd: main.c
	$(CC) $^ -lpidc -o $@
.phony: clean test
clean:
	rm -f boilerd
