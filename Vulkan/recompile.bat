@echo off
call ..\..\..\..\Builds\Spir-V\compile.bat VertShader_vk.vsh GBufferVertexShader.vsh.spv vert
call ..\..\..\..\Builds\Spir-V\compile.bat FragShader_vk.fsh GBufferFragmentShader.fsh.spv frag
