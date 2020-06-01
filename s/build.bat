@echo off

mkdir build
pushd build

cl -GS -WX -W4 -wd4100 -wd4189 -wd4576 -wd4702 -wd4706 -FC -Zi -Od ..\code\vccnow.cpp

popd