#include "Renderer/BindlessTextures.h"
#include "Renderer/ShaderData.h" // MAX_TEXTURES

namespace Yogi
{

namespace
{
BindlessTextures* g_instance = nullptr;

constexpr int kBindingSampler         = 0;
constexpr int kBindingSamplerMax      = 1;
constexpr int kBindingSampledTextures = 2;
} // namespace

void BindlessTextures::Initialize()
{
    YG_CORE_ASSERT(g_instance == nullptr, "BindlessTextures::Initialize called twice");
    g_instance = new BindlessTextures();
}

void BindlessTextures::Shutdown()
{
    delete g_instance;
    g_instance = nullptr;
}

bool BindlessTextures::IsInitialized()
{
    return g_instance != nullptr;
}

BindlessTextures& BindlessTextures::Get()
{
    YG_CORE_ASSERT(g_instance, "BindlessTextures::Get called before Initialize");
    return *g_instance;
}

BindlessTextures::BindlessTextures()
{
    // Two immutable samplers (linear / max-reduction) followed by the
    // variable-count sampled-image array. Vulkan permits VARIABLE_DESCRIPTOR_
    // COUNT only on the highest-numbered binding, so the texture array MUST
    // be last. The RHI uniformly applies PARTIALLY_BOUND + UPDATE_AFTER_BIND
    // to non-sampler bindings, so unused slots in the bindless array can
    // stay unwritten and per-frame texture registration doesn't stall.
    const ShaderStage stages = ShaderStage::Vertex | ShaderStage::Fragment | ShaderStage::Compute | ShaderStage::Task |
        ShaderStage::Mesh | ShaderStage::Geometry;

    m_srb = IShaderResourceBinding::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ kBindingSampler, 1, ShaderResourceType::Sampler, stages },
            ShaderResourceAttribute{ kBindingSamplerMax, 1, ShaderResourceType::Sampler, stages },
            ShaderResourceAttribute{
                kBindingSampledTextures, MAX_TEXTURES, ShaderResourceType::SampledTexture, stages } },
        std::vector<ImmutableSamplerDesc>{ ImmutableSamplerDesc{ kBindingSampler, SamplerReductionMode::None },
                                           ImmutableSamplerDesc{ kBindingSamplerMax, SamplerReductionMode::Max } });

    EnsureDefaultWhite();
}

BindlessTextures::~BindlessTextures()
{
    m_defaultWhiteView = nullptr;
    m_defaultWhite     = nullptr;
    m_srb              = nullptr;
}

void BindlessTextures::EnsureDefaultWhite()
{
    if (m_defaultWhite)
        return;

    TextureDesc desc{};
    desc.Width         = 1;
    desc.Height        = 1;
    desc.MipLevels     = 1;
    desc.Format        = ITexture::Format::R8G8B8A8_UNORM;
    desc.Usage         = ITexture::Usage::Texture2D;
    desc.NumSamples    = SampleCountFlagBits::Count1;
    m_defaultWhite     = ITexture::Create(desc);
    m_defaultWhiteView = ITextureView::Create(WRef<ITexture>::Create(m_defaultWhite), TextureViewDesc{});

    const uint8_t whiteRGBA[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    m_defaultWhiteView->SetData(const_cast<uint8_t*>(whiteRGBA), sizeof(whiteRGBA));

    // Slot 0 reserved by convention; bind directly so the free-list never
    // hands out slot 0.
    m_srb->BindTextureView(m_defaultWhiteView.Get(), kBindingSampledTextures, 0);
    m_viewToSlot[m_defaultWhiteView.Get()] = 0u;
}

uint32_t BindlessTextures::Register(const ITextureView* view)
{
    YG_CORE_ASSERT(view, "BindlessTextures::Register called with null view");

    auto it = m_viewToSlot.find(view);
    if (it != m_viewToSlot.end())
        return it->second;

    uint32_t slot;
    if (!m_freeList.empty())
    {
        slot = m_freeList.back();
        m_freeList.pop_back();
    }
    else
    {
        if (m_nextSlot >= MAX_TEXTURES)
        {
            YG_CORE_WARN("BindlessTextures: pool exhausted ({0} slots), falling back to slot 0 "
                         "(default white). Raise MAX_TEXTURES in ShaderData.h.",
                         MAX_TEXTURES);
            return 0u;
        }
        slot = m_nextSlot++;
    }

    m_srb->BindTextureView(view, kBindingSampledTextures, static_cast<int>(slot));
    m_viewToSlot[view] = slot;
    return slot;
}

void BindlessTextures::Unregister(uint32_t slot)
{
    if (slot == INVALID_SLOT || slot == 0u)
        return; // slot 0 (default white) is never released

    for (auto it = m_viewToSlot.begin(); it != m_viewToSlot.end(); ++it)
    {
        if (it->second == slot)
        {
            m_viewToSlot.erase(it);
            break;
        }
    }
    m_freeList.push_back(slot);
}

uint32_t BindlessTextures::Find(const ITextureView* view) const
{
    auto it = m_viewToSlot.find(view);
    return it == m_viewToSlot.end() ? INVALID_SLOT : it->second;
}

} // namespace Yogi
