struct Immediates {
    deltaTime: f32
}
struct Boid {
    position: vec3f,
    velocity: vec3f,
    goal: vec4f
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
    targetFactor: f32,
    jitterRadius: f32,
    frame: u32,
    wanderFactor: f32,
    pad2: f32
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
var<storage, read> pointParams: PointData;
@group(1) @binding(1)
var<storage, read> points: array<PointXYZRGB>;

fn pcg(n: u32) -> u32 {
    var h = n * 747796405u + 2891336453u;
    h = ((h >> ((h >> 28u) + 4u)) ^ h) * 277803737u;
    return (h >> 22u) ^ h;
}
fn rand01(n: u32) -> f32 {
    return f32(pcg(n)) * (1.0 / 4294967296.0);
}

const INIT_SAMPLES: u32 = 48u;
const TRACK_SAMPLES: u32 = 12u;
const TRACK_RADIUS: f32 = 2.0;
const REJOIN_RATE: f32 = 0.02;
fn convertPoint(p: vec3f) -> vec3f {
    return (p * 10) - vec3f(0, 0, 10);
}
@compute @workgroup_size(64)
fn update_boids(@builtin(global_invocation_id) pos: vec3u) {
    let index = pos.x;
    if (index >= params.count) {
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

    if (pointParams.count > 0u) {
        let seed = params.frame * 747796405u + index * 2891336453u;
        if (boid.goal.w < 0.5) {
            // Unassigned: anchor to the nearest of a random sample of points.
            // Sampling distributes targets proportional to point density.
            var bestPos = vec3f(0.0);
            var bestDist = 1e30;
            for (var s = 0u; s < INIT_SAMPLES; s++) {
                let pi = pcg(seed + s * 97u) % pointParams.count;
                let p = convertPoint(points[pi].position.xyz);
                let d = distance(p, boid.position);
                if (d < bestDist) {
                    bestDist = d;
                    bestPos = p;
                }
            }
            boid.goal = vec4f(bestPos, 1.0);
        } else {
            // Track: adopt a uniformly random sampled point close to the current
            // target (reservoir sampling). Random choice makes goals diffuse over
            // the local cluster instead of collapsing onto the nearest point.
            var snapPos = boid.goal.xyz;
            var candidates = 0u;
            var nearPos = vec3f(0.0);
            var nearDist = 1e30;
            for (var s = 0u; s < TRACK_SAMPLES; s++) {
                let pi = pcg(seed + s * 57u + 1u) % pointParams.count;
                let p = convertPoint(points[pi].position.xyz);
                let d = distance(p, boid.goal.xyz);
                if (d < nearDist) {
                    nearDist = d;
                    nearPos = p;
                }
                if (d < TRACK_RADIUS) {
                    candidates++;
                    if (pcg(seed + s * 31u) % candidates == 0u) {
                        snapPos = p;
                    }
                }
            }
            if (candidates > 0u) {
                boid.goal = vec4f(snapPos, 1.0);
            } else {
                // Local cluster is gone: drift slowly back toward the cloud.
                boid.goal = vec4f(boid.goal.xyz + (nearPos - boid.goal.xyz) * REJOIN_RATE, 1.0);
            }
        }

        // Deterministic per-boid jitter spreads boids sharing nearby targets.
        let jseed = index * 3u;
        let jitter = (vec3f(rand01(jseed), rand01(jseed + 1u), rand01(jseed + 2u)) * 2.0 - vec3f(1.0)) * params.jitterRadius;

        // Weaken the pull and add wander when crowded, so dense regions spread out.
        let crowd = f32(neighbors);
        let pull = params.targetFactor / (1.0 + crowd * 0.25);
        boid.velocity += (boid.goal.xyz + jitter - boid.position) * pull;

        let wander = vec3f(rand01(seed + 101u), rand01(seed + 102u), rand01(seed + 103u)) * 2.0 - vec3f(1.0);
        boid.velocity += wander * params.wanderFactor * (1.0 + crowd * 0.5);
    }

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