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

    QueueFamilyIndices indices           = FindQueueFamilies(context->GetVkPhysicalDevice(), context->GetVkSurface());
    initInfo.Instance                    = context->GetVkInstance();
    initInfo.PhysicalDevice              = context->GetVkPhysicalDevice();
    initInfo.Device                      = context->GetVkDevice();
    initInfo.QueueFamily                 = indices.graphicsFamily.value();
    initInfo.Queue                       = context->GetGraphicsQueue();
    initInfo.PipelineCache               = VK_NULL_HANDLE;
    initInfo.DescriptorPool              = context->GetVkDescriptorPool();
    initInfo.MinImageCount               = swapChain->GetImageCount();
    initInfo.ImageCount                  = swapChain->GetImageCount();
    initInfo.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator                   = nullptr;
    initInfo.CheckVkResultFn             = nullptr;
    initInfo.UseDynamicRendering         = true;
    VkFormat swapchainFormat             = YgTextureFormat2VkFormat(swapChain->GetColorFormat());
    initInfo.PipelineRenderingCreateInfo = {};
    initInfo.PipelineRenderingCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;
    initInfo.PipelineRenderingCreateInfo.depthAttachmentFormat   = VK_FORMAT_UNDEFINED;
    initInfo.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    ImGui_ImplVulkan_LoadFunctions(0, &VkLoadFunction, initInfo.Instance);
    ImGui_ImplVulkan_Init(&initInfo);

    m_commandBuffer = ResourceManager::CreateResource<ICommandBuffer>(
        CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
#endif
}

void ImGuiEndLayer::RendererDraw()
{
    ImDrawData* mainDrawData = ImGui::GetDrawData();
#ifdef YG_RENDERER_VULKAN
    auto               swapChain     = Application::GetInstance().GetSwapChain();
    WRef<ITextureView> currentTarget = swapChain->AcquireCurrentTarget();
    const ITexture*    targetTex     = currentTarget->GetTexture();

    m_commandBuffer->Begin();

    m_commandBuffer->Barrier(BarrierDesc{
        .TextureView = currentTarget.Get(),
        .BeforeState = ResourceState::ColorAttachment,
        .AfterState  = ResourceState::ColorAttachment,
    });
    {
        RenderingDesc rdesc{};
        rdesc.Width   = targetTex->GetWidth();
        rdesc.Height  = targetTex->GetHeight();
        rdesc.Samples = SampleCountFlagBits::Count1;
        RenderingAttachment color{};
        color.View        = currentTarget.Get();
        color.LoadAction  = LoadOp::Load;
        color.StoreAction = StoreOp::Store;
        rdesc.ColorAttachments.push_back(color);
        m_commandBuffer->BeginRendering(rdesc);
    }
    ImGui_ImplVulkan_RenderDrawData(mainDrawData,
                                    static_cast<VulkanCommandBuffer*>(m_commandBuffer.Get())->GetVkCommandBuffer());
    m_commandBuffer->EndRendering();

    m_commandBuffer->Barrier(BarrierDesc{
        .TextureView = currentTarget.Get(),
        .BeforeState = ResourceState::ColorAttachment,
        .AfterState  = ResourceState::Present,
    });

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
