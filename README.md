# A tool for building self executable Lua programs

This tool is an alternative for ```srlua``` which can be found [here](http://tecgraf.puc-rio.br/~lhf/ftp/lua/).

The main points where ```dogfood``` differs from ```srlua``` are;

* One self containing executable.
* Can embed multiple Lua modules in one executable.
* No support for Lua 5.1 and older.

```dogfood``` consists of two parts; a Lua interpreter and a Lua module that creates the self executable Lua programs.
The interpreter runs the dogfood's Lua module that is placed at the end of the executable file.
The module reuses the interpreter when it builds a self executing Lua program by copying its interpreter and append the user's Lua modules.

```dogfood``` owes its name to the reuse of its own interpreter.
Using your own software products is also knowns as [dogfooding](https://en.wikipedia.org/wiki/Eating_your_own_dog_food).

## Requirements

* Lua version 5.2, 5.3 or 5.4

## Usage of dogfood

``` sh
dogfood [OPTIONS] OUT MODULE...
```

```OUT``` is the destination of the resulting program  
The first ```MODULE``` from the list is the entry point of the program while the optional extra modules are accessable by ```require```.
The modules are provided _without_ the '.lua' extention.  
C modules cannot be embedded in the resulting program but the program can load C modules when they are placed in one of the ```package.cpath``` search paths.

The following ```OPTIONS``` are available.

|Option | Description|
|-------|------------|
|-c, --compile | Compile the modules and embed them as bytecode.|
|-s, --strip-debug-information | Strips the debug information from the bytecode.|
|-h, --help | Shows the help.|
|-m | Adds the given path to ```package.loaded``` to search for modules that are provided as parameter.|
|-v, --lua-version | Shows the Lua language version of the interpreter used by the resulting program..|
                              
                              
## Bootstrapping

```dogfood``` must be bootstrapped before it is able to build self executable Lua programs.
The makefile provided with the sources already takes care of this.
In case when you have your own build environment, a bootstrap shell script is provided for Unix like environments and for Windows.

The usage of the bootstrap script is as follows;

``` sh
bootstrap.sh EXECUTABLE food.lua OUT
```

For Windows ```bootstrap.sh``` is replaced by ```bootstrap.bat```.

```EXECUTABLE``` is the path to the executable that needs to be bootstrapped.  
The file ```food.lua``` contains the dogfood's logic.
You can find this file in the [source](/src) directory.  
```OUT``` is the destination of the resulting executable.
```EXECUTABLE``` and ```OUT``` can point to the same file.
