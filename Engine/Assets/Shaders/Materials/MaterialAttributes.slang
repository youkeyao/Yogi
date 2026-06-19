[__AttributeUsage(_AttributeTargets.Var)]
struct RangeAttribute
{
    float minValue;
    float maxValue;
    float defaultValue;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct ColorAttribute
{
    float r;
    float g;
    float b;
    float a;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct HDRAttribute
{};

[__AttributeUsage(_AttributeTargets.Var)]
struct TextureAttribute
{};
