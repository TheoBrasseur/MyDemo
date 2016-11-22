#include <PVRShell/PVRShell.h>
#include <PVRApi/PVRApi.h>
#include <PVRUIRenderer/PVRUIRenderer.h>
#include <PVRApi/TextureUtils.h>

const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_ES3.vsh";
const char * fragShaderFile = "FragShader_ES3.fsh";

pvr::float32 vertices[] = { -0.9f, 0.0f, 0.0f,   0.0f, 0.0f,     0.9f, 0.0f, 0.0f,    1.0f, 0.0f,    0.0f, 0.9f, 0.9f,   0.5f, 0.5f };
pvr::uint16 indices[] = { 0, 1, 2 };

class MyDemo : public pvr::Shell 
{
	struct ApiObject
	{
		pvr::api::GraphicsPipeline graphicsPipeline;
		pvr::GraphicsContext context;
		pvr::api::CommandBuffer commandBuffer;
		pvr::api::Fbo fbo;
		pvr::api::DescriptorSetLayout descSetLayout;
		pvr::api::PipelineLayout pipelineLayout;
		pvr::api::DescriptorSet descSet;
		pvr::api::Buffer vbos;
		pvr::api::Buffer ibos;
	};

	std::auto_ptr<ApiObject> apiObject;
	pvr::ui::UIRenderer uiRenderer;

  bool loadGeometry(); 
  bool  createImageSamplerDescriptor();
	pvr::Result drawMesh(int nodeIndex);
	bool configureGraphicsPipeline();
	bool configureCommandBuffer();

public:
	virtual pvr::Result initApplication();
	virtual pvr::Result initView();
	virtual pvr::Result quitApplication();
	virtual pvr::Result renderFrame();
	virtual pvr::Result releaseView();
};

bool MyDemo::loadGeometry() 
{
  apiObject->vbos = apiObject->context->createBuffer(sizeof(vertices), pvr::types::BufferBindingUse::VertexBuffer, false);
  apiObject->ibos = apiObject->context->createBuffer(sizeof(indices), pvr::types::BufferBindingUse::IndexBuffer, false);
  apiObject->vbos->update(vertices, 0, sizeof(vertices));
  apiObject->ibos->update(indices, 0, sizeof(indices));
  
  return true;  
}

bool MyDemo::configureCommandBuffer()
{
	glm::mat4 identity =  glm::mat4(1, 0, 0, 0,
                                  0, 1, 0, 0,
                                  0, 0, 1, 0,
                                  0, 0, 0, 1);

	apiObject->commandBuffer->beginRecording();
	apiObject->commandBuffer->beginRenderPass(apiObject->fbo, pvr::Rectanglei(0, 0, this->getWidth(), this->getHeight()), true, glm::vec4(0.0f, 0.5f, 0.5f, 1.0f));
	apiObject->commandBuffer->bindPipeline(apiObject->graphicsPipeline);
	apiObject->commandBuffer->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->descSet);
	apiObject->commandBuffer->setUniformPtr<glm::mat4>(apiObject->graphicsPipeline->getUniformLocation("mvp"), 1, &identity);
	drawMesh(0);
	pvr::api::SecondaryCommandBuffer uiCmdBuffer = apiObject->context->createSecondaryCommandBufferOnDefaultPool();
	uiRenderer.beginRendering(uiCmdBuffer);
	uiRenderer.getDefaultTitle()->render();
	uiRenderer.getSdkLogo()->render();
	apiObject->commandBuffer->enqueueSecondaryCmds(uiCmdBuffer);
	uiRenderer.endRendering();
	apiObject->commandBuffer->endRenderPass();
	apiObject->commandBuffer->endRecording();

	return true;
}

pvr::Result MyDemo::drawMesh(int nodeIndex)
{
	apiObject->commandBuffer->bindVertexBuffer(apiObject->vbos, 0, 0);
  apiObject->commandBuffer->bindIndexBuffer(apiObject->ibos, 0, pvr::types::IndexType::IndexType16Bit);

  if (apiObject->ibos.isValid())
  {
    apiObject->commandBuffer->drawIndexed(0, 3, 0, 0, 1);
  }
  else
  {
    apiObject->commandBuffer->drawArrays(0, 3, 0, 1);
  }

	return pvr::Result::Success;
}

bool MyDemo::createImageSamplerDescriptor()
{
	pvr::api::TextureView textureView;
  pvr::assets::Texture texture;
  pvr::assets::assetReaders::TextureReaderPVR textureReader;

  if(!textureReader.newAssetStream(this->getAssetStream(textureFileName, true)))
  {
    pvr::Log(pvr::Log.Error, "Failed to create stream to texture file");
    return false;
  }
  if(!textureReader.openAssetStream())
  {
    pvr::Log(pvr::Log.Error, "Failed to open texture stream");
    return false;
  }
  if(!textureReader.readAsset(texture))
  {
    pvr::Log(pvr::Log.Error, "Failed to read stream into texture file");
    return false;
  }
  textureReader.closeAssetStream();

  if(pvr::utils::textureUpload(apiObject->context, texture, textureView) != pvr::Result::Success)
  {
    pvr::Log(pvr::Log.Error, "Failed to upload stream into texture file");
    return false;
  }

	pvr::assets::SamplerCreateParam samplerInfo;

	samplerInfo.magnificationFilter = pvr::types::SamplerFilter::Linear;
	samplerInfo.minificationFilter = pvr::types::SamplerFilter::Linear;

	pvr::api::Sampler sampler = apiObject->context->createSampler(samplerInfo);

	pvr::api::DescriptorSetUpdate descSetUpdate;
	descSetUpdate.setCombinedImageSampler(0, textureView, sampler);
	apiObject->descSet = apiObject->context->createDescriptorSetOnDefaultPool(apiObject->descSetLayout);
	if(!apiObject->descSet.isValid())
  {
    return false;
  }
	apiObject->descSet->update(descSetUpdate);

	return true;
}

bool MyDemo::configureGraphicsPipeline()
{
	pvr::api::GraphicsPipelineCreateParam pipelineInfo;
  pvr::types::BlendingConfig colorBlendState;
  colorBlendState.blendEnable = false;

	pvr::assets::ShaderFile shaderFile;

  pvr::api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
  descSetLayoutInfo.setBinding(0, pvr::types::DescriptorType::CombinedImageSampler, 1, pvr::types::ShaderStageFlags::Fragment);
  apiObject->descSetLayout = apiObject->context->createDescriptorSetLayout(descSetLayoutInfo);

  pipelineInfo.colorBlend.setAttachmentState(0, colorBlendState);

  pvr::api::PipelineLayoutCreateParam pipelineLayoutInfo;
  pipelineLayoutInfo.addDescSetLayout(apiObject->descSetLayout);

	shaderFile.populateValidVersions(vertexShaderFile, *this);
	pipelineInfo.vertexShader = apiObject->context->createShader(*shaderFile.getBestStreamForApi(apiObject->context->getApiType()), pvr::types::ShaderType::VertexShader);

	shaderFile.populateValidVersions(fragShaderFile, *this);
	pipelineInfo.fragmentShader = apiObject->context->createShader(*shaderFile.getBestStreamForApi(apiObject->context->getApiType()), pvr::types::ShaderType::FragmentShader);

	pipelineInfo.inputAssembler.setPrimitiveTopology(pvr::types::PrimitiveTopology::TriangleList);
  apiObject->pipelineLayout = apiObject->context->createPipelineLayout(pipelineLayoutInfo);
	pipelineInfo.rasterizer.setCullFace(pvr::types::Face::Back);
	pipelineInfo.depthStencil.setDepthTestEnable(true).setDepthCompareFunc(pvr::types::ComparisonMode::Less).setDepthWrite(true);

  pvr::assets::VertexAttributeLayout positionAttribLayout;
  positionAttribLayout.dataType = pvr::types::DataType::Float32;
  positionAttribLayout.offset = 0;
  positionAttribLayout.width = 3; 

  pvr::assets::VertexAttributeLayout texCoordAttribLayout;
  positionAttribLayout.dataType = pvr::types::DataType::Float32;
  positionAttribLayout.offset = 3 * sizeof(pvr::float32);
  positionAttribLayout.width = 2; 

  pipelineInfo.vertexInput.addVertexAttribute(0, 0, positionAttribLayout);
  pipelineInfo.vertexInput.addVertexAttribute(1, 0, texCoordAttribLayout);
  pipelineInfo.vertexInput.setInputBinding(0, 0, pvr::types::StepRate::Vertex);

  apiObject->graphicsPipeline = apiObject->context->createGraphicsPipeline(pipelineInfo);
	
	apiObject->commandBuffer->beginRecording();
  apiObject->commandBuffer->bindPipeline(apiObject->graphicsPipeline);
	apiObject->commandBuffer->setUniform<pvr::int32>(apiObject->graphicsPipeline->getUniformLocation("sTexture"), 0);
	apiObject->commandBuffer->endRecording();
	apiObject->commandBuffer->submit();
	
	return true;
}

pvr::Result MyDemo::initApplication()
{
	return pvr::Result::Success;
}

pvr::Result MyDemo::initView()
{
	apiObject.reset(new ApiObject());
	apiObject->context = getGraphicsContext();
  apiObject->commandBuffer = apiObject->context->createCommandBufferOnDefaultPool();

  if (!loadGeometry())
  {
    return pvr::Result::UnknownError;
  }

	if (!configureGraphicsPipeline())
	{
		return pvr::Result::UnknownError;
	}

	if (!createImageSamplerDescriptor())
	{
		return pvr::Result::UnknownError;
	}

  apiObject->fbo = apiObject->context->createOnScreenFbo(0);

  if (uiRenderer.init(apiObject->fbo->getRenderPass(), 0) != pvr::Result::Success)
  {
    return pvr::Result::UnknownError;
  }

  if(!configureCommandBuffer())
  {
    return pvr::Result::UnknownError;
  }

	return pvr::Result::Success;
}

pvr::Result MyDemo::releaseView()
{
	apiObject.reset();
	uiRenderer.release();
	return pvr::Result::Success;
}

pvr::Result MyDemo::renderFrame()
{
	apiObject->commandBuffer->submit();

	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
