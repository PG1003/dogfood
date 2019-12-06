#!/bin/sh

# Usage: bootstrap.sh EXE SCRIPT OUTPUT
# EXE      Path of executable part of the dogfood program
# SCRIPT   Path of the lua script of the dogfood program
# OUTPUT   Path of the resulting dogfood executable program
#
# EXE and OUTPUT can target the same file which in this case will
# append SCRIPT to EXE

PAYLOAD=$(printf "\r\n-- dogfood %08X\r\n" $(stat -L -c %s $1))

if [[ ! $1 -ef $3 ]]
then
  cp -f $1 $3
fi

printf "\r\n-- food %08X\r\n" $(stat -L -c %s $2) >> $3
cat $2 >> $3
echo $PAYLOAD >> $3
