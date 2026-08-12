// available to post:
// color: texture_2d
// depth: texture_depth_2d
// samp: sampler
// linear_depth(pos: vec2f) -> depth at UV in world units
// global.time

struct FogParams {
    color: vec4f,
    start: f32,
    end: f32,
    density: f32
}

var<immediate> params: FogParams;

const E: f32 = 2.718281828459045;

@fragment
fn post(@location(0) uv: vec2f) -> @location(0) vec4f {
    let raw_depth = linear_depth(uv);

    let d = saturate((raw_depth - params.start) / (params.end - params.start));
    let c = textureSample(color, samp, uv);

    let fog = saturate(pow(E, -pow(params.density * d, 2)));

    return mix(c, params.color, 1-fog);
}

