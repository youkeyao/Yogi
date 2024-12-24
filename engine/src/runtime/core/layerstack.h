#pragma once

#include "runtime/core/layer.h"

namespace Yogi {

class LayerStack
{
public:
    LayerStack();
    ~LayerStack();

    void push_layer(Layer *layer);
    void push_overlay(Layer *layer);
    void pop_layer(Layer *layer);
    void pop_overlay(Layer *layer);

    std::vector<Layer *>::iterator begin() { return m_layers.begin(); }
    std::vector<Layer *>::iterator end() { return m_layers.end(); }

private:
    std::vector<Layer *> m_layers;
    uint32_t             m_layer_insert_index = 0;
};

}  // namespace Yogi