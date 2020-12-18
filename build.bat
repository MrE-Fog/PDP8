@echo off

if not exist build mkdir build
pushd build
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp

cl -nologo -Zi ..\src\pdp8.c

del lock.tmp
del *.obj

popd