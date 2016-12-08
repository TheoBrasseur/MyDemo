@echo off
call ..\..\..\..\Builds\Spir-V\compile.bat VertShader.vsh GBufferVertexShader.vsh.spv vert
call ..\..\..\..\Builds\Spir-V\compile.bat FragShader.fsh GBufferFragmentShader.fsh.spv frag
