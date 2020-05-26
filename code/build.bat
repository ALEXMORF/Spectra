@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

ctime -begin sdf_engine.ctm
cl -FC -nologo -Z7 -WX -W4 -wd4100 -wd4189 -wd4201 -wd4505 -wd4996 ..\code\main.cpp User32.lib D3D12.lib DXGI.lib D3DCompiler.lib

ctime -end sdf_engine.ctm

popd