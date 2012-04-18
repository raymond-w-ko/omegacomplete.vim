@ECHO OFF

pushd %~dp0

set PATH=C:\MinGW\bin

cls
mingw32-make %1 %2 %3 %4 %5

popd
