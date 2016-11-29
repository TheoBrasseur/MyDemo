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
  GLuint renderBufferDepth;
  GLuint renderBufferColor;
  GLuint vbo;
  GLuint vao;
  GLuint fbo;
  GLuint texture;

  bool createShaderProgram(pvr::native::HShader_ shaders[], pvr::uint32 count, GLuint& shaderProgram);
  bool loadTexture();
  bool blit();
  bool checkCompleteness();

public:
	virtual pvr::Result initApplication();
	virtual pvr::Result initView();
	virtual pvr::Result quitApplication();
	virtual pvr::Result renderFrame();
	virtual pvr::Result releaseView();
};

bool MyDemo::checkCompleteness()
{
  GLenum result = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
  pvr::string message;
  bool isComplete = true;
  switch (result) {
    case GL_FRAMEBUFFER_COMPLETE:
      message = "framebuffer is complete.";
      isComplete = true;
      break;
    case GL_FRAMEBUFFER_UNDEFINED:
      message = "framebuffer undefined";
      isComplete = false;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      message = "framebuffer incomplete attachment";
      isComplete = false;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      message = "framebuffer missing attachment";
      isComplete = false;
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      message = "framebuffer unsupported";
      isComplete = false;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      message = "framebuffer incomplete dimensions";
      isComplete = false;
      break;
    default:
      message = "Unhandled framebuffer case";
      isComplete = false;
      break;
  }
  pvr::Log(pvr::Log.Error, "%s", message.c_str());
  return isComplete;
}

bool MyDemo::blit()
{
  GLuint tmpfbo, tmpRenderBufferColor, tmpRenderBufferDepth;
  gl::GenRenderbuffers(1, &tmpRenderBufferColor);
  gl::BindRenderbuffer(GL_RENDERBUFFER, tmpRenderBufferColor);
  gl::RenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, getWidth(), getHeight());

  gl::GenRenderbuffers(1, &tmpRenderBufferDepth);
  gl::BindRenderbuffer(GL_RENDERBUFFER, tmpRenderBufferDepth);
  gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, getWidth(), getHeight());

  gl::GenFramebuffers(1, &tmpfbo);
  gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, tmpfbo);
  gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, tmpRenderBufferColor);
  gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tmpRenderBufferDepth);

  GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
  gl::DrawBuffers(1, drawBuffers);

  gl::BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  gl::BlitFramebuffer(0, 0, getWidth(), getHeight(), 0, 0, getWidth(), getHeight(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  if(gl::GetError() == (GL_INVALID_OPERATION | GL_INVALID_FRAMEBUFFER_OPERATION))
  {
    pvr::Log(pvr::Log.Error, "Invalid blit");
    return false;
  }
  return true;
}

bool MyDemo::loadTexture()
{
  pvr::Result result;
  pvr::assets::Texture textureAsset;
  pvr::assets::assetReaders::TextureReaderPVR textureReader;

  if(!textureReader.newAssetStream(this->getAssetStream(textureFileName)))
  {
    pvr::Log(pvr::Log.Error, "textureAsset file not found");
    return false;
  }
  if(!textureReader.openAssetStream())
  {
    pvr::Log(pvr::Log.Error, "Failed to open textureAsset stream");
    return false;
  }
  if(!textureReader.readAsset(textureAsset))
  {
    pvr::Log(pvr::Log.Error, "Failed to read textureAsset stream");
    return false;
  }
  textureReader.closeAssetStream();

  GLsizei width, height;
  GLenum internalformat;

  width = textureAsset.getHeader().getWidth();
  height = textureAsset.getHeader().getHeight();

  pvr::float32 data[] = { 0.0f, 0.0f, 0.0f,  1.0f, 1.0f, 1.0f,  
                          1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f };

  gl::GenTextures(1, &texture);
  gl::ActiveTexture(GL_TEXTURE0);
  gl::BindTexture(GL_TEXTURE_2D, texture);
  /* gl::TexStorage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 2, 2); */
  /* gl::TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RGB, GL_FLOAT, data); */
  gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, data);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::Uniform1i(gl::GetUniformLocation(shaderProgram, "sTexture"), 0);

  return true;
}

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

  GLint param0, param1, param2;
  gl::GetIntegerv(GL_DEPTH_BITS, &param0);
  gl::GetIntegerv(GL_RED_BITS, &param1);
  gl::GetIntegerv(GL_ALPHA_BITS, &param2);
  pvr::Log(pvr::Log.Error, "Depth bits in default framebuffer: %d", param0);
  pvr::Log(pvr::Log.Error, "Red bits in default framebuffer: %d", param1);
  pvr::Log(pvr::Log.Error, "alpha bits in default framebuffer: %d", param2);

  // VBO bindings are not part of VAO state
  gl::GenBuffers(1, &vbo);
  gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
  gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STREAM_DRAW);
  gl::BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  //

  gl::GenRenderbuffers(1, &renderBufferColor);
  gl::BindRenderbuffer(GL_RENDERBUFFER, renderBufferColor);
  //Needs to be called before binding to FBO
  gl::RenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, getWidth(), getHeight());

  gl::GenRenderbuffers(1, &renderBufferDepth);
  gl::BindRenderbuffer(GL_RENDERBUFFER, renderBufferDepth);
  //Needs to be called before binding to FBO
  gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, getWidth(), getHeight());

  //FBO creation
  gl::GenFramebuffers(1, &fbo);
  gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
  
  gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBufferDepth);
  gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER, renderBufferColor);
  
  GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  gl::DrawBuffers(1, drawBuffers);

  if(!checkCompleteness())
  {
    pvr::Log(pvr::Log.Error, "Framebuffer is not complete");
    return pvr::Result::InvalidData;
  }

  gl::GenVertexArrays(1, &vao);
  gl::BindVertexArray(vao);
  gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
  gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), (void *) (3 * sizeof(GL_FLOAT)));
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  gl::EnableVertexAttribArray(0);
  gl::EnableVertexAttribArray(1);
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

  if(!loadTexture())
  {
    return pvr::Result::InvalidData;
  }

	return pvr::Result::Success;
}

pvr::Result MyDemo::renderFrame()
{
  gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl::UseProgram(shaderProgram);
  gl::BindVertexArray(vao);
  gl::DrawArrays(GL_TRIANGLES, 0, 3);

  if(!blit())
  {
    return pvr::Result::InvalidArgument;
  }

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
