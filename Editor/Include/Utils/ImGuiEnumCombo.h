#pragma once

#include "Utils/magic_enum.hpp"

#include <imgui.h>

namespace Yogi
{

template <typename E>
bool ImGuiEnumCombo(const char* label, E& current)
{
    static_assert(std::is_enum_v<E>, "ImGuiEnumCombo requires enum type");

    constexpr auto values = magic_enum::enum_values<E>();
    int            index  = magic_enum::enum_index(current).value_or(0);

    std::vector<const char*> items;
    items.reserve(values.size());
    for (auto v : values)
        items.push_back(magic_enum::enum_name(v).data());

    bool changed = ImGui::Combo(label, &index, items.data(), (int)items.size());
    if (changed)
    {
        current = values[index];
    }
    return changed;
}

} // namespace Yogi
