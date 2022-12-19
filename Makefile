all: boilerd
boilerd: main.c opts.c timer.c
	$(CC) $^ -lpidc -o $@
.phony: clean test
clean:
	rm -f boilerd
