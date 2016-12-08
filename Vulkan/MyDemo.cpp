#include <PVRShell/PVRShell.h>
#include <PVRApi/PVRApi.h>
#include <PVRUIRenderer/PVRUIRenderer.h>

const char * modelFilename = "teapot.pod";
const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_ES3.vsh";
const char * fragShaderFile = "FragShader_ES3.fsh";

pvr::float32 angleY = glm::pi<pvr::float32>() / 16;
pvr::utils::VertexBindings_Name vertexBinding_Names[] = { {"POSITION", "inPositions"} };

class MyDemo : public pvr::Shell
{
	struct ApiObject
	{
		pvr::api::GraphicsPipeline graphicsPipeline;	
		pvr::api::RenderPass renderPass;
		pvr::api::PipelineLayout pipelineLayout;
		pvr::api::DescriptorSetLayout uboDescSetLayout;
		std::vector<pvr::api::DescriptorSet> uboDescSet;
		pvr::api::FboSet fbos;
		std::vector<pvr::api::Buffer> vbos;
		std::vector<pvr::api::Buffer> ibos;
		std::vector <pvr::api::CommandBuffer> commandBuffer;
		pvr::utils::StructuredMemoryView ubo;
	};
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::mat4 mvp;

	pvr::GraphicsContext context;
	pvr::api::AssetStore assetStore;
	pvr::assets::ModelHandle modelHandle;
	std::auto_ptr<ApiObject> apiObject;
	pvr::ui::UIRenderer uiRenderer;

	pvr::Result drawMesh(int nodeIndex, int swapChainIndex);
  bool configureUbo();
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

bool MyDemo::configureUbo()
{
  pvr::api::DescriptorSetUpdate descSetUpdate;
  apiObject->uboDescSet.resize(context->getSwapChainLength());
  for (int i = 0; i < context->getSwapChainLength(); ++i) {
    apiObject->ubo.addEntryPacked("MVPMatrix", pvr::types::GpuDatatypes::mat4x4, i);
    pvr::api::Buffer uboBuffer = context->createBuffer(apiObject->ubo.getAlignedTotalSize(), pvr::types::BufferBindingUse::UniformBuffer, 0);
    pvr::api::BufferView uboBufferView = context->createBufferView(uboBuffer, 0, uboBuffer->getSize());
    apiObject->ubo.connectWithBuffer(i, uboBufferView, pvr::types::BufferViewTypes::UniformBuffer, pvr::types::MapBufferFlags::Write, 0);
    apiObject->uboDescSet[i] = context->createDescriptorSetOnDefaultPool(apiObject->uboDescSetLayout);
    descSetUpdate.setUbo(0, apiObject->ubo.getConnectedBuffer(i)); 
    apiObject->uboDescSet[i]->update(descSetUpdate);
  }

  return true;
}


bool MyDemo::configureCommandBuffer()
{
	for (pvr::uint32 i = 0; i < context->getSwapChainLength(); i++)
	{
		apiObject->commandBuffer[i]->beginRecording();
		apiObject->commandBuffer[i]->beginRenderPass(apiObject->fbos[i], apiObject->renderPass, pvr::Rectanglei(0, 0, getWidth(), getHeight()), false, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 1.f, 0);
		apiObject->commandBuffer[i]->bindPipeline(apiObject->graphicsPipeline);
		apiObject->commandBuffer[i]->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->uboDescSet[i]);
		drawMesh(0, i);
		pvr::api::SecondaryCommandBuffer uiCmdBuffer = context->createSecondaryCommandBufferOnDefaultPool();
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

pvr::Result MyDemo::drawMesh(int nodeIndex, int swapChainIndex)
{
	pvr::uint32 meshId = modelHandle->getNode(nodeIndex).getObjectId();
	pvr::assets::Mesh& mesh = modelHandle->getMesh(meshId);

  apiObject->commandBuffer[swapChainIndex]->bindVertexBuffer(apiObject->vbos[meshId], 0, 0);
  apiObject->commandBuffer[swapChainIndex]->bindIndexBuffer(apiObject->ibos[meshId], 0, mesh.getFaces().getDataType());

	if (mesh.getNumStrips() != 0)
	{
		pvr::Log(pvr::Log.Error, "Model is not a triangle list");
		return pvr::Result::InvalidData;
	}
	else
	{
		if (apiObject->ibos[meshId].isValid())
		{
			for (pvr::uint16 i = 0; i < context->getSwapChainLength(); i++)
			{
				apiObject->commandBuffer[swapChainIndex]->drawIndexed(0, mesh.getNumFaces() * 3, 0, 0, 1);
			}
		}
		else
		{
			for (pvr::uint16 i = 0; i < context->getSwapChainLength(); i++)
			{
				apiObject->commandBuffer[swapChainIndex]->drawArrays(0, mesh.getNumFaces() * 3, 0, 1);
			}
		}
	}

	return pvr::Result::Success;
}

bool MyDemo::configureFbo()
{
  apiObject->fbos.resize(context->getSwapChainLength());
	apiObject->fbos = context->createOnScreenFboSet();

	/* std::vector <pvr::api::OnScreenFboCreateParam> fbosInfo; */
	/* fbosInfo.resize(context->getSwapChainLength()); */

	/* for (size_t i = 0; i < context->getSwapChainLength(); i++) */
	/* { */
    /* /1* fbosInfo[i]. *1/ */
    /* context->createOnS */
	/* } */

	return true;
}

bool MyDemo::configureGraphicsPipeline()
{
	pvr::api::GraphicsPipelineCreateParam pipelineInfo;
	pvr::types::BlendingConfig colorBlendState;
	colorBlendState.blendEnable = false;

	pvr::assets::ShaderFile shaderFile;

	pvr::api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
	descSetLayoutInfo.setBinding(0, pvr::types::DescriptorType::UniformBufferDynamic, 1, pvr::types::ShaderStageFlags::AllGraphicsStages);

	apiObject->uboDescSetLayout = context->createDescriptorSetLayout(descSetLayoutInfo);

	pipelineInfo.colorBlend.setAttachmentState(0, colorBlendState);

	pvr::api::PipelineLayoutCreateParam pipelineLayoutInfo;
	pipelineLayoutInfo.addDescSetLayout(apiObject->uboDescSetLayout);

	shaderFile.populateValidVersions(vertexShaderFile, *this);
	pipelineInfo.vertexShader = context->createShader(*shaderFile.getBestStreamForApi(context->getApiType()), pvr::types::ShaderType::VertexShader);

	shaderFile.populateValidVersions(fragShaderFile, *this);
	pipelineInfo.fragmentShader = context->createShader(*shaderFile.getBestStreamForApi(context->getApiType()), pvr::types::ShaderType::FragmentShader);

	pvr::assets::Mesh mesh = modelHandle->getMesh(0);
	pipelineInfo.inputAssembler.setPrimitiveTopology(mesh.getPrimitiveType());
	pipelineInfo.inputAssembler.setPrimitiveTopology(pvr::types::PrimitiveTopology::TriangleList);
	apiObject->pipelineLayout = context->createPipelineLayout(pipelineLayoutInfo);
	pipelineInfo.rasterizer.setCullFace(pvr::types::Face::Back);
	pipelineInfo.depthStencil.setDepthTestEnable(true).setDepthCompareFunc(pvr::types::ComparisonMode::Less).setDepthWrite(true);
	pvr::utils::createInputAssemblyFromMesh(mesh, vertexBinding_Names, sizeof(vertexBinding_Names) / sizeof(vertexBinding_Names[0]), pipelineInfo);

	apiObject->graphicsPipeline = context->createGraphicsPipeline(pipelineInfo);

	for (int i = 0; i < context->getSwapChainLength(); ++i) {
    apiObject->commandBuffer[i]->beginRecording();
    apiObject->commandBuffer[i]->bindPipeline(apiObject->graphicsPipeline);
    /* apiObject->commandBuffer[i]->setUniform<pvr::int32>(apiObject->graphicsPipeline->getUniformLocation("sTexture"), 0); */
    apiObject->commandBuffer[i]->endRecording();
    apiObject->commandBuffer[i]->submit();
	}

	return true;
}

pvr::Result MyDemo::initApplication()
{
	assetStore.init(*this);
	int numNode;

	// Load model
	if (!assetStore.loadModel(modelFilename, modelHandle, false))
	{
		pvr::Log(pvr::Log.Error, "Failed to load model for file: %s", modelFilename);
		return pvr::Result::NotFound;
	}
	return pvr::Result::Success;
}

pvr::Result MyDemo::initView()
{
	apiObject.reset(new ApiObject());
	context = getGraphicsContext();
	for (int i = 0; i < context->getSwapChainLength(); ++i)
	{
		apiObject->commandBuffer[i] = context->createCommandBufferOnDefaultPool();
	}

	pvr::utils::appendSingleBuffersFromModel(getGraphicsContext(), *modelHandle, apiObject->vbos, apiObject->ibos);

	if (!configureGraphicsPipeline())
	{
		return pvr::Result::UnknownError;
	}

	if (!configureUbo())
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

	glm::mat4 modelMatrix = glm::mat4(1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	projMatrix = glm::perspectiveFov<pvr::float32>(fovy, this->getWidth(), this->getHeight(), near, far);

	viewMatrix = glm::lookAt(from, to, up);

	mvp = projMatrix * viewMatrix * modelMatrix;

	if (!configureCommandBuffer())
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
  angleY = (glm::pi<pvr::float32>() / 150) * 0.05f * this->getFrameTime();
  modelMatrix = glm::rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f));
  mvp = mvp * modelMatrix;
  apiObject->ubo.map(context->getSwapChainIndex(), pvr::types::MapBufferFlags::Write, 0);
  apiObject->ubo.setValue(0, mvp);
  apiObject->ubo.unmap(context->getSwapChainIndex());

	for (int i = 0; i < context->getSwapChainLength(); ++i) {
    pvr::api::DescriptorSetUpdate descSetUpdate;
    descSetUpdate.setUbo(0, apiObject->mvpBufferView);
	  apiObject->uboDescSet[i]->update(descSetUpdate);
    apiObject->commandBuffer[i]->submit();
	}

	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
