#pragma once

#ifdef __cplusplus
#    include <cstdint>
namespace Yogi
{
#endif

struct OutlinePush
{
    uint64_t SceneFrameAddr; // for ZNear / ZFar
    float    Thickness;      // edge sampling radius in texels
    float    DepthThreshold; // min linearized-depth delta to count as an edge
    float    Color[3];       // outline color (RGB)
};

#ifdef __cplusplus
} // namespace Yogi
#endif
