
struct Boid {
    position: vec4f,
    velocity: vec4f,
    destination: vec4f,
    counter: u32
}

struct RenderUniforms {
    projectionViewMatrix: mat4x4<f32>,
    projectionMatrix: mat4x4<f32>,
    viewMatrix: mat4x4<f32>,
    lightCount: i32,
    time: f32
}
struct ModelData {
    m: mat4x4<f32>
}

@group(0)
@binding(0)
var<uniform> camera: RenderUniforms;

@group(1)
@binding(0)
var<storage, read> boids: array<Boid>;

var<immediate> m: ModelData;

struct VertexIn {
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @builtin(instance_index) idx: u32
}

struct VertexOut {
    @builtin(position) pos: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
}

const worldUp: vec3f = vec3f(0, 1, 0);

@vertex
fn vs(i: VertexIn) -> VertexOut {
    let MVP = camera.projectionViewMatrix * m.m;
    var result: VertexOut;

    let forward = -normalize(boids[i.idx].velocity.xyz);
    let right = normalize(cross(worldUp, forward));
    let up = cross(forward, right);
    let boidRotation = mat3x3<f32>(right, up, forward);
    result.pos = MVP * vec4f((boidRotation * i.pos * 0.5) + boids[i.idx].position.xyz, 1.0);
    result.normal = (m.m * vec4f(boidRotation * i.normal, 1)).xyz;
    result.uv = i.uv;
    return result;
}

@fragment
fn fs(i: VertexOut) -> @location(0) vec4f {
    return vec4f(abs(normalize(i.normal)), 1);
}