all: boilerd boilerc
boilerd: main.c
	$(CC) $^ -lpidc -lzmq -o $@
boilerc: client.c
	$(CC) $^ -lzmq -o $@
.phony: clean test
clean:
	rm -f boilerd boilerc
