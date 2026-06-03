@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
CD /D "%~dp0"

IF NOT EXIST DIST\mll.exe ECHO Run build.bat first && EXIT /B 1
WHERE magick >NUL 2>NUL || ECHO Image Magick required && EXIT /B 1

REM Extract first arg as path, rest as extra MLL args
SET TARGET=%~1
FOR /F "tokens=1,*" %%A IN ("%*") DO SET MLL_EXTRA=%%B
REM Default to DATA\* if no path given
FOR %%A IN ("%TARGET%") DO IF "%%~xA"=="" IF NOT EXIST "%TARGET%\" SET MLL_EXTRA=%* & SET TARGET=DATA\*

IF EXIST "%TARGET%\" SET TARGET=%TARGET%\*

MKDIR TMP 2>NUL

ECHO.
ECHO ====== BENCHMARK ======
ECHO Name                   MLL sz   JXL sz   Time      MLL PSNR      JXL PSNR
ECHO ----                   ------   ------   ----      --------      --------

FOR %%F IN (%TARGET%) DO (
    SET EXT=%%~xF
    SET GO=
    IF /I "!EXT!"==".png"  SET GO=1
    IF /I "!EXT!"==".jpg"  SET GO=1
    IF /I "!EXT!"==".jpeg" SET GO=1
    IF /I "!EXT!"==".webp" SET GO=1

    IF DEFINED GO (
        SET NAME=%%~nF

        magick "%%F" -type TrueColor -depth 8 BMP3:TMP\_ref.bmp >NUL 2>NUL

        PUSHD TMP
        ..\DIST\mll.exe _ref.bmp -yuv !MLL_EXTRA! >NUL 2>_b_log.txt
        IF !ERRORLEVEL! NEQ 0 (
            POPD
            ECHO !NAME!                 FAILED
        ) ELSE (
            FOR %%A IN (_ref.mll) DO SET MLLSZ=%%~zA

            magick _ref.bmp -quality 90 _ref.jxl >NUL 2>NUL
            magick _ref.jxl -type TrueColor -depth 8 BMP3:_j.bmp >NUL 2>NUL
            FOR %%A IN (_ref.jxl) DO SET JXLSZ=%%~zA

            FOR /F "tokens=2" %%T IN ('findstr "Time:" _b_log.txt') DO SET TIME=%%T

            ..\DIST\psnr.exe _ref.bmp _ref_d.bmp >_b_psnr.txt
            FOR /F "tokens=2" %%P IN ('findstr "PSNR:" _b_psnr.txt') DO SET MLLP=%%P

            ..\DIST\psnr.exe _ref.bmp _j.bmp >_j_psnr.txt
            FOR /F "tokens=2" %%P IN ('findstr "PSNR:" _j_psnr.txt') DO SET JXLP=%%P

            POPD
            ECHO !NAME!        !MLLSZ!    !JXLSZ!    !TIME! ms   !MLLP! dB    !JXLP! dB
        )

        DEL TMP\_ref.* TMP\_ref_d.bmp TMP\_j.bmp TMP\_b_log.txt TMP\_b_psnr.txt TMP\_j_psnr.txt 2>NUL
        SET GO=
    )
)

ECHO ======================
ECHO.
EXIT /B 0
