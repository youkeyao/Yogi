#include "Layers/ImGuiEndLayer.h"
#include "Utils/ImGuiBackends.h"

namespace Yogi
{

ImGuiEndLayer::ImGuiEndLayer() : Layer("ImGuiEndLayer")
{
    RendererInit();
}

ImGuiEndLayer::~ImGuiEndLayer()
{
    m_commandBuffer->Wait();
    m_commandBuffer = nullptr;
    m_renderPass    = nullptr;
    m_frameBuffers.clear();
    WindowShutdown();
    RendererShutdown();
}

void ImGuiEndLayer::OnUpdate(Timestep ts)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::End();
    ImGui::Render();
    RendererDraw();

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiEndLayer::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(YG_BIND_FN(ImGuiEndLayer::OnWindowResize));
}

// ---------------------------------------------------------------------------------------------
bool ImGuiEndLayer::OnWindowResize(WindowResizeEvent& e)
{
    m_commandBuffer->Wait();
    m_frameBuffers.clear();
    return false;
}

void ImGuiEndLayer::WindowShutdown()
{
#ifdef YG_WINDOW_GLFW
    ImGui_ImplGlfw_Shutdown();
#endif
}

void ImGuiEndLayer::RendererInit()
{
#ifdef YG_RENDERER_VULKAN
    ImGui_ImplVulkan_InitInfo initInfo  = {};
    VulkanDeviceContext*      context   = static_cast<VulkanDeviceContext*>(Application::GetInstance().GetContext());
    VulkanSwapChain*          swapChain = static_cast<VulkanSwapChain*>(Application::GetInstance().GetSwapChain());

    VkExtent2D extent = { swapChain->GetWidth(), swapChain->GetHeight() };

    m_renderPass = ResourceManager::AcquireSharedResource<IRenderPass>(
        RenderPassDesc{ { AttachmentDesc{ swapChain->GetColorFormat(), ResourceState::Present } },
                        AttachmentDesc{ ITexture::Format::NONE, ResourceState::FragmentShaderResource },
                        swapChain->GetNumSamples() });

    QueueFamilyIndices indices = FindQueueFamilies(context->GetVkPhysicalDevice(), context->GetVkSurface());
    initInfo.Instance          = context->GetVkInstance();
    initInfo.PhysicalDevice    = context->GetVkPhysicalDevice();
    initInfo.Device            = context->GetVkDevice();
    initInfo.QueueFamily       = indices.graphicsFamily.value();
    initInfo.Queue             = context->GetGraphicsQueue();
    initInfo.PipelineCache     = VK_NULL_HANDLE;
    initInfo.DescriptorPool    = context->GetVkDescriptorPool();
    initInfo.RenderPass        = static_cast<VulkanRenderPass*>(m_renderPass.Get())->GetVkRenderPass();
    initInfo.Subpass           = 0;
    initInfo.MinImageCount     = swapChain->GetImageCount();
    initInfo.ImageCount        = swapChain->GetImageCount();
    initInfo.MSAASamples       = (VkSampleCountFlagBits)swapChain->GetNumSamples();
    initInfo.Allocator         = nullptr;
    initInfo.CheckVkResultFn   = nullptr;
    ImGui_ImplVulkan_LoadFunctions(0, &VkLoadFunction, initInfo.Instance);
    ImGui_ImplVulkan_Init(&initInfo);

    m_commandBuffer = ResourceManager::CreateResource<ICommandBuffer>(
        CommandBufferDesc{ CommandBufferUsage::Persistent, SubmitQueue::Graphics });
#endif
}

void ImGuiEndLayer::RendererDraw()
{
    ImDrawData* mainDrawData = ImGui::GetDrawData();
#ifdef YG_RENDERER_VULKAN
    auto            swapChain     = Application::GetInstance().GetSwapChain();
    auto            currentTarget = swapChain->GetCurrentTarget();
    FrameBufferDesc desc{
        swapChain->GetWidth(), swapChain->GetHeight(), m_renderPass.Get(), { currentTarget }, nullptr,
    };
    uint64_t key = HashArgs(desc);
    auto     it  = m_frameBuffers.find(key);
    if (it == m_frameBuffers.end())
    {
        it = m_frameBuffers.insert({ key, ResourceManager::AcquireSharedResource<IFrameBuffer>(desc) }).first;
    }
    auto& frameBuffer = it->second;

    m_commandBuffer->Begin();
    m_commandBuffer->BeginRenderPass(
        m_renderPass.Get(), frameBuffer.Get(), { ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, ClearValue{ 1.0f, 0 });
    ImGui_ImplVulkan_RenderDrawData(mainDrawData,
                                    static_cast<VulkanCommandBuffer*>(m_commandBuffer.Get())->GetVkCommandBuffer());
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->End();
    m_commandBuffer->Submit();
    m_commandBuffer->Wait();
#endif
}

void ImGuiEndLayer::RendererShutdown()
{
#ifdef YG_RENDERER_VULKAN
    ImGui_ImplVulkan_Shutdown();
#endif
}

} // namespace Yogi
