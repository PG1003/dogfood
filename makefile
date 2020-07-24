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
	@echo "Running tests..."
	@echo "> Test 1; happy flow"
	@$< "-param" && : || { echo ">>> Test 1 failed!"; exit 1; }
	@echo "> Test 2; expect error from program"
	@$< && { echo ">>> Test 2 failed!"; exit 1; } || :
	@echo "Tests completed :)"

$(OUTDIR)foobar: $(OUTDIR)dogfood $(TSTDIR)foo.lua $(TSTDIR)bar.lua $(TSTDIR)bar/baz.lua
	$(OUTDIR)dogfood -c -s -m $(TSTDIR)?.lua $@ foo bar bar.baz
	chmod u+x $@

clean:
	rm -rf $(OUTDIR)
