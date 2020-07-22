CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3
INCLUDES = -I "./src"
LDFLAGS = -llua

SRCDIR = src/
OUTDIR = out/
TSTDIR = test/

SRC = $(shell find $(SRCDIR) -type f -name '*.c')
OBJ = $(patsubst $(SRCDIR)%.c, $(OUTDIR)%.o, $(SRC))

.phony: dogfood test clean

dogfood: $(OUTDIR)dogfood

$(OUTDIR)dogfood: $(OUTDIR) ./bootstrap.sh $(OUTDIR)dog $(SRCDIR)food.lua
	./bootstrap.sh $(OUTDIR)dog $(SRCDIR)food.lua $@

$(OUTDIR)dog: $(OBJ)
	$(CC) $(LDFLAGS) $+ -o $@

$(OUTDIR)%.o: $(SRCDIR)%.c
	$(CC) $(CFLAGS) -c $? -o $(patsubst $(SRCDIR)%.c, $(OUTDIR)%.o, $@)

$(OUTDIR):
	mkdir $@

test: $(OUTDIR)foobar
	@echo "Running test..."
	@$< "-param" || ( echo "---"; echo "Dogfood test failed!"; exit 1; )

$(OUTDIR)foobar: $(OUTDIR)dogfood $(TSTDIR)foo.lua $(TSTDIR)bar.lua
	$(OUTDIR)dogfood -c -s -m $(TSTDIR)?.lua $@ foo bar
	chmod u+x $@
	
clean:
	rm -rf $(OUTDIR)
