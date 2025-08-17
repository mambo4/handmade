@REM REM @echo off
@REM subst w: "C:\Users\logan\source\repos"
@REM set path = %path%;w:\handmade\misc
@REM Call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
@REM cd /d W:\handmade
@REM code .
@REM pause

:: edit %msdev_cmd% to your chosen vcvars bat file
:: edit  %root% to your actual project root (i.e, parent root of .vscode)
:: set msdev_cmd="%programfiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
set msdev_cmd="%programfiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
set root=W:/handmade

::leave this alone
set path = %path%;%root%
Call %msdev_cmd%
echo root: %root%
cd /d %root%
code .
pause