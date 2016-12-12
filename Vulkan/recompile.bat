@echo off
<<<<<<< HEAD
call ..\..\..\..\Builds\Spir-V\compile.bat VertShader_vk.vsh VertShader_vk.spv vert
call ..\..\..\..\Builds\Spir-V\compile.bat FragShader_vk.fsh FragShader_vk.spv frag
=======
call ..\..\..\..\Builds\Spir-V\compile.bat VertShader.vsh GBufferVertexShader.vsh.spv vert
call ..\..\..\..\Builds\Spir-V\compile.bat FragShader.fsh GBufferFragmentShader.fsh.spv frag
>>>>>>> 0db5ac3ba1217b05d46a9dc0c88fbeea0a244fb3
