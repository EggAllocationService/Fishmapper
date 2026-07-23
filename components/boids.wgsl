struct Immediates {
    deltaTime: f32
}
struct Boid {
    position: vec3f,
    velocity: vec3f
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
    biasIncrement: f32
}
struct PointXYZRGB {
    position: vec4f
}

struct PointData {
    d1: u32,
    count: u32,
    d2: u32,
    d3: u32
}

@group(0) @binding(0)
var<uniform> params: Params;
@group(0) @binding(1)
var<storage, read_write> boids: array<Boid>;

@group(1) @binding(0)
var<storage, read> pointData: PointData;
@group(1) @binding(1)
var<storage, read> points: array<PointXYZRGB>;

@compute @workgroup_size(64)
fn update_boids(@builtin(global_invocation_id) pos: vec3u) {
    let index = pos.x;
    if (index > params.count) {
        return;
    }
    let boid = &boids[index];
    var avgPos = vec3f(0);
    var avgVel = vec3f(0);
    var neighbors: u32 = 0;
    var closeD = vec3f(0);

    for (var i: u32 = 0; i < params.count; i++) {
        if (i == index) {
            continue;
        }
        let delta = boid.position - boids[i].position;
        let absD = abs(delta);
        if (absD.x < params.visualRange && absD.y < params.visualRange && absD.z < params.visualRange) {
            let distance = length(delta);
            if (distance < params.protectedRange) {
                closeD += delta;
            } else if (distance < params.visualRange) {
                avgPos += boids[i].position;
                avgVel += boids[i].velocity;
                neighbors += 1;
            }
        }
    }

    if (neighbors > 0) {
        avgPos /= f32(neighbors);
        avgVel /= f32(neighbors);

        boid.velocity = (boid.velocity + (avgPos - boid.position)*params.centeringFactor + (avgVel - boid.velocity)*params.matchingFactor);
    }

    boid.velocity += closeD * params.avoidFactor;

    // avoid edges of box
    boid.velocity -= select(vec3f(0), vec3f(params.turnFactor), boid.position > vec3f(10));
    boid.velocity += select(vec3f(0), vec3f(params.turnFactor), boid.position < vec3f(-10));

    let speed = length(boid.velocity);
    if (speed < params.minSpeed) {
        boid.velocity = normalize(boid.velocity) * params.minSpeed;
    } else if (speed > params.maxSpeed) {
        boid.velocity = normalize(boid.velocity) * params.maxSpeed;
    }
}

var<immediate> im: Immediates;
@compute @workgroup_size(1)
fn apply_velocities(@builtin(global_invocation_id) idx: vec3u) {
    let i = idx.x;
    boids[i].position += boids[i].velocity * im.deltaTime;
}