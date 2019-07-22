REM @echo off
subst w: "C:\Users\logan\source\repos"
set path = %path%;w:\handmade\misc
Call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d W:\handmade
code .
pause