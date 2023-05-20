@echo off
title Compiling %~n1.vmf
setlocal
choice /d N /t 5 /m "fast compile?"
if ERRORLEVEL 2 (set fast= ) else set fast=-fast
set gamepath=E:\lychy-mod\mp\game\mod_hl2
set VPROJECT=%gamepath%
set copypath="%gamepath%\maps\"
mkdir %copypath%
set compilerpath=E:\SteamLibrary\steamapps\common\Source SDK Base 2013 Multiplayer\bin
"%compilerpath%\vbsp.exe" -allowdynamicpropsasstatic -game  "%gamepath%" %1 
"%compilerpath%\vvis.exe"  %fast% -game "%gamepath%"  %1
"%compilerpath%\vrad.exe"  -staticproppolys -staticproplighting -textureshadows -both -final -game "%gamepath%" %1
"%compilerpath%\bspzip.exe" -deletecubemaps "%~dpn1.bsp"
title Copying to %copypath%

move "%~dp1\*.bsp" %copypath%
move "%~dp1\*.log" %copypath%
move "%~dp1\*.prt" %copypath%
move "%~dp1\*.lin" %copypath%
move "%~dp1\*.gl" %copypath%
title Done.
rundll32.exe cmdext.dll,MessageBeepStub
pause