#pragma once

#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

// Engine-level bindless texture manager. Lives ABOVE the RHI: the RHI itself
// has no notion of "bindless"; it just exposes generic IShaderResourceBinding,
// which this class owns. Everything bindless-y is in here.
//
// Most renderer pipelines declare ResourceBinding = BindlessTextures::Get()
// .GetSRB() and index resources by slot through push constants or per-frame
// data structures (MaterialData, SceneFrame, ScenePush, ...). A small number
// of pipelines (currently DepthReduce.comp) handle private write targets and
// own their own SRB instead -- "bindless" here is purely a SAMPLED concept,
// matching D3D12 SM 6.6 ResourceDescriptorHeap semantics where the global
// heap is for SRV/CBV/Sampler reads and UAV writes typically use private
// descriptors.
//
// Layout of the managed SRB:
//   binding=0  sampler  u_sampler       (immutable linear/repeat)
//   binding=1  sampler  u_samplerMax    (immutable max-reduction, for Hi-Z)
//   binding=2  texture2D u_textures[MAX_TEXTURES]
//              SAMPLED_IMAGE, PARTIALLY_BOUND + UPDATE_AFTER_BIND +
//              VARIABLE_DESCRIPTOR_COUNT. Material textures + sampled views
//              of pyramid output that downstream passes consume.
//
// Slot 0 of u_textures[] is reserved for the engine's 1x1 white default
// texture. Materials without an AlbedoTexture inherit slot 0 via the schema's
// 0u default and sample opaque white -- BaseColor is then the only tint.
//
// Slot allocation reuses freed slots via a LIFO free-list so frequent
// register/unregister (Editor viewport resize, texture streaming, pyramid
// rebuild) doesn't drift the slot counter toward MAX_TEXTURES.
//
// Thread-safety: not thread-safe. Call from the main / render thread only.
class YG_API BindlessTextures
{
public:
    static void Initialize();
    static void Shutdown();
    static bool IsInitialized();

    static BindlessTextures& Get();

    // Allocate a sampled-image slot for `view`, write the descriptor, return
    // the slot. Re-registering the same view is idempotent.
    uint32_t Register(const ITextureView* view);
    void     Unregister(uint32_t slot);
    uint32_t Find(const ITextureView* view) const;

    // The SRB declared in PipelineDesc::ResourceBinding for every renderer
    // pipeline that consumes bindless textures. Owned by this manager;
    // borrowed pointers stay valid until Shutdown().
    IShaderResourceBinding* GetSRB() const { return m_srb.Get(); }

    // Default 1x1 white at slot 0. Most callers reach it implicitly via the
    // MaterialSchema's 0u default; exposed for explicit references.
    const ITextureView* GetDefaultWhiteView() const { return m_defaultWhiteView.Get(); }

    static constexpr uint32_t INVALID_SLOT = ~0u;

private:
    BindlessTextures();
    ~BindlessTextures();

    void EnsureDefaultWhite();

    Owner<IShaderResourceBinding> m_srb;

    uint32_t                                          m_nextSlot = 1; // 0 reserved for default white
    std::vector<uint32_t>                             m_freeList;
    std::unordered_map<const ITextureView*, uint32_t> m_viewToSlot;

    // Engine-wide 1x1 white default texture.
    Owner<ITexture>     m_defaultWhite;
    Owner<ITextureView> m_defaultWhiteView;
};

} // namespace Yogi
