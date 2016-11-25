#include <PVRShell/PVRShellNoPVRApi.h>
#include <PVRNativeApi/OGLES/NativeObjectsGles.h>
#include <PVRNativeApi/OGLES/OpenGLESBindings.h>
#include <PVRAssets/PVRAssets.h>
#include <PVRNativeApi/TextureUtils.h>
#include <PVRNativeApi/ShaderUtils.h>

const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_ES3.vsh";
const char * fragShaderFile = "FragShader_ES3.fsh";

GLfloat vertices[] = { -0.9f, 0.0f, 0.0f,   0.0f, 0.0f,     0.9f, 0.0f, 0.0f,    1.0f, 0.0f,    0.0f, 0.9f, 0.0f,   0.5f, 0.5f };

class MyDemo : public pvr::Shell 
{
  GLuint shaderProgram;
  pvr::native::HShader_ shaders[2];
  GLuint vbo;
  GLuint vao;
  GLuint texture;

  bool createShaderProgram(pvr::native::HShader_ shaders[], pvr::uint32 count, GLuint& shaderProgram);

public:
	virtual pvr::Result initApplication();
	virtual pvr::Result initView();
	virtual pvr::Result quitApplication();
	virtual pvr::Result renderFrame();
	virtual pvr::Result releaseView();
};

bool MyDemo::createShaderProgram(pvr::native::HShader_ shaders[], pvr::uint32 count, GLuint& shaderProgram)
{
  shaderProgram = gl::CreateProgram();
  for(int i = 0; i < count; i++)
  {
    gl::AttachShader(shaderProgram, shaders[i]);
  }

  GLint status;
  gl::LinkProgram(shaderProgram);
  gl::GetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
  if(!status)
  {
    std::string infoLog;
    pvr::int32 infoLogLength, charWritten;
    gl::GetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);
    infoLog.resize(infoLogLength);
    if(infoLogLength)
    {
      gl::GetProgramInfoLog(shaderProgram, infoLogLength, &charWritten, &infoLog[0]);
      pvr::Log(pvr::Log.Debug, infoLog.c_str());
    }   
    return false;
  }
  for(int i = 0; i < count; i++)
  {
    gl::DetachShader(shaderProgram, shaders[i]);
    gl::DeleteShader(shaders[i]);
  }

  return true;
}
pvr::Result MyDemo::initApplication()
{
	return pvr::Result::Success;
}

pvr::Result MyDemo::initView()
{
  gl::initGl();
  // VBO attachments are not part of VAO state
  gl::GenBuffers(1, &vbo);
  gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
  gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STREAM_DRAW);
  gl::BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  //
  gl::GenVertexArrays(1, &vao);
  gl::BindVertexArray(vao);
  gl::EnableVertexAttribArray(0);
  gl::EnableVertexAttribArray(1);
  gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * GL_FLOAT, 0);
  gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * GL_FLOAT, (void *) (3 * sizeof(GL_FLOAT)));
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  gl::BindVertexArray(0);
  gl::DeleteBuffers(1, &vbo);
  gl::DisableVertexAttribArray(0);
  gl::DisableVertexAttribArray(1);

  pvr::assets::ShaderFile shaderfile;
  shaderfile.populateValidVersions(vertexShaderFile, *this);

  if(!pvr::utils::loadShader(pvr::native::HContext_(), *shaderfile.getBestStreamForApi(pvr::Api::OpenGLES3), pvr::types::ShaderType::VertexShader, 0, 0, shaders[0]))
  {
    return pvr::Result::InvalidData;
  }

  shaderfile.populateValidVersions(fragShaderFile, *this);
  if(!pvr::utils::loadShader(pvr::native::HContext_(), *shaderfile.getBestStreamForApi(pvr::Api::OpenGLES3), pvr::types::ShaderType::FragmentShader, 0, 0, shaders[1]))
  {
    return pvr::Result::InvalidData;
  }

  if(!createShaderProgram(shaders, 2, shaderProgram))
  {
    return pvr::Result::InvalidData;
  }

  gl::CullFace(GL_BACK);
  gl::Enable(GL_CULL_FACE);
  gl::Enable(GL_DEPTH_TEST);
  gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  /* if(!loadTexture()) */

	return pvr::Result::Success;
}

pvr::Result MyDemo::renderFrame()
{
  gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl::UseProgram(shaderProgram);
  gl::BindVertexArray(vao);
  gl::DrawArrays(GL_TRIANGLES, 0, 3);
  gl::BindVertexArray(0);
  gl::UseProgram(0);

	return pvr::Result::Success;
}

pvr::Result MyDemo::releaseView()
{
  gl::DeleteProgram(shaderProgram);
  gl::DeleteVertexArrays(1, &vao);
  gl::DeleteTextures(1, &texture);
  return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
