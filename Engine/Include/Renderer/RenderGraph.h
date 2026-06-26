#pragma once

#include "Core/Type.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITextureView.h"
#include "Renderer/ShaderData.h"

namespace Yogi
{

class YG_API RenderGraphBuilder
{
public:
    void UseBuffer(const IBuffer* buffer, ResourceState state);
    void UseTexture(const ITextureView* view, ResourceState state);

private:
    friend class RenderGraph;

    struct TextureUse
    {
        const ITextureView* View  = nullptr;
        ResourceState       State = ResourceState::None;
    };

    struct BufferUse
    {
        const IBuffer* Buffer = nullptr;
        ResourceState  State  = ResourceState::None;
    };

    std::vector<BufferUse>  m_bufferUses;
    std::vector<TextureUse> m_textureUses;
};

class YG_API RenderGraphContext
{
public:
    struct TextureInitialState
    {
        const ITextureView* View  = nullptr;
        ResourceState       State = ResourceState::Undefined;
    };

    void SetInitialTextureState(const ITextureView* view, ResourceState state);

    ICommandBuffer*                  CommandBuffer       = nullptr;
    uint64_t                         SceneFrameAddr      = 0;
    ITextureView*                    CurrentTarget       = nullptr;
    ITextureView*                    DepthView           = nullptr;
    uint32_t                         TargetWidth         = 0;
    uint32_t                         TargetHeight        = 0;
    const std::vector<DrawBucket>*   Buckets             = nullptr;
    uint32_t                         TotalDrawCount      = 0;
    bool                             TransitionToPresent = false;
    std::vector<TextureInitialState> TextureInitialStates;
};

class YG_API RenderGraph
{
public:
    RenderGraph()  = default;
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = default;
    RenderGraph& operator=(RenderGraph&&)      = default;

    template <typename T, typename... Args>
    T& RegisterPass(Args&&... args)
    {
        static_assert(std::is_base_of_v<RenderPass, T>, "RenderGraph::RegisterPass<T>: T must derive from RenderPass");
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        T&   ref  = *pass;
        m_ownedPasses.push_back(std::move(pass));
        constexpr std::string_view label = GetTypeName<T>();
        m_passes.push_back(PassEntry{ &ref, std::string(label) });
        ref.Init(*this);
        return ref;
    }

    template <typename T>
    T* GetPass() const
    {
        for (const PassEntry& entry : m_passes)
        {
            if (auto* pass = dynamic_cast<T*>(entry.Pass))
                return pass;
        }
        return nullptr;
    }

    void Clear();

    void Execute(ICommandBuffer* commandBuffer, RenderGraphContext context);

    void                 RegisterBuffer(const std::string& name, WRef<IBuffer> buffer);
    WRef<IBuffer>        GetBuffer(const std::string& name) const;
    Owner<ITextureView>& RegisterTexture(const std::string& name);
    const ITextureView*  GetTexture(const std::string& name) const;

private:
    void ResetStateTracking();
    void SeedTextureStates(const RenderGraphContext& context);
    void RequireBufferState(ICommandBuffer* commandBuffer,
                            const IBuffer*  buffer,
                            ResourceState   after,
                            ResourceState   initial = ResourceState::None);
    void RequireTextureState(ICommandBuffer* commandBuffer, const ITextureView* view, ResourceState after);

    struct PassEntry
    {
        RenderPass* Pass = nullptr;
        std::string Label;
    };

    std::vector<std::unique_ptr<RenderPass>>               m_ownedPasses;
    std::vector<PassEntry>                                 m_passes;
    std::vector<RenderGraphBuilder>                        m_frameBuilders;
    std::unordered_map<std::string, WRef<IBuffer>>         m_buffers;
    std::unordered_map<std::string, Owner<ITextureView>>   m_textures;
    std::unordered_map<const IBuffer*, ResourceState>      m_bufferStates;
    std::unordered_map<const ITextureView*, ResourceState> m_textureStates;
    std::unordered_map<const ITextureView*, ResourceState> m_textureBeginStates;
};

} // namespace Yogi
