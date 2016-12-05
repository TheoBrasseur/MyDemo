#include <PVRShell/PVRShell.h>
#include <PVRApi/PVRApi.h>
#include <PVRUIRenderer/PVRUIRenderer.h>

const char * modelFilename = "teapot.pod";
const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_ES3.vsh";
const char * fragShaderFile = "FragShader_ES3.fsh";

pvr::float32 translateX = 0;
pvr::utils::VertexBindings_Name vertexBinding_Names[] = { {"POSITION", "inPositions"} };

class MyDemo : public pvr::Shell 
{
	struct ApiObject
	{
		pvr::api::GraphicsPipeline graphicsPipeline;
		pvr::GraphicsContext context;
		pvr::api::CommandBuffer commandBuffer0;
    pvr::api::CommandBuffer commandBuffer1;
		pvr::api::Fbo fbo0;
    pvr::api::Fbo fbo1;
		pvr::api::DescriptorSetLayout descSetLayout;
		pvr::api::PipelineLayout pipelineLayout;
		pvr::api::DescriptorSet descSet;
    pvr::api::RenderPass renderpass;
		std::vector<pvr::api::Buffer> vbos;
		std::vector<pvr::api::Buffer> ibos;
	};
  glm::mat4 modelMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 mvp;
  pvr::api::AssetStore assetStore;
  pvr::assets::ModelHandle modelHandle;
	std::auto_ptr<ApiObject> apiObject;
	pvr::ui::UIRenderer uiRenderer;

  pvr::Result createDescriptorSets();
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

bool MyDemo::configureCommandBuffer()
{
  pvr::api::ImageDataFormat colorImageDataFormat;
  pvr::api::ImageDataFormat depthImageDataFormat(pvr::PixelFormat::Depth24Stencil8);

  pvr::api::RenderPassColorInfo renderPassColorInfo(colorImageDataFormat, pvr::types::LoadOp::Load, pvr::types::StoreOp::Store, 1);

  pvr::api::RenderPassDepthStencilInfo depthStencilInfo(depthImageDataFormat, pvr::types::LoadOp::Load, pvr::types::StoreOp::Store, pvr::types::LoadOp::Load, pvr::types::StoreOp::Store, 1);

  pvr::api::RenderPassCreateParam renderPassInfo;
  renderPassInfo.setColorInfo(0, renderPassColorInfo);
  renderPassInfo.setDepthStencilInfo(depthStencilInfo);

  apiObject->renderpass = apiObject->context->createRenderPass(renderPassInfo);

  pvr::api::TextureView colorTexture;
  pvr::api::TextureView depthTexture;
  pvr::api::FboCreateParam fboInfo;

  fboInfo.setColor(0, colorTexture);
  fboInfo.setDepthStencil(depthTexture);

  apiObject->fbo0 = apiObject->context->createFbo(fboInfo);
  
  apiObject->commandBuffer0->beginRecording();
  apiObject->commandBuffer0->beginRenderPass(apiObject->fbo0, apiObject->renderpass, pvr::Rectanglei(0, 0, getWidth(), getHeight()), false, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
  apiObject->commandBuffer0->bindPipeline(apiObject->graphicsPipeline);
  apiObject->commandBuffer0->endRenderPass();
  apiObject->commandBuffer0->endRecording();

	apiObject->commandBuffer1->beginRecording();
	apiObject->commandBuffer1->beginRenderPass(apiObject->fbo0, pvr::Rectanglei(0, 0, this->getWidth(), this->getHeight()), true, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	apiObject->commandBuffer1->bindPipeline(apiObject->graphicsPipeline);
	apiObject->commandBuffer1->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->descSet);
	apiObject->commandBuffer1->setUniformPtr<glm::mat4>(apiObject->graphicsPipeline->getUniformLocation("mvp"), 1, &mvp);
	drawMesh(0);
	pvr::api::SecondaryCommandBuffer uiCmdBuffer = apiObject->context->createSecondaryCommandBufferOnDefaultPool();
	uiRenderer.beginRendering(uiCmdBuffer);
	uiRenderer.getDefaultTitle()->render();
	uiRenderer.getSdkLogo()->render();
	apiObject->commandBuffer1->enqueueSecondaryCmds(uiCmdBuffer);
	uiRenderer.endRendering();
	apiObject->commandBuffer1->endRenderPass();
	apiObject->commandBuffer1->endRecording();

	return true;
}

pvr::Result MyDemo::drawMesh(int nodeIndex)
{
	pvr::uint32 meshId = modelHandle->getNode(nodeIndex).getObjectId();
	pvr::assets::Mesh& mesh = modelHandle->getMesh(meshId);

	apiObject->commandBuffer0->bindVertexBuffer(apiObject->vbos[meshId], 0, 0);
  apiObject->commandBuffer0->bindIndexBuffer(apiObject->ibos[meshId], 0, mesh.getFaces().getDataType());

	if (mesh.getNumStrips() != 0)
	{
		pvr::Log(pvr::Log.Error, "Model is not a triangle list");
		return pvr::Result::InvalidData;
	}
	else
	{
		if (apiObject->ibos[meshId].isValid())
		{
			apiObject->commandBuffer0->drawIndexed(0, mesh.getNumFaces() * 3, 0, 0, 1);
		}
		else
		{
			apiObject->commandBuffer0->drawArrays(0, mesh.getNumFaces() * 3, 0, 1);
		}
	}

	return pvr::Result::Success;
}

pvr::Result MyDemo::createDescriptorSets()
{
  pvr::api::TextureStore colorTextureStore;
  pvr::api::TextureStore depthTextureStore;
  pvr::api::ImageStorageFormat colorImageStorageFormat; 
  pvr::api::ImageStorageFormat depthImageStorageFormat; 

  colorTextureStore->allocate2D(colorImageStorageFormat, getWidth(), getHeight(), pvr::types::ImageUsageFlags::ColorAttachment, pvr::types::ImageLayout::ColorAttachmentOptimal);
  depthTextureStore->allocate2D(depthImageStorageFormat, getWidth(), getHeight(), pvr::types::ImageUsageFlags::DepthStencilAttachment, pvr::types::ImageLayout::DepthStencilAttachmentOptimal);

  pvr::api::TextureView colorTextureView;
  pvr::api::TextureView depthTextureView;

	pvr::assets::SamplerCreateParam samplerInfo;

	samplerInfo.magnificationFilter = pvr::types::SamplerFilter::Linear;
	samplerInfo.minificationFilter = pvr::types::SamplerFilter::Linear;
	pvr::api::Sampler samplerColor = apiObject->context->createSampler(samplerInfo);

  samplerInfo.magnificationFilter = pvr::types::SamplerFilter::Nearest;
  samplerInfo.minificationFilter = pvr::types::SamplerFilter::Nearest;
  pvr::api::Sampler samplerDepth = apiObject->context->createSampler(samplerInfo);

	pvr::api::DescriptorSetUpdate descSetUpdate;
	descSetUpdate.setCombinedImageSampler(0, colorTextureView, samplerColor).setCombinedImageSampler(1, depthTextureView, samplerDepth);
	apiObject->descSet = apiObject->context->createDescriptorSetOnDefaultPool(apiObject->descSetLayout);
	if(!apiObject->descSet.isValid())
  {
    return pvr::Result::UnknownError;
  }
	apiObject->descSet->update(descSetUpdate);

	return pvr::Result::Success;
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
  apiObject->commandBuffer0 = apiObject->context->createCommandBufferOnDefaultPool();

  pvr::utils::appendSingleBuffersFromModel(getGraphicsContext(), *modelHandle, apiObject->vbos, apiObject->ibos); 

	if (!configureGraphicsPipeline())
	{
		return pvr::Result::UnknownError;
	}

	if (createDescriptorSets() != pvr::Result::Success)
	{
		return pvr::Result::UnknownError;
	}

  apiObject->fbo0 = apiObject->context->createOnScreenFbo(0);

  if (uiRenderer.init(apiObject->fbo0->getRenderPass(), 0) != pvr::Result::Success)
  {
    return pvr::Result::UnknownError;
  }

  glm::vec3 from, to, up;

  pvr::float32 fovy, near, far;

  far = 8.0f;
  near = 4.0f;
  from = glm::vec3(0.0f, 0.0f, 7.0f);
  to = glm::vec3(0.0f, 0.0f, -1.0f);
  up = glm::vec3(0.0f, 1.0f, 0.0f);
  fovy = glm::pi<pvr::float32>() / 2;

  glm::mat4 modelMatrix =  glm::rotate(glm::pi<pvr::float32>() / 8, glm::vec3(1.0f, 0.0f, 0.0f));

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
  translateX = this->getWidth() / 1000.0f * this->getFrameTime() / 1000.0f; 
  pvr::Log(pvr::Log.Information, "translateX: %f", translateX);
	modelMatrix = glm::translate(glm::vec3(translateX, 0.0, 0.00));
	mvp = mvp * modelMatrix;
	apiObject->commandBuffer0->submit();
	
	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
