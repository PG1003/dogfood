CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3
INCLUDES = -I "./src"
LDFLAGS = -llua

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
	LIB_FLAGS = -bundle -undefined dynamic_lookup
else
	LIB_FLAGS = -shared -fPIC
endif

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

# The first part of the testing dogfood is done by building the foobar excutable successfully.
# Then the tests below are executed to test the foobar executable and by trying some error conditions.
test: $(OUTDIR)foobar $(OUTDIR)pg1005.so
	@echo "Running tests..."
	@echo "> Test 1; happy flow foobar"
	@cd $(OUTDIR); ./foobar -param && : || { echo ">>> Test 1 failed!"; exit 1; }
	@echo "> Test 2; expect a failed assertion from foobar"
	@cd $(OUTDIR); ./foobar && { echo ">>> Test 2 failed!"; exit 1; } || :
	@echo "> Test 3; expect error from dogfood about an issue with the pg1005 module"
	@cd $(OUTDIR); ./dogfood -m ../$(TSTDIR)?.lua foobar_failed foo bar pg1005 && { echo ">>> Test 3 failed!";  exit 1; } || :
	@echo ""
	@echo "...tests completed"
	@echo "      _"
	@echo "     /(|"
	@echo "    (  :"
	@echo "   __\  \  _____"
	@echo " (____)  '|"
	@echo "(____)|   |"
	@echo " (____).__|"
	@echo "  (___)__.|_____"
# https://asciiart.website/index.php?art=people/body%20parts/hand%20gestures


$(OUTDIR)foobar: $(OUTDIR)dogfood $(TSTDIR)foo.lua $(TSTDIR)bar.lua $(TSTDIR)bar/baz.lua $(OUTDIR)pg1003.so
	cd $(OUTDIR) && ./dogfood -c -s -m ../$(TSTDIR)?.lua foobar foo foo bar bar.baz pg1003
	chmod u+x $@

$(OUTDIR)pg1003.so: $(TSTDIR)pg1003.c
	$(CC)  $(CFLAGS) $(LIB_FLAGS) -o $@ $+

# By copying pg1003.so to pg1005.so we have created an invalid lua library (wrong entry point) that is used to test dogfood
$(OUTDIR)pg1005.so: $(OUTDIR)pg1003.so
	cp $(OUTDIR)pg1003.so $(OUTDIR)pg1005.so

clean:
	rm -rf $(OUTDIR)
