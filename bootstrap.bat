@echo off

:: Usage: bootstrap.bat EXE SCRIPT OUTPUT
:: EXE      Path of executable part of the dogfood program
:: SCRIPT   Path of the lua script of the dogfood program
:: OUTPUT   Path of the resulting dogfood executable program
::
:: EXE and OUTPUT can target the same file which in this case will
:: append SCRIPT to EXE

setlocal

:: tohex strips the leading zeros when setting the return value

set dogsize=0
call :tohex dogsize %~z1
set dogsize=00000000%dogsize%

set foodsize=0
call :tohex foodsize %~z2
set foodsize=00000000%foodsize%

if /I not %1==%3 copy /B /Y %1 %3

echo.>> %3
echo -- %~n2 %foodsize:~-8%>> %3

type %2 >> %3

echo.>> %3
echo -- %~n1 %dogsize:~-8%>> %3

endlocal

goto exit


:: https://www.dostips.com/DtTipsArithmetic.php#toHex
:tohex
SETLOCAL ENABLEDELAYEDEXPANSION
set /A dec=%~2
set "hex="
set "map=0123456789ABCDEF"
for /L %%N in (1,1,8) do (
    set /A "d=dec&15,dec>>=4"
    for %%D in (!d!) do set "hex=!map:~%%D,1!!hex!"
)
ENDLOCAL & SET %1=%hex%


:exit
