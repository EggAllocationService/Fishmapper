struct Immediates {
    deltaTime: f32
}
struct Boid {
    position: vec4f,
    velocity: vec4f,
    destination: vec4f,
    counter: u32
}

struct Params {
    count: u32,
    turnFactor: f32,
    visualRange: f32,
    protectedRange: f32,
    centeringFactor: f32,
    avoidFactor: f32,
    matchingFactor: f32,
    maxSpeed: f32,
    minSpeed: f32,
    maxBias: f32,
    biasIncrement: f32,
    foodSeekingFactor: f32
}

struct VoxelParams {
    bounds: f32,
    resolution: u32,
    foodPerBoid: u32,
    padding: u32
}

@group(0) @binding(0)
var<uniform> params: Params;
@group(0) @binding(1)
var<storage, read_write> boids: array<Boid>;

@group(1) @binding(0)
var foodField: texture_3d<f32>;
@group(1) @binding(1)
var foodFieldSampler: sampler;
@group(1) @binding(2)
var<uniform> voxelParams: VoxelParams;

fn food_vector(position: vec3f) -> vec3f {
    let normalizedPosition = (position + vec3f(voxelParams.bounds)) / (2.0 * voxelParams.bounds);
    if (any(normalizedPosition < vec3f(0.0)) || any(normalizedPosition >= vec3f(1.0))) {
        return vec3f(0.0);
    }

    let field = textureSampleLevel(foodField, foodFieldSampler, normalizedPosition, 0.0);
    return field.xyz * field.w;
}

@compute @workgroup_size(64)
fn update_boids(@builtin(global_invocation_id) pos: vec3u) {
    let index = pos.x;
    if (index >= params.count) {
        return;
    }

    let boid = &boids[index];
    let boidPosition = boid.position.xyz;
    var velocity = boid.velocity.xyz;
    var avgPos = vec3f(0.0);
    var avgVel = vec3f(0.0);
    var neighbors: u32 = 0;
    var closeD = vec3f(0.0);

    for (var i: u32 = 0; i < params.count; i++) {
        if (i == index) {
            continue;
        }
        let delta = boidPosition - boids[i].position.xyz;
        let absD = abs(delta);
        if (absD.x < params.visualRange && absD.y < params.visualRange && absD.z < params.visualRange) {
            let distance = length(delta);
            if (distance < params.protectedRange) {
                closeD += delta;
            } else if (distance < params.visualRange) {
                avgPos += boids[i].position.xyz;
                avgVel += boids[i].velocity.xyz;
                neighbors += 1;
            }
        }
    }

    if (neighbors > 0) {
        avgPos /= f32(neighbors);
        avgVel /= f32(neighbors);
        velocity += (avgPos - boidPosition) * params.centeringFactor + (avgVel - velocity) * params.matchingFactor;
    }

    velocity += food_vector(boidPosition) * params.foodSeekingFactor;
    velocity += closeD * params.avoidFactor;

    velocity -= select(vec3f(0.0), vec3f(params.turnFactor), boidPosition > vec3f(10.0));
    velocity += select(vec3f(0.0), vec3f(params.turnFactor), boidPosition < vec3f(-10.0));

    let speed = length(velocity);
    if (speed < params.minSpeed) {
        velocity = normalize(velocity) * params.minSpeed;
    } else if (speed > params.maxSpeed) {
        velocity = normalize(velocity) * params.maxSpeed;
    }

    boid.velocity = vec4f(velocity, 0.0);
}

var<immediate> im: Immediates;
@compute @workgroup_size(1)
fn apply_velocities(@builtin(global_invocation_id) idx: vec3u) {
    let i = idx.x;
    if (i >= params.count) {
        return;
    }

    boids[i].position = vec4f(boids[i].position.xyz + boids[i].velocity.xyz * im.deltaTime, 0.0);
}
