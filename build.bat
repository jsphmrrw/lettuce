@echo off
if not exist build mkdir build
pushd build
cl -nologo /Zi ../lettuce.c /link /out:lettuce.exe
popd