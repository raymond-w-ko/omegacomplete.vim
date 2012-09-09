pushd %~dp0
del *.tmp *.def *.lib *.exp
set PATH=C:\MinGW32\bin;C:\cygwin\bin;%PATH%
objdump -p C:\clang\bin\libclang.dll >objdump.tmp
grep clang_ objdump.tmp >filtered_exports.tmp
echo EXPORTS >libclang.def
sed -E "s/^.*?\[.*?\]\s//" filtered_exports.tmp >>libclang.def
unix2dos libclang.def
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
lib /machine:i386 /def:libclang.def
