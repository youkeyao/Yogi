#pragma once

namespace Yogi
{

enum class Filter : uint8_t
{
    Nearest = 0,
    Linear,
};

enum class MipmapMode : uint8_t
{
    Nearest = 0,
    Linear,
};

enum class SamplerAddressMode : uint8_t
{
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
};

enum class SamplerReductionMode : uint8_t
{
    None,
    Min,
    Max,
};

struct SamplerDesc
{
    Filter               MagFilter     = Filter::Linear;
    Filter               MinFilter     = Filter::Linear;
    MipmapMode           MipMode       = MipmapMode::Linear;
    SamplerAddressMode   AddressU      = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode   AddressV      = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode   AddressW      = SamplerAddressMode::ClampToEdge;
    SamplerReductionMode Reduction     = SamplerReductionMode::None;
    float                MaxAnisotropy = 1.0f;
    float                MinLod        = 0.0f;
    float                MaxLod        = 1000.0f;
};

class YG_API ISampler
{
public:
    virtual ~ISampler() = default;

    static Owner<ISampler> Create(const SamplerDesc& desc);
};

template <>
template <typename... Args>
inline Owner<ISampler> Owner<ISampler>::Create(Args&&... args)
{
    return ISampler::Create(std::forward<Args>(args)...);
}

} // namespace Yogi
