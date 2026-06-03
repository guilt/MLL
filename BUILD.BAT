@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
IF NOT DEFINED EXTDEV ECHO ERROR: EXTDEV environment variable not set. && EXIT /B 1
SET APP=%~dp0
CD /D "%APP%"

SET MINGDIR=%EXTDEV%\MinGW32
SET GCC="%MINGDIR%\Bin\Gcc"
SET CFLAGS=-std=gnu99 -Wall -m32 -O1 -fopenmp -I"." -I"%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Include"

SET LIBS=-L"%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Lib\MinGW\x86"
SET ALLEGRO_LIBS=-lalleg -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -ldinput -lddraw -ldxguid -lwinmm -ldsound

MKDIR OBJ 2>NUL
MKDIR DIST 2>NUL

ECHO Compiling...
%GCC% %CFLAGS% -c mll.c -o OBJ\MLL.OBJ
%GCC% %CFLAGS% -c lvq.c -o OBJ\LVQ.OBJ
%GCC% %CFLAGS% -c psnr.c -o OBJ\PSNR.OBJ
IF %ERRORLEVEL% NEQ 0 ECHO Compilation failed && EXIT /B 1

ECHO Linking...
%GCC% -static -static-libgcc -m32 -fopenmp -Wl,--allow-multiple-definition -o DIST\mll.exe OBJ\MLL.OBJ OBJ\LVQ.OBJ %LIBS% %ALLEGRO_LIBS%
%GCC% -static -static-libgcc -m32 -fopenmp -Wl,--allow-multiple-definition -o DIST\psnr.exe OBJ\PSNR.OBJ %LIBS% %ALLEGRO_LIBS%
IF %ERRORLEVEL% NEQ 0 ECHO Linking failed && EXIT /B 1

COPY /Y "%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Bin\x86\alleg42.dll" DIST\ >NUL

SET UPXE="%EXTDEV%\UPX-5.0.1-Win32\upx.exe"
IF EXIST %UPXE% (
    %UPXE% --best --quiet DIST\mll.exe DIST\psnr.exe DIST\alleg42.dll
)

ECHO SUCCESS: DIST\mll.exe DIST\psnr.exe
EXIT /B 0
