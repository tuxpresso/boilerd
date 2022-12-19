all: boilerd boilerc
boilerd: main.c opts.c timer.c
	$(CC) $^ -lpidc -o $@
boilerc: client.c opts.c
	$(CC) $^ -o $@
.phony: clean test
clean:
	rm -f boilerd boilerc
