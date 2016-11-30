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
		pvr::api::CommandBuffer commandBuffer;
		pvr::api::Fbo fbo;
		pvr::api::DescriptorSetLayout descSetLayout;
		pvr::api::PipelineLayout pipelineLayout;
		pvr::api::DescriptorSet descSet;
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

  pvr::Result createImageSamplerDescriptor();
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
	apiObject->commandBuffer->beginRecording();
	apiObject->commandBuffer->beginRenderPass(apiObject->fbo, pvr::Rectanglei(0, 0, this->getWidth(), this->getHeight()), true, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	apiObject->commandBuffer->bindPipeline(apiObject->graphicsPipeline);
	apiObject->commandBuffer->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->descSet);
	apiObject->commandBuffer->setUniformPtr<glm::mat4>(apiObject->graphicsPipeline->getUniformLocation("mvp"), 1, &mvp);
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

pvr::Result MyDemo::createImageSamplerDescriptor()
{
	pvr::api::TextureView textureV;

	pvr::assets::SamplerCreateParam samplerInfo;

	samplerInfo.magnificationFilter = pvr::types::SamplerFilter::Linear;
	samplerInfo.minificationFilter = pvr::types::SamplerFilter::Linear;

	pvr::api::Sampler sampler = apiObject->context->createSampler(samplerInfo);

	// Load texture
  if(!assetStore.getTextureWithCaching(getGraphicsContext(), textureFileName, &textureV, NULL))
  {
    this->setExitMessage("Failed to load texture");
    return pvr::Result::UnknownError;
  }

	pvr::api::DescriptorSetUpdate descSetUpdate;
	descSetUpdate.setCombinedImageSampler(0, textureV, sampler);
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
  apiObject->commandBuffer = apiObject->context->createCommandBufferOnDefaultPool();

  pvr::utils::appendSingleBuffersFromModel(getGraphicsContext(), *modelHandle, apiObject->vbos, apiObject->ibos); 

	if (!configureGraphicsPipeline())
	{
		return pvr::Result::UnknownError;
	}

	if (createImageSamplerDescriptor() != pvr::Result::Success)
	{
		return pvr::Result::UnknownError;
	}

  apiObject->fbo = apiObject->context->createOnScreenFbo(0);

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
  translateX = this->getWidth() / 200 * this->getFrameTime();
	modelMatrix = glm::translate(glm::vec3(translateX, 0, 0));
	mvp = mvp * modelMatrix;
	apiObject->commandBuffer->submit();

	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
