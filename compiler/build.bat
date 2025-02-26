@echo off

SETLOCAL ENABLEDELAYEDEXPANSION

set prjdir=%CD%

set WarnDis= /wd4201 /wd4239 /wd4100 /wd4189 /wd4127 /wd4150 /wd4996 /wd4700
set CompDebOpt= /c /nologo /EHsc /Zi /Za /Od /MTd /W4 %WarnDis%    
set CompRelOpt= /D NDEBUG /nologo /EHsc /O2 /MT /W4 %WarnDis%  

set LinkDebOpt= /NOLOGO /INCREMENTAL:NO /DEBUG:FULL  
set LinkRelOpt= /NOLOGO /INCREMENTAL:NO  

echo ***DEBUG***
pushd build
set objs=

for %%v in ("%prjdir%\code\*.cpp") do cl %CompDebOpt% %%v
for %%v in ("%prjdir%\build\*.obj") do set objs=!objs! %%v

LINK /OUT:main.exe %LinkDebOpt% %objs%

main %prjdir%\test\main.lox

popd



