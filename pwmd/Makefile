all: pwmd pwmc
pwmd: main.c opts.c ../boilerd/timer.c
	$(CC) $^ -I ../boilerd -o $@
pwmc: client.c opts.c
	$(CC) $^ -o $@
.phony: clean test
clean:
	rm -f pwmd pwmc
