all: boilerd pwmd
.PHONY: clean boilerd pwmd
boilerd:
	$(MAKE) -C boilerd
pwmd:
	$(MAKE) -C pwmd
clean:
	$(MAKE) -C boilerd clean
	$(MAKE) -C pwmd clean
