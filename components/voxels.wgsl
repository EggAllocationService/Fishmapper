struct VoxelParams {
    bounds: f32,
    resolution: u32,
    foodPerBoid: u32,
    padding: u32
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

struct Boid {
    position: vec4f,
    velocity: vec4f,
    destination: vec4f,
    counter: u32
}

@group(0) @binding(0)
var<storage, read> points: array<PointXYZRGB>;
@group(0) @binding(1)
var<storage, read> pointParams: PointData;

@group(1) @binding(0)
var<storage, read_write> pointDensityGrid: array<atomic<u32>>;
@group(1) @binding(1)
var<storage, read_write> eatenPointGrid: array<atomic<u32>>;
@group(1) @binding(2)
var<uniform> densityParams: VoxelParams;

@group(2) @binding(0)
var<storage, read> boids: array<Boid>;

@group(3) @binding(0)
var gradientTexture: texture_storage_3d<rgba16float, write>;

fn is_inside_bounds(position: vec3f) -> bool {
    let normalized = (position + vec3f(densityParams.bounds)) / (2.0 * densityParams.bounds);
    return all(normalized >= vec3f(0.0)) && all(normalized < vec3f(1.0));
}

fn point_to_voxel(position: vec3f) -> vec3u {
    let normalized = (position + vec3f(densityParams.bounds)) / (2.0 * densityParams.bounds);
    return vec3u(floor(normalized * f32(densityParams.resolution)));
}

fn voxel_to_index(voxel: vec3u) -> u32 {
    return voxel.x * densityParams.resolution * densityParams.resolution + voxel.y * densityParams.resolution + voxel.z;
}

fn index_to_voxel(index: u32) -> vec3u {
    let resolutionSquared = densityParams.resolution * densityParams.resolution;
    return vec3u(index / resolutionSquared, (index / densityParams.resolution) % densityParams.resolution, index % densityParams.resolution);
}

fn remaining_points(index: u32) -> u32 {
    let density = atomicLoad(&pointDensityGrid[index]);
    let eaten = atomicLoad(&eatenPointGrid[index]);
    return density - min(density, eaten);
}

fn remaining_points_at(voxel: vec3i) -> f32 {
    let maximumCoordinate = i32(densityParams.resolution) - 1;
    let clampedVoxel = vec3u(clamp(voxel, vec3i(0), vec3i(maximumCoordinate)));
    return f32(remaining_points(voxel_to_index(clampedVoxel)));
}

fn blurred_density(voxel: vec3i) -> f32 {
    var totalDensity = 0.0;
    for (var x = -1; x <= 1; x++) {
        for (var y = -1; y <= 1; y++) {
            for (var z = -1; z <= 1; z++) {
                totalDensity += remaining_points_at(voxel + vec3i(x, y, z));
            }
        }
    }
    return totalDensity / 27.0;
}

fn convert_point(position: vec3f) -> vec3f {
    return (position * 8.0) - vec3f(0.0, 0.0, 13.0);
}

@compute @workgroup_size(64)
fn clear_voxels(@builtin(global_invocation_id) invocation: vec3u) {
    let index = invocation.x;
    let voxelCount = densityParams.resolution * densityParams.resolution * densityParams.resolution;
    if (index >= voxelCount) {
        return;
    }

    atomicStore(&pointDensityGrid[index], 0u);
}

@compute @workgroup_size(64)
fn voxelize_points(@builtin(global_invocation_id) invocation: vec3u) {
    let index = invocation.x;
    if (index >= pointParams.count) {
        return;
    }

    let position = convert_point(points[index].position.xyz);
    if (!is_inside_bounds(position)) {
        return;
    }

    atomicAdd(&pointDensityGrid[voxel_to_index(point_to_voxel(position))], 1u);
}

@compute @workgroup_size(64)
fn eat_points(@builtin(global_invocation_id) invocation: vec3u) {
    let index = invocation.x;
    if (index >= arrayLength(&boids) || !is_inside_bounds(boids[index].position.xyz)) {
        return;
    }

    let voxelIndex = voxel_to_index(point_to_voxel(boids[index].position.xyz));
    let density = atomicLoad(&pointDensityGrid[voxelIndex]);
    loop {
        let eaten = atomicLoad(&eatenPointGrid[voxelIndex]);
        if (eaten >= density) {
            return;
        }

        let nextEaten = min(density, eaten + densityParams.foodPerBoid);
        let result = atomicCompareExchangeWeak(&eatenPointGrid[voxelIndex], eaten, nextEaten);
        if (result.exchanged) {
            return;
        }
    }
}

@compute @workgroup_size(4, 4, 4)
fn generate_vector_field(@builtin(global_invocation_id) invocation: vec3u) {
    if (any(invocation >= vec3u(densityParams.resolution))) {
        return;
    }

    let voxel = vec3i(invocation);
    let gradient = vec3f(
        blurred_density(voxel + vec3i(1, 0, 0)) - blurred_density(voxel - vec3i(1, 0, 0)),
        blurred_density(voxel + vec3i(0, 1, 0)) - blurred_density(voxel - vec3i(0, 1, 0)),
        blurred_density(voxel + vec3i(0, 0, 1)) - blurred_density(voxel - vec3i(0, 0, 1))
    );
    let gradientMagnitude = length(gradient);
    let direction = select(vec3f(0.0), gradient / gradientMagnitude, gradientMagnitude > 0.0);
    let localFood = blurred_density(voxel);
    let strength = gradientMagnitude / (gradientMagnitude + localFood + 1.0);
    textureStore(gradientTexture, voxel, vec4f(direction, strength));
}
