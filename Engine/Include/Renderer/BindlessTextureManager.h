#pragma once

#include "Core/Singleton.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

class YG_API BindlessTextureManager : public Singleton<BindlessTextureManager>
{
    friend class Singleton<BindlessTextureManager>;

public:
    static uint32_t Register(const ITextureView* view);
    static void     Unregister(uint32_t slot);
    static uint32_t Find(const ITextureView* view);

    static IShaderResourceBinding* GetSRB() { return Get().m_srb.Get(); }
    static const ITextureView*     GetDefaultWhiteView() { return Get().m_defaultWhiteView.Get(); }

    static constexpr uint32_t INVALID_SLOT = ~0u;

private:
    BindlessTextureManager();
    ~BindlessTextureManager();

    void EnsureDefaultWhite();

    static BindlessTextureManager* s_instance; // defined in .cpp, exported via YG_API

    Owner<IShaderResourceBinding> m_srb;

    uint32_t                                          m_nextSlot = 1; // 0 reserved for default white
    std::vector<uint32_t>                             m_freeList;
    std::unordered_map<const ITextureView*, uint32_t> m_viewToSlot;

    Owner<ITexture>     m_defaultWhite;
    Owner<ITextureView> m_defaultWhiteView;
};

} // namespace Yogi
