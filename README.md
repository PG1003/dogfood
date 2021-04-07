# A tool for building self contained Lua executables

This tool is an alternative for ```srlua``` which can be found [here](http://tecgraf.puc-rio.br/~lhf/ftp/lua/).

The main points where ```dogfood``` differs from ```srlua``` are:

* One self contained executable.
* Can embed multiple Lua modules in one executable.
* No support for Lua 5.1 and older.

Other features:

* Does not depend on a C compiler to build self contained Lua executables.
* ```dogfood``` self can be build for Unix like operating systems _and_ Windows.

```dogfood``` consists of two parts; a Lua interpreter and a Lua module that creates the self executable Lua programs.
The interpreter runs the dogfood's Lua module that is appended at the end of the executable.
The module reuses the interpreter when it builds a self contained Lua executable by copying its interpreter and then append the user's Lua modules.

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

## Dogfood binaries for Windows

Building dogfood binaries for Windows requires more effort than for Unix like operating systems.  
The following executables are available for download to lower the bar for Windows users that want to try ```dogfood``` or don't have the knowledge to build it.

| Lua version | dogfood v1.0.2 x86-64 | dogfood v1.0.2 x86-32 |
|-------------|-----|-----|
| 5.2.4 | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood52_win_x86-64.zip) | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood52_win_x86-32.zip) |
| 5.3.6 | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood53_win_x86-64.zip) | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood53_win_x86-32.zip) |
| 5.4.3 | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood54_win_x86-64.zip) | [download](https://raw.githubusercontent.com/PG1003/dogfood/master/exe/dogfood54_win_x86-32.zip) |
                              
## Bootstrapping

Right after compilation, a dogfood binary must be bootstrapped before it is able to build self contained Lua executables.
The makefile provided with the sources already takes care of this.
In case when you have your own build environment, a bootstrap shell script is provided for Unix like environments and for Windows.

The usage of the bootstrap script is as follows;

``` sh
bootstrap.sh EXECUTABLE food.lua OUT
```

For Windows ```bootstrap.bat``` instead of ```bootstrap.sh``` is used but the commandline prameters are same.

```EXECUTABLE``` is the path to the executable that needs to be bootstrapped.  
The file ```food.lua``` contains dogfood's logic.
You can find this file in the [source](/src) directory.  
```OUT``` is the destination of the resulting executable.
```EXECUTABLE``` and ```OUT``` can point to the same file.
