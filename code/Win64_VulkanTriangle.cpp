

#define VK_USE_PLATFORM_WIN32_KHR
#include "win64_vulkantriangle.h"
#include <vulkan/vulkan.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "commonvulkan.h"
#include "commonwindows.h"
#include "surface.h"
#include "util.h"

#if VALIDATION_LAYERS
#include <vulkan/vk_layer.h>
#endif

void UpdateCamera(VkDevice logicalDevice, Camera& camera, uint32_t width, uint32_t height, float zoom, glm::vec3 rotation)
{
	camera.position.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
	camera.position.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
	camera.position.model = glm::mat4();
	camera.position.model = glm::rotate(camera.position.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	camera.position.model = glm::rotate(camera.position.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.position.model = glm::rotate(camera.position.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	void* destPointer;
	VkResult error = vkMapMemory(logicalDevice, camera.memory, 0, sizeof(CameraPosition), 0, &destPointer);
	Assert(error, "could not map uniform data in update camera");
	memcpy(destPointer, &camera.position, sizeof(CameraPosition));
	vkUnmapMemory(logicalDevice, camera.memory);
}

void PrepareVertexData(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memoryProperties, VkQueue queue, VkCommandPool cmdPool, VertexBuffer* vertexBuffer)
{
	size_t vertexBufferSize = vertexBuffer->vPos.size() * sizeof(Vertex);
	size_t indexBufferSize = vertexBuffer->iPos.size() * sizeof(uint32_t);

	StagingBuffers stagingBuffers;

	VkCommandBuffer copyCommandBuffer = NewCommandBuffer(logicalDevice, cmdPool);
	vertexBuffer->vBuffer = NewBuffer(logicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	vertexBuffer->iBuffer = NewBuffer(logicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	
	stagingBuffers.vertices = AllocBindDataToGPU(logicalDevice, memoryProperties, vertexBufferSize, vertexBuffer->vPos.data(), &vertexBuffer->vBuffer, &vertexBuffer->vMemory);
	stagingBuffers.indices = AllocBindDataToGPU(logicalDevice, memoryProperties, indexBufferSize, vertexBuffer->iPos.data(), &vertexBuffer->iBuffer, &vertexBuffer->iMemory);

	VkCommandBufferBeginInfo cbbInfo = {};
	cbbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkBufferCopy copyRegion = {};
	VkResult error = vkBeginCommandBuffer(copyCommandBuffer, &cbbInfo);
	Assert(error, "could not begin copy cmd buffer for vertex data");
	copyRegion.size = vertexBufferSize;

	vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.vertices.buffer, vertexBuffer->vBuffer, 1, &copyRegion);
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.indices.buffer, vertexBuffer->iBuffer, 1, &copyRegion);

	error = vkEndCommandBuffer(copyCommandBuffer);
	Assert(error, "could not end copy cmd buffer for vertex data");

	//submit copies to queue
	VkSubmitInfo csInfo = {};
	csInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	csInfo.commandBufferCount = 1;
	csInfo.pCommandBuffers = &copyCommandBuffer;

	error = vkQueueSubmit(queue, 1, &csInfo, nullptr);
	Assert(error, "could not submit cmd buffer to copy data");
	error = vkQueueWaitIdle(queue);
	Assert(error, "wait for queue during vertex data copy failed");

	//TODO sync?
	vkDestroyBuffer(logicalDevice, stagingBuffers.vertices.buffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBuffers.vertices.memory, nullptr);
	vkDestroyBuffer(logicalDevice, stagingBuffers.indices.buffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBuffers.indices.memory, nullptr);

	//binding desc
	vertexBuffer->vBindingDesc.resize(1);
	vertexBuffer->vBindingDesc[0].binding = VERTEX_BUFFER_BIND_ID;
	vertexBuffer->vBindingDesc[0].stride = sizeof(Vertex);
	vertexBuffer->vBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexBuffer->vBindingAttr.resize(2);
	//location 0 is position
	vertexBuffer->vBindingAttr[0].binding = VERTEX_BUFFER_BIND_ID;
	vertexBuffer->vBindingAttr[0].location = 0;
	vertexBuffer->vBindingAttr[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexBuffer->vBindingAttr[0].offset = 0;
	vertexBuffer->vBindingAttr[0].binding = 0;

	vertexBuffer->vBindingAttr[1].binding = VERTEX_BUFFER_BIND_ID;
	vertexBuffer->vBindingAttr[1].location = 1;
	vertexBuffer->vBindingAttr[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexBuffer->vBindingAttr[1].offset = sizeof(float) * 3;
	vertexBuffer->vBindingAttr[1].binding = 0;

	vertexBuffer->viInfo = {}; //must zero init to ensure pnext is zero (and everything else isnt trash)
	vertexBuffer->viInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexBuffer->viInfo.vertexBindingDescriptionCount = vertexBuffer->vBindingDesc.size();
	vertexBuffer->viInfo.pVertexBindingDescriptions = vertexBuffer->vBindingDesc.data();
	vertexBuffer->viInfo.vertexAttributeDescriptionCount = vertexBuffer->vBindingAttr.size();
	vertexBuffer->viInfo.pVertexAttributeDescriptions = vertexBuffer->vBindingAttr.data();

}

void SetupCommandBuffers(DeviceInfo* deviceInfo,
	std::vector<VkCommandBuffer>* drawCmdBuffers,
	uint32_t swapchainImageCount)
{
	drawCmdBuffers->resize(swapchainImageCount);
	*drawCmdBuffers = NewCommandBuffer(deviceInfo->device, deviceInfo->cmdPool, swapchainImageCount);
	deviceInfo->prePresentCmdBuffer = NewCommandBuffer(deviceInfo->device, deviceInfo->cmdPool);
	deviceInfo->postPresentCmdBuffer = NewCommandBuffer(deviceInfo->device, deviceInfo->cmdPool);
}

void PrepareCameraBuffers(VkDevice logicalDevice, 
	VkPhysicalDeviceMemoryProperties memoryProperties,
	Camera* camera,
	uint32_t width, 
	uint32_t height, 
	float zoom, 
	glm::vec3 rotation)
{
	camera->buffer = NewBuffer(logicalDevice, sizeof(CameraPosition), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	VkMemoryAllocateInfo aInfo = NewMemoryAllocInfo(logicalDevice, memoryProperties, camera->buffer, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	VkResult error;
	error = vkAllocateMemory(logicalDevice, &aInfo, nullptr, &camera->memory);
	Assert(error, "could not allocate memory in prepapre camera buffers");
	error = vkBindBufferMemory(logicalDevice, camera->buffer, camera->memory, 0);
	Assert(error, "could not bind buffer memory in prepare camera buffers");

	camera->desc.buffer = camera->buffer;
	camera->desc.offset = 0;
	camera->desc.range = sizeof(CameraPosition);
	UpdateCamera(logicalDevice, *camera, width, height, zoom, rotation);
}


VkPipeline PreparePipelines(VkDevice logicalDevice, PipelineInfo* pipelineInfo, VkPipelineVertexInputStateCreateInfo* vertexInputInfo)
{

	VkResult error;

	//vertex input
	VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
	iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	//rasterization state
	VkPipelineRasterizationStateCreateInfo rInfo = {};
	rInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rInfo.cullMode = VK_CULL_MODE_NONE;
	rInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rInfo.depthClampEnable = VK_FALSE;
	rInfo.rasterizerDiscardEnable = VK_FALSE;
	rInfo.depthBiasEnable = VK_FALSE;

	//color blend state (only one here)
	VkPipelineColorBlendAttachmentState baState[1] = {};
	baState[0].colorWriteMask = 0xf;
	baState[0].blendEnable - VK_FALSE;

	VkPipelineColorBlendStateCreateInfo cbInfo = {};
	cbInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cbInfo.attachmentCount = 1;
	cbInfo.pAttachments = baState;

	//viewport state
	VkPipelineViewportStateCreateInfo vInfo = {};
	vInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vInfo.viewportCount = 1;
	vInfo.scissorCount = 1;

	//dynamic states
	std::vector<VkDynamicState> dEnables;
	dEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

	VkPipelineDynamicStateCreateInfo dInfo = {};
	dInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dInfo.dynamicStateCount = dEnables.size();
	dInfo.pDynamicStates = dEnables.data();

	//depth and stencil
	VkPipelineDepthStencilStateCreateInfo dsInfo = {};
	dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	dsInfo.depthTestEnable = VK_TRUE;
	dsInfo.depthWriteEnable = VK_TRUE;
	dsInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	dsInfo.depthBoundsTestEnable = VK_FALSE;
	dsInfo.back.failOp = VK_STENCIL_OP_KEEP;
	dsInfo.back.passOp = VK_STENCIL_OP_KEEP;
	dsInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	dsInfo.stencilTestEnable = VK_FALSE;
	dsInfo.front = dsInfo.back;

	//multisampling
	VkPipelineMultisampleStateCreateInfo msInfo = {};
	msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msInfo.pSampleMask = nullptr;
	msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	File vertexShader = OpenFile("D:\\scottsdocs\\sourcecode\\VulkanTriangle\\code\\triangle.vert.spv");
	File fragmentShader = OpenFile("D:\\scottsdocs\\sourcecode\\VulkanTriangle\\code\\triangle.frag.spv");

	VkPipelineShaderStageCreateInfo sInfo[2] = {};
	sInfo[0] = NewShaderStageInfo(logicalDevice, (uint32_t*)vertexShader.data, vertexShader.size, VK_SHADER_STAGE_VERTEX_BIT);
	sInfo[1] = NewShaderStageInfo(logicalDevice, (uint32_t*)fragmentShader.data, fragmentShader.size, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo pInfo = {};
	pInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pInfo.layout = pipelineInfo->pipelineLayout;
	pInfo.renderPass = pipelineInfo->renderPass;
	pInfo.stageCount = 2;
	pInfo.pVertexInputState = vertexInputInfo;
	pInfo.pInputAssemblyState = &iaInfo;
	pInfo.pRasterizationState = &rInfo;
	pInfo.pColorBlendState = &cbInfo;
	pInfo.pMultisampleState = &msInfo;
	pInfo.pViewportState = &vInfo;
	pInfo.pDepthStencilState = &dsInfo;
	pInfo.pStages = sInfo;
	pInfo.pDynamicState = &dInfo;

	VkPipeline pipeline;
	error = vkCreateGraphicsPipelines(logicalDevice, pipelineInfo->pipelineCache, 1, &pInfo, nullptr, &pipeline);
	Assert(error, "could not create pipeline");
	return pipeline;
}

VkDescriptorPool PrepareDescriptorPool(VkDevice logicalDevice)
{
	VkDescriptorPoolSize typeCount[1];
	typeCount[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCount[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo dpInfo = {};
	dpInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dpInfo.poolSizeCount = 1;
	dpInfo.pPoolSizes = typeCount;
	dpInfo.maxSets = 1;
	VkDescriptorPool descriptorPool;
	VkResult error = vkCreateDescriptorPool(logicalDevice, &dpInfo, nullptr, &descriptorPool);
	Assert(error, "could not create descriptor pool");
	return descriptorPool;

}

void PrepareDescriptorSet(VkDevice logicalDevice, PipelineInfo* pipelineInfo, VkDescriptorBufferInfo uniformBufferInfo)
{

	VkDescriptorSetAllocateInfo dsInfo = {};
	dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsInfo.descriptorPool = pipelineInfo->descriptorPool;
	dsInfo.descriptorSetCount = 1;
	dsInfo.pSetLayouts = &pipelineInfo->descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkResult error = vkAllocateDescriptorSets(logicalDevice, &dsInfo, &descriptorSet);
	Assert(error, "could not allocate descriptor set");

	//binding 0: uniform buffer
	VkWriteDescriptorSet wdSet = {};
	wdSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wdSet.dstSet = descriptorSet;
	wdSet.descriptorCount = 1;
	wdSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	wdSet.pBufferInfo = &uniformBufferInfo;
	wdSet.dstBinding = 0;
	vkUpdateDescriptorSets(logicalDevice, 1, &wdSet, 0, nullptr);

}

void BuildCmdBuffers(DeviceInfo* deviceInfo, PipelineInfo* pipelineInfo, VkDescriptorSet descriptorSet, uint32_t width, uint32_t height)
{
	VkCommandBufferBeginInfo cbInfo = {};
	cbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
	VkClearValue clearVals[2] = {};
	clearVals[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clearVals[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo rbInfo = {};
	rbInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rbInfo.renderPass = pipelineInfo->renderPass;
	rbInfo.renderArea.offset = {0, 0};
	rbInfo.renderArea.extent = {width, height};
	rbInfo.clearValueCount = 2;
	rbInfo.pClearValues = clearVals;

	VkResult error;
	for (uint32_t i = 0; i < deviceInfo->drawCmdBuffers.size(); i++)
	{
		VkFramebuffer currFrame = deviceInfo->frameBuffers[i];
		VkCommandBuffer currCmd = deviceInfo->drawCmdBuffers[i];

		rbInfo.framebuffer = currFrame;

		error = vkBeginCommandBuffer(currCmd, &cbInfo);
		Assert(error, "could not begin command buffer");

		vkCmdBeginRenderPass(currCmd, &rbInfo, VK_SUBPASS_CONTENTS_INLINE);

		//update dynamic states
		VkViewport vp = {};
		vp.height = (float)height;
		vp.width = (float)width;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(currCmd, 0, 1, &vp);

		VkRect2D scissor = {};
		scissor.extent = { width, height };
		scissor.offset = { 0, 0 };
		vkCmdSetScissor(currCmd, 0, 1, &scissor);

		//bind descriptor sets describing shader bind points
		vkCmdBindDescriptorSets(currCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	}

}

void Init(MainMemory* m)
{

    m->running = true;
    m->consoleHandle = GetConsoleWindow();
    //ShowWindow(m->consoleHandle, SW_HIDE);
    m->clientWidth = 1200;
    m->clientHeight = 800;

    m->windowHandle = NewWindow(EXE_NAME, m, m->clientWidth, m->clientHeight);
	//ShowWindow(m->windowHandle, SW_HIDE);

#if VALIDATION_LAYERS
	std::vector<VkLayerProperties> layerProps = GetInstalledVkLayers();
	for (uint32_t i = 0; i < layerProps.size(); i++)
	{
		m->debugInfo.instanceLayerList.push_back(layerProps[i].layerName);
	}
#endif

	m->debugInfo.instanceExtList.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	m->debugInfo.instanceExtList.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#if VALIDATION_LAYERS
	m->debugInfo.instanceExtList.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    m->vkInstance = NewVkInstance(EXE_NAME, m->debugInfo->instanceLayerList, m->debugInfo->instanceExtList);
    m->surfaceInfo.surface = NewSurface(m->windowHandle, m->exeHandle, m->vkInstance);


    uint32_t gpuCount;
    std::vector<VkPhysicalDevice> physicalDevices = EnumeratePhysicalDevices(m->vkInstance, &gpuCount);
    m->physDeviceInfo.physicalDevice = physicalDevices[0];

    //see what the gpu is capable of
    //NOTE not actually used in other code
    vkGetPhysicalDeviceFeatures(m->physDeviceInfo.physicalDevice, &m->physDeviceInfo.deviceFeatures);

    vkGetPhysicalDeviceMemoryProperties(m->physDeviceInfo.physicalDevice, &m->physDeviceInfo.memoryProperties);

    m->physDeviceInfo.renderingQueueFamilyIndex = FindGraphicsQueueFamilyIndex(m->physDeviceInfo.physicalDevice, m->surfaceInfo.surface);

#if VALIDATION_LAYERS
    std::vector<VkLayerProperties> layerPropsDevice = GetInstalledVkLayers(m->physDeviceInfo.physicalDevice);
    for (uint32_t i = 0; i < layerPropsDevice.size(); i++)
    {
        m->debugInfo.deviceLayerList.push_back(layerPropsDevice[i].layerName);

    }
#endif
    m->deviceInfo.device = NewLogicalDevice(m->physDeviceInfo.physicalDevice, m->physDeviceInfo.renderingQueueFamilyIndex, m->debugInfo.deviceLayerList, m->debugInfo.deviceExtList);
    vkGetDeviceQueue(m->deviceInfo.device, m->physDeviceInfo.renderingQueueFamilyIndex, 0, &m->deviceInfo.queue);
    m->physDeviceInfo.supportedDepthFormat = GetSupportedDepthFormat(m->physDeviceInfo.physicalDevice);

    m->deviceInfo.presentComplete = NewSemaphore(m->deviceInfo.device);
    m->deviceInfo.renderComplete = NewSemaphore(m->deviceInfo.device);

    m->submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    m->submitInfo.pWaitDstStageMask = (VkPipelineStageFlags*)VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    m->submitInfo.waitSemaphoreCount = 1;
    m->submitInfo.pWaitSemaphores = &m->deviceInfo.presentComplete;
    m->submitInfo.signalSemaphoreCount = 1;
    m->submitInfo.pSignalSemaphores = &m->deviceInfo.renderComplete;


    GetSurfaceColorSpaceAndFormat(m->physicalDevice,
                                  m->surface,
                                  &m->surfaceColorFormat,
                                  &m->surfaceColorSpace);

    m->cmdPool = NewCommandPool(m->renderingQueueFamilyIndex, m->logicalDevice);
    m->setupCommandBuffer = NewSetupCommandBuffer(m->logicalDevice,
                                     m->cmdPool);
    InitSwapChain(m, &m->clientWidth, &m->clientHeight);
	SetupCommandBuffers(m->logicalDevice,
		&m->drawCmdBuffers,
		&m->prePresentCmdBuffer,
		&m->postPresentCmdBuffer,
		m->surfaceImageCount,
		m->cmdPool);
	setupDepthStencil(m->logicalDevice, 
		&m->depthStencil, 
		m->surfaceColorFormat, 
		m->clientWidth, 
		m->clientHeight, 
		m->memoryProperties, 
		m->setupCommandBuffer);

	m->renderPass = NewRenderPass(m->logicalDevice, m->surfaceColorFormat, m->supportedDepthFormat);
	m->pipelineCache = NewPipelineCache(m->logicalDevice);
	m->frameBuffers = NewFrameBuffer(m->logicalDevice,
		m->surfaceBuffers, 
		m->renderPass, 
		m->depthStencil.view, 
		m->surfaceImageCount, 
		m->clientWidth, 
		m->clientHeight);
	//TODO why does the setup cmd buffer need to be flushed and recreated?
	FlushSetupCommandBuffer(m->logicalDevice, m->cmdPool, &m->setupCommandBuffer, m->queue);
	m->setupCommandBuffer = NewSetupCommandBuffer(m->logicalDevice,
		m->cmdPool);
	//create cmd buffer for image barriers and converting tilings
	//TODO what are tilings?
	m->textureCmdBuffer = NewCommandBuffer(m->logicalDevice, m->cmdPool);

	m->vertexBuffer.vPos =
	{
		{ { 1.0f, 1.0f, 0.0f  },{ 1.0f, 0.0f, 0.0f } },
		{ { -1.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		{ { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
	};
	m->vertexBuffer.iPos = { 0, 1, 2 };
	m->vertexBuffer.iCount = 3;
	PrepareVertexData(m->logicalDevice, m->memoryProperties, m->queue, m->cmdPool, &m->vertexBuffer);
	PrepareCameraBuffers(m->logicalDevice, m->memoryProperties, &m->camera, m->clientWidth, m->clientHeight, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	m->descriptorSetlayout = NewDescriptorSetLayout(m->logicalDevice, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m->pipelineLayout = NewPipelineLayout(m->logicalDevice, m->descriptorSetlayout);
	m->pipeline = PreparePipelines(m->logicalDevice, m->pipelineLayout, m->renderPass, &m->vertexBuffer.viInfo, m->pipelineCache);
	m->descriptorPool = PrepareDescriptorPool(m->logicalDevice);
	PrepareDescriptorSet(m->logicalDevice, m->descriptorPool, m->descriptorSetlayout, m->camera.desc);

}


void UpdateAndRender(MainMemory* mainMemory)
{

}

void PollEvents(HWND windowHandle)
{
    MSG msg;
    while (PeekMessage(&msg, windowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
}

void Quit(MainMemory* m)
{
    DestroyWindow(m->windowHandle);
#if VALIDATION_LAYERS
	DestroyInstance(m->vkInstance, m->debugReport);
#else
	DestroyInstance(m->vkInstance);
#endif
}

//app entrypoint, replacing with winmain seems redundant to me (plus its function signature is annoying to remember)
int main(int argv, char** argc)
{

    MainMemory* mainMemory = new MainMemory();
    Init(mainMemory);
    while (mainMemory->running)
    {
        PollEvents(mainMemory->windowHandle);
        UpdateAndRender(mainMemory);
    }
    Quit(mainMemory);
    delete mainMemory;
    return 0;
}

