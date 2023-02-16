@echo off
title Compiling %~n1.vmf
setlocal
choice /d N /t 5 /m "fast compile?"
if ERRORLEVEL 2 (set fast= ) else set fast=-fast
set gamepath=C:\Program Files (x86)\Steam\steamapps\sourcemods\lychy
set VPROJECT=%gamepath%
set copypath="%gamepath%\maps\"
mkdir %copypath%
set compilerpath=C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin
"%compilerpath%\vbsp.exe" -verbose -game "%gamepath%" %1 
"%compilerpath%\vvis.exe" -verbose %fast% -game "%gamepath%"  %1
"%compilerpath%\vrad.exe" -bounce 21476548123  -verbose -staticproppolys -staticproplighting -textureshadows -both -final -game "%gamepath%" %1
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