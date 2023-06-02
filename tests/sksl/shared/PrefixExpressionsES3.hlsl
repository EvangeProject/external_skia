cbuffer _UniformBuffer : register(b0, space0)
{
    float4 _10_colorGreen : packoffset(c0);
    float4 _10_colorRed : packoffset(c1);
};


static float4 sk_FragColor;

struct SPIRV_Cross_Output
{
    float4 sk_FragColor : SV_Target0;
};

float4 main(float2 _24)
{
    bool ok = true;
    uint _38 = uint(_10_colorGreen.x);
    uint val = _38;
    uint2 _43 = uint2(_38, ~_38);
    uint2 mask = _43;
    uint2 _47 = ~_43;
    int2 _52 = int2(int(_47.x), int(_47.y));
    int2 imask = _52;
    int2 _54 = ~_52;
    uint2 _60 = (~_43) & uint2(uint(_54.x), uint(_54.y));
    mask = _60;
    bool _69 = false;
    if (true)
    {
        _69 = all(bool2(_60.x == uint2(0u, 0u).x, _60.y == uint2(0u, 0u).y));
    }
    else
    {
        _69 = false;
    }
    ok = _69;
    bool4 _71 = _69.xxxx;
    return float4(_71.x ? _10_colorGreen.x : _10_colorRed.x, _71.y ? _10_colorGreen.y : _10_colorRed.y, _71.z ? _10_colorGreen.z : _10_colorRed.z, _71.w ? _10_colorGreen.w : _10_colorRed.w);
}

void frag_main()
{
    float2 _20 = 0.0f.xx;
    sk_FragColor = main(_20);
}

SPIRV_Cross_Output main()
{
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.sk_FragColor = sk_FragColor;
    return stage_output;
}
