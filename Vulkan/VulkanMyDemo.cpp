#include <PVRShell/PVRShell.h>
#include <PVRApi/PVRApi.h>
#include <PVREngineUtils/PVREngineUtils.h>

const char * modelFilename = "teapot.pod";
const char * textureFileName = "Marble.pvr";
const char * vertexShaderFile = "VertShader_vk.spv";
const char * fragShaderFile = "FragShader_vk.spv";

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

	pvr::utils::AssetStore assetStore;
	pvr::GraphicsContext context;
	pvr::assets::ModelHandle modelHandle;
	std::auto_ptr<ApiObject> apiObject;

	pvr::Result drawMesh(int nodeIndex, pvr::api::CommandBuffer);
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
	apiObject->ubo.addEntryPacked("MVPMatrix", pvr::types::GpuDatatypes::mat4x4);
	for (int i = 0; i < getPlatformContext().getSwapChainLength(); ++i) {
		pvr::api::Buffer uboBuffer = context->createBuffer(apiObject->ubo.getAlignedTotalSize(), pvr::types::BufferBindingUse::UniformBuffer, true);
		pvr::api::BufferView uboBufferView = context->createBufferView(uboBuffer, 0, apiObject->ubo.getAlignedElementSize());
		apiObject->ubo.connectWithBuffer(i, uboBufferView, pvr::types::BufferViewTypes::UniformBuffer, pvr::types::MapBufferFlags::Write, 0);
		apiObject->uboDescSet[i] = context->createDescriptorSetOnDefaultPool(apiObject->uboDescSetLayout);
		descSetUpdate.setUbo(0, apiObject->ubo.getConnectedBuffer(i));
		apiObject->uboDescSet[i]->update(descSetUpdate);
	}

	return true;
}

bool MyDemo::configureCommandBuffer()
{
	apiObject->commandBuffer.resize(getPlatformContext().getSwapChainLength());
	for (pvr::uint32 i = 0; i < getPlatformContext().getSwapChainLength(); ++i)
	{
		apiObject->commandBuffer[i] = context->createCommandBufferOnDefaultPool();
		pvr::api::CommandBuffer commandBuffer = apiObject->commandBuffer[i];
		commandBuffer->beginRecording();
		//commandBuffer->beginRenderPass(apiObject->fbos[i], apiObject->renderPass, pvr::Rectanglei(0, 0, getWidth(), getHeight()), false, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 1.f, 0);
		commandBuffer->beginRenderPass(apiObject->fbos[i], pvr::Rectanglei(0, 0, getWidth(), getHeight()), true, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 1.f, 0);
		commandBuffer->bindPipeline(apiObject->graphicsPipeline);
		commandBuffer->bindDescriptorSet(apiObject->pipelineLayout, 0, apiObject->uboDescSet[i]);
		drawMesh(0, commandBuffer);
		commandBuffer->endRenderPass();
		commandBuffer->endRecording();
	}

	return true;
}

pvr::Result MyDemo::drawMesh(int nodeIndex, pvr::api::CommandBuffer commandBuffer)
{
	pvr::uint32 meshId = modelHandle->getNode(nodeIndex).getObjectId();
	pvr::assets::Mesh& mesh = modelHandle->getMesh(meshId);

	commandBuffer->bindVertexBuffer(apiObject->vbos[meshId], 0, 0);
	
	if (mesh.getNumStrips() != 0)
	{
		pvr::Log(pvr::Log.Error, "Model is not a triangle list");
		return pvr::Result::InvalidData;
	}
	else
	{
		if (apiObject->ibos[meshId].isValid())
		{
			commandBuffer->bindIndexBuffer(apiObject->ibos[meshId], 0, mesh.getFaces().getDataType());
			commandBuffer->drawIndexed(0, mesh.getNumFaces() * 3, 0, 0, 1);
		}
		else
		{
			commandBuffer->drawArrays(0, mesh.getNumFaces() * 3, 0, 1);
		}
	}

	return pvr::Result::Success;
}

bool MyDemo::configureFbo()
{
	apiObject->fbos = context->createOnScreenFboSet();

	return true;
}

bool MyDemo::configureGraphicsPipeline()
{
	pvr::api::GraphicsPipelineCreateParam pipelineInfo;
	pvr::types::BlendingConfig colorBlendState;
	
	colorBlendState.blendEnable = false;
	pipelineInfo.colorBlend.setAttachmentState(0, colorBlendState);

	pvr::api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
	descSetLayoutInfo.setBinding(0, pvr::types::DescriptorType::UniformBuffer, 1, pvr::types::ShaderStageFlags::AllGraphicsStages);
	apiObject->uboDescSetLayout = context->createDescriptorSetLayout(descSetLayoutInfo);

	pvr::api::PipelineLayoutCreateParam pipelineLayoutInfo;
	pipelineLayoutInfo.addDescSetLayout(apiObject->uboDescSetLayout);
	apiObject->pipelineLayout = context->createPipelineLayout(pipelineLayoutInfo);

	pipelineInfo.vertexShader = context->createShader(*getAssetStream(vertexShaderFile), pvr::types::ShaderType::VertexShader);
	pipelineInfo.fragmentShader = context->createShader(*getAssetStream(fragShaderFile), pvr::types::ShaderType::FragmentShader);
	if (!pipelineInfo.vertexShader.getShader().isValid() || !pipelineInfo.fragmentShader.getShader().isValid())
	{
		setExitMessage("Failed to create vertshader or Fragment shader");
		return false;
	}

	pvr::assets::Mesh mesh = modelHandle->getMesh(0);
	pipelineInfo.pipelineLayout = apiObject->pipelineLayout;
	pipelineInfo.inputAssembler.setPrimitiveTopology(mesh.getPrimitiveType());
	pipelineInfo.rasterizer.setCullFace(pvr::types::Face::Back);
	pipelineInfo.renderPass = apiObject->fbos[0]->getRenderPass();
	pipelineInfo.depthStencil.setDepthTestEnable(true).setDepthCompareFunc(pvr::types::ComparisonMode::Less).setDepthWrite(true);
	pvr::utils::createInputAssemblyFromMesh(mesh, vertexBinding_Names, sizeof(vertexBinding_Names) / sizeof(vertexBinding_Names[0]), pipelineInfo);

	apiObject->graphicsPipeline = context->createGraphicsPipeline(pipelineInfo);

	return (apiObject->graphicsPipeline.isValid());
}

pvr::Result MyDemo::initApplication()
{
	assetStore.init(*this);

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
	context = getGraphicsContext();
	apiObject.reset(new ApiObject());
	
	pvr::utils::appendSingleBuffersFromModel(getGraphicsContext(), *modelHandle, apiObject->vbos, apiObject->ibos);

	if (!configureFbo())
	{
		setExitMessage("Failed to create FBO");
		return pvr::Result::UnknownError;
	}

	if (!configureGraphicsPipeline())
	{
		setExitMessage("Failed to create graphics pipeline");
		return pvr::Result::UnknownError;
	}

	if (!configureUbo())
	{
		setExitMessage("Failed to create UBO");
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
	modelHandle.reset();
	assetStore.releaseAll();
	return pvr::Result::Success;
}

pvr::Result MyDemo::renderFrame()
{
	angleY = (glm::pi<pvr::float32>() / 150) * 0.05f * this->getFrameTime();
	modelMatrix = glm::rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f));
	mvp = mvp * modelMatrix;
	apiObject->ubo.map(context->getSwapChainIndex());
	apiObject->ubo.setValue(0, mvp);
	apiObject->ubo.unmap(context->getSwapChainIndex());

	apiObject->commandBuffer[getPlatformContext().getSwapChainIndex()]->submit();

	return pvr::Result::Success;
}

pvr::Result MyDemo::quitApplication()
{
	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo() { return std::auto_ptr<pvr::Shell>(new MyDemo()); }
