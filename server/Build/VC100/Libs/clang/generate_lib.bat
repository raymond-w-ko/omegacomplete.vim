pushd %~dp0
set PATH=C:\MinGW32\bin;C:\cygwin\bin;%PATH%
objdump -p C:\clang\bin\clang.dll >objdump.tmp
grep clang_ objdump.tmp >filtered_exports.tmp
echo EXPORTS >clang.def
sed -E "s/^.8?\[.*?\]\s//" filtered_exports.tmp >>clang.def
unix2dos clang.def
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
lib /machine:i386 /def:clang.def
