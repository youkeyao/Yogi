TestComponent = {
    value = "string",
    tmp = "float",
    tmp2 = "bool"
}

function TestSystem_on_update(ts, scene)
    scene:view_components({"TestComponent"}, function(e)
        print(e:get_component("TestComponent").value)
    end)
    -- scene:view_components(function(e)
    --     print(e:get_id())
    -- end)
end

function TestSystem_on_event(e, scene)
end

register_component("TestComponent", TestComponent)
register_system("TestSystem", TestSystem_on_update, TestSystem_on_event)