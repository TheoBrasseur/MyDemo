#include <PVRShell/PVRShell.h>
#include <PVRApi/PVRApi.h>
#include <PVRUIRenderer/PVRUIRenderer.h>

const char * modelFilename = "teapot.pod";
const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_ES3.vsh";
const char * fragShaderFile = "FragShader_ES3.fsh";

pvr::float32 angleY = glm::pi<pvr::float32>()/16;
pvr::utils::VertexBindings_Name vertexBinding_Names[] = { {"POSITION", "inPositions"} };

class MyDemo : public pvr::Shell 
{
	struct ApiObject
	{
		pvr::api::GraphicsPipeline graphicsPipeline;
		pvr::GraphicsContext context;
		pvr::api::DescriptorSetLayout descSetLayout;
    pvr::api::RenderPass renderPass;
		pvr::api::PipelineLayout pipelineLayout;
		pvr::api::DescriptorSet descSet;
    pvr::api::FboSet fbos;
		std::vector<pvr::api::Buffer> vbos;
		std::vector<pvr::api::Buffer> ibos;
    pvr::Multi <pvr::api::CommandBuffer> commandBuffer;
	};
  glm::mat4 modelMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 mvp;

  pvr::api::AssetStore assetStore;
  pvr::assets::ModelHandle modelHandle;
	std::auto_ptr<ApiObject> apiObject;
	pvr::ui::UIRenderer uiRenderer;

  pvr::Result createDescriptorSet();
	pvr::Result drawMesh(int nodeIndex);
  bool configureFbo();
	bool configureGraphicsPipeline();
	bool configureCommandBuffer();

public:
	virtual pvr::Result initApplication();
	virtual pvr::Result initView();
	virtual pvr::Result quitApplication();
	virtual pvr::Result renderFrame();
	virtual pvr::Result releaseView();
};

bool MyDemo::configureCommandBuffer()
{
	for(pvr::uint32 i = 0; i < apiObject->context->getSwapChainLength(); i++)
  {
    apiObject->commandBuffer[i]->beginRecording();
    apiObject->commandBuffer[i]->beginRenderPass(apiObject->fbos[i], apiObject->renderPass, pvr::Rectanglei(0, 0, getWidth(), getHeight()), false, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 1.f, 0);
    apiObject->commandBuffer[i]->bindPipeline(apiObject->graphicsPipeline);
    apiObject->commandBuffer[i]->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->descSet);
    apiObject->commandBuffer[i]->setUniformPtr<glm::mat4>(apiObject->graphicsPipeline->getUniformLocation("mvp"), 1, &mvp);
    drawMesh(0);
    pvr::api::SecondaryCommandBuffer uiCmdBuffer = apiObject->context->createSecondaryCommandBufferOnDefaultPool();
    uiRenderer.beginRendering(uiCmdBuffer);
    uiRenderer.getDefaultTitle()->render();
    uiRenderer.getSdkLogo()->render();
    apiObject->commandBuffer[i]->enqueueSecondaryCmds(uiCmdBuffer);
    uiRenderer.endRendering();
    apiObject->commandBuffer[i]->endRenderPass();
    apiObject->commandBuffer[i]->endRecording();
  }

	return true;
}

pvr::Result MyDemo::drawMesh(int nodeIndex)
{
	pvr::uint32 meshId = modelHandle->getNode(nodeIndex).getObjectId();
	pvr::assets::Mesh& mesh = modelHandle->getMesh(meshId);

	apiObject->commandBuffer->bindVertexBuffer(apiObject->vbos[meshId], 0, 0);
  apiObject->commandBuffer->bindIndexBuffer(apiObject->ibos[meshId], 0, mesh.getFaces().getDataType());

	if (mesh.getNumStrips() != 0)
	{
		pvr::Log(pvr::Log.Error, "Model is not a triangle list");
		return pvr::Result::InvalidData;
	}
	else
	{
		if (apiObject->ibos[meshId].isValid())
		{
			apiObject->commandBuffer->drawIndexed(0, mesh.getNumFaces() * 3, 0, 0, 1);
		}
		else
		{
			apiObject->commandBuffer->drawArrays(0, mesh.getNumFaces() * 3, 0, 1);
		}
	}

	return pvr::Result::Success;
}

pvr::Result MyDemo::createDescriptorSet()
{
	pvr::api::DescriptorSetUpdate descSetUpdate;
	apiObject->descSet = apiObject->context->createDescriptorSetOnDefaultPool(apiObject->descSetLayout);
	if(!apiObject->descSet.isValid())
  {
    return pvr::Result::UnknownError;
  }
	apiObject->descSet->update(descSetUpdate);

	return pvr::Result::Success;
}

bool MyDemo::configureFbo()
{
  pvr::api::TextureStore colorTextureStore;
  pvr::api::TextureStore depthTextureStore;

  pvr::api::ImageStorageFormat colorImageStorageFormat;
  pvr::api::ImageStorageFormat depthImageStorageFormat;

  colorTextureStore = apiObject->context->createTexture();
  depthTextureStore = apiObject->context->createTexture();

  colorTextureStore->allocate2D(colorImageStorageFormat, getWidth(), getHeight(), pvr::types::ImageUsageFlags::ColorAttachment, pvr::types::ImageLayout::ColorAttachmentOptimal);
  depthTextureStore->allocate2D(depthImageStorageFormat, getWidth(), getHeight(), pvr::types::ImageUsageFlags::DepthStencilAttachment, pvr::types::ImageLayout::DepthStencilAttachmentOptimal);

  apiObject->fbos = apiObject->context->createOnScreenFboSet();
  apiObject->fbos.resize(apiObject->context->getSwapChainLength());

  std::vector <pvr::api::OnScreenFboCreateParam> fboInfo;
  fboInfo.resize(apiObject->context->getSwapChainLength());
  for (uint i = 0; i < apiObject->context->getSwapChainLength(); ++i) {
    pvr::api::TextureView colorTextureView = apiObject->context->createTextureView(colorTextureStore);
    pvr::api::TextureView depthTextureView = apiObject->context->createTextureView(depthTextureStore);
    fboInfo[i].setOffScreenColor(1, colorTextureView);
    fboInfo[i].setOffScreenDepthStencil(1, depthTextureView);
    apiObject->fbos[i] = apiObject->context->createOnScreenFboSet
  }                       
  pvr::api::ImageDataFormat colorDataFormat(pvr::PixelFormat::RGBA_8888, pvr::VariableType::UnsignedByteNorm, pvr::types::ColorSpace::lRGB);
  pvr::api::ImageDataFormat depthDataFormat(pvr::PixelFormat::Depth24Stencil8, pvr::VariableType::UnsignedByteNorm, pvr::types::ColorSpace::lRGB);

  pvr::api::RenderPassColorInfo renderPassColorInfo(colorDataFormat, pvr::types::LoadOp::Load, pvr::types::StoreOp::Store);
  pvr::api::RenderPassDepthStencilInfo renderPassDepthStencilInfo(depthDataFormat, pvr::types::LoadOp::Load, pvr::types::StoreOp::Store, pvr::types::LoadOp::Ignore, pvr::types::StoreOp::Ignore);


  pvr::api::RenderPassCreateParam renderPassInfo;
  renderPassInfo.setColorInfo(0, apiObject->context->getPresentationImageFormat());
  renderPassInfo.setColorInfo(1, renderPassColorInfo);
  renderPassInfo.setDepthStencilInfo(renderPassDepthStencilInfo);

  apiObject->context->createRenderPass(renderPassInfo);

  return true;
}

bool MyDemo::configureGraphicsPipeline()
{
	pvr::api::GraphicsPipelineCreateParam pipelineInfo;
  pvr::types::BlendingConfig colorBlendState;
  colorBlendState.blendEnable = false;

	pvr::assets::ShaderFile shaderFile;

  pvr::api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
  descSetLayoutInfo.setBinding(0, pvr::types::DescriptorType::StorageImage, 1, pvr::types::ShaderStageFlags::AllGraphicsStages);
  descSetLayoutInfo.setBinding(1, pvr::types::DescriptorType::StorageImage, 1, pvr::types::ShaderStageFlags::AllGraphicsStages);
    
  apiObject->descSetLayout = apiObject->context->createDescriptorSetLayout(descSetLayoutInfo);

  pipelineInfo.colorBlend.setAttachmentState(0, colorBlendState);

  pvr::api::PipelineLayoutCreateParam pipelineLayoutInfo;
  pipelineLayoutInfo.addDescSetLayout(apiObject->descSetLayout);

	shaderFile.populateValidVersions(vertexShaderFile, *this);
	pipelineInfo.vertexShader = apiObject->context->createShader(*shaderFile.getBestStreamForApi(apiObject->context->getApiType()), pvr::types::ShaderType::VertexShader);

	shaderFile.populateValidVersions(fragShaderFile, *this);
	pipelineInfo.fragmentShader = apiObject->context->createShader(*shaderFile.getBestStreamForApi(apiObject->context->getApiType()), pvr::types::ShaderType::FragmentShader);


  pvr::assets::Mesh mesh = modelHandle->getMesh(0);
	pipelineInfo.inputAssembler.setPrimitiveTopology(mesh.getPrimitiveType());
  pipelineInfo.inputAssembler.setPrimitiveTopology(pvr::types::PrimitiveTopology::TriangleList);
  apiObject->pipelineLayout = apiObject->context->createPipelineLayout(pipelineLayoutInfo);
	pipelineInfo.rasterizer.setCullFace(pvr::types::Face::Back);
	pipelineInfo.depthStencil.setDepthTestEnable(true).setDepthCompareFunc(pvr::types::ComparisonMode::Less).setDepthWrite(true);
	pvr::utils::createInputAssemblyFromMesh(mesh, vertexBinding_Names, sizeof(vertexBinding_Names) / sizeof(vertexBinding_Names[0]), pipelineInfo);

  apiObject->graphicsPipeline = apiObject->context->createGraphicsPipeline(pipelineInfo);
	
	apiObject->commandBuffer->beginRecording();
  apiObject->commandBuffer->bindPipeline(apiObject->graphicsPipeline);
	/* apiObject->commandBuffer->setUniform<pvr::int32>(apiObject->graphicsPipeline->getUniformLocation("sTexture"), 0); */
	apiObject->commandBuffer->endRecording();
	apiObject->commandBuffer->submit();
	
	return true;
}

pvr::Result MyDemo::initApplication()
{
  assetStore.init(*this);
  int numNode;

	// Load model
	if(!assetStore.loadModel(modelFilename, modelHandle, false))
  {
    pvr::Log(pvr::Log.Error, "Failed to load model for file: %s", modelFilename);
    return pvr::Result::NotFound;
  }
	return pvr::Result::Success;
}

pvr::Result MyDemo::initView()
{
	apiObject.reset(new ApiObject());
	apiObject->context = getGraphicsContext();
  for (int i = 0; i < apiObject->context->getSwapChainLength(); ++i)
  {
    apiObject->commandBuffer[i] = apiObject->context->createCommandBufferOnDefaultPool();
  }

  pvr::utils::appendSingleBuffersFromModel(getGraphicsContext(), *modelHandle, apiObject->vbos, apiObject->ibos); 

	if (!configureGraphicsPipeline())
	{
		return pvr::Result::UnknownError;
	}

	if (createDescriptorSet() != pvr::Result::Success)
	{
		return pvr::Result::UnknownError;
	}

  apiObject->fbos = apiObject->context->createOnScreenFbo(0);

  if (uiRenderer.init(apiObject->fbo->getRenderPass(), 0) != pvr::Result::Success)
  {
    return pvr::Result::UnknownError;
  }

  glm::vec3 from, to, up;

  pvr::float32 fovy, near, far;

  far = 100.0f;
  near = 0.1f;
  from = glm::vec3(7.0f, 2.0f, 0.0f);
  to = glm::vec3(0.0f, 0.0f, 1.0f);
  up = glm::vec3(0.0f, 1.0f, 0.0f);
  fovy = glm::pi<pvr::float32>() / 2;

  glm::mat4 modelMatrix =  glm::mat4(1, 0, 0, 0,
                                      0, 1, 0, 0,
                                      0, 0, 1, 0,
                                      0, 0, 0, 1);

  projMatrix = glm::perspectiveFov<pvr::float32>(fovy, this->getWidth(), this->getHeight(), near, far);

  viewMatrix = glm::lookAt(from, to, up);

  mvp = projMatrix * viewMatrix * modelMatrix; 

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
	modelHandle.reset();
  assetStore.releaseAll();
	return pvr::Result::Success;
}

pvr::Result MyDemo::renderFrame()
{
  angleY = (glm::pi<pvr::float32>()/150) * 0.05f * this->getFrameTime();
	modelMatrix = glm::rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f));
	mvp = mvp * modelMatrix;
	apiObject->commandBuffer->submit();

	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
