@echo off

mkdir build
pushd build

cl -WX -W4 -wd4100 -wd4576 -FC -Zi -Fe:vccnow.exe ..\src\vccnow.cpp
REM cl -WX -W4 -wd4576 -FC -Zi -Fe:pp.exe ..\src\preproc_cmdline.cpp
REM cl -WX -W4 -wd4576 -FC -Zi -Fe:dc.exe ..\src\decompile.cpp

popd