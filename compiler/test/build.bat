@echo off

SETLOCAL ENABLEDELAYEDEXPANSION

set prjdir=%CD%

pushd ..\build
main %prjdir%\main.lox
popd
