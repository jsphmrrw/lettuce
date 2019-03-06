@echo off
if not exist build mkdir build
pushd build
cl -nologo /Zi ../source/lettuce_main.c /link /out:lettuce.exe
popd