#pragma once

#include "Renderer/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class StagingArena;

class YG_API ObjectCullPass : public RenderPass
{
public:
    static constexpr uint64_t MAX_MESH_DRAWS                 = 1000000ull;
    static constexpr uint64_t MAX_MESHLET_VIS_COUNT          = 32ull * 1024ull * 1024ull;
    static constexpr uint64_t INDIRECT_COMMAND_BUFFER_SIZE   = MAX_MESH_DRAWS * sizeof(uint32_t) * 3;
    static constexpr uint64_t VISIBLE_DRAW_INDEX_BUFFER_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t);
    static constexpr uint64_t INDIRECT_COUNT_BUFFER_SIZE     = 256ull;
    static constexpr uint64_t OBJECT_VIS_BITFIELD_SIZE       = ((MAX_MESH_DRAWS + 31ull) / 32ull) * sizeof(uint32_t);
    static constexpr uint64_t MESHLET_VIS_BITFIELD_SIZE = ((MAX_MESHLET_VIS_COUNT + 31ull) / 32ull) * sizeof(uint32_t);

    ObjectCullPass()           = default;
    ~ObjectCullPass() override = default;

    void Initialize() override;
    void Shutdown() override;

    void BeginFrame() override;

    void FillSceneFrame(SceneFrame& sceneFrame) override;

    void PrepareEarly(ICommandBuffer* cmd);
    void ExecuteEarly(ICommandBuffer* cmd, uint64_t sceneFrameAddr, uint32_t drawBase, uint32_t drawCount);
    void PrepareLate(ICommandBuffer* cmd);
    void ExecuteLate(ICommandBuffer* cmd,
                     uint64_t        sceneFrameAddr,
                     uint32_t        drawBase,
                     uint32_t        drawCount,
                     uint32_t        pyramidSlot);

    WRef<IBuffer> AcquireIndirectCommandBuffer() const { return m_indirectCommandBuffer; }
    WRef<IBuffer> AcquireIndirectCountBuffer() const { return m_indirectCountBuffer; }
    WRef<IBuffer> AcquireVisibleDrawIndexBuffer() const { return m_visibleDrawIndexBuffer; }

private:
    static const char* k_objectCullShaderEarly;
    static const char* k_objectCullShaderLate;

    WRef<IPipeline> m_cullPipelineEarly = nullptr;
    WRef<IPipeline> m_cullPipelineLate  = nullptr;

    WRef<IBuffer> m_indirectCommandBuffer  = nullptr;
    WRef<IBuffer> m_indirectCountBuffer    = nullptr;
    WRef<IBuffer> m_visibleDrawIndexBuffer = nullptr;

    WRef<IBuffer> m_objectVis[2]  = { nullptr, nullptr };
    WRef<IBuffer> m_meshletVis[2] = { nullptr, nullptr };
    uint32_t      m_visReadIdx    = 0;
};

} // namespace Yogi
