//
// Created by Kyle Smith on 2026-07-22.
//

#ifndef FISHMAPPER_BOIDSCOMPONENT_H
#define FISHMAPPER_BOIDSCOMPONENT_H

#include <cstdint>
#include <functional>
#include <vector>

#include "Vectors.h"
#include "Matrix.h"
#include "3d/ActorSceneComponent.h"
#include "pipeline/wgpu/WGPURenderer.h"
#include "pipeline/wgpu/pipeline/ComputePipeline.h"
#include "pipeline/wgpu/pipeline/RenderPipeline.h"
#include "pointmapper/pipeline/nodes/Node.h"
#include "pointmapper/pipeline/types/CommonTypes.h"

struct alignas(16) Boid {
    float4 position;
    float4 velocity;
    float4 destination;
    uint32_t counter;
};

struct SimParams {
    unsigned int count;
    float turnFactor;
    float visualRange;
    float protectedRange;
    float centeringFactor;
    float avoidFactor;
    float matchingFactor;
    float maxSpeed;
    float minSpeed;
    float maxBias;
    float biasIncrement;
    float foodSeekingFactor;
};

struct VoxelParams {
    float bounds;
    uint32_t resolution;
    uint32_t foodPerBoid;
    uint32_t padding;
};

struct BoidImmediates {
    float deltaTime;
};

struct VoxelizeImmediates {
    mat4 transform;
};

struct CloudInput {
    std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud;
    std::function<mat4()> getTransform;
};

void registerBoids(glengine::pipeline::wgpu::WGPURenderer* renderer);

class BoidsComponent : public glengine::world::ActorSceneComponent {
public:
    BoidsComponent(unsigned int count, const std::vector<CloudInput>& clouds);
    ~BoidsComponent() override;

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    glengine::pipeline::wgpu::GPUPointer<SimParams> Params;
private:
    static constexpr unsigned int VoxelResolution = 32;
    static constexpr unsigned int VoxelCount = VoxelResolution * VoxelResolution * VoxelResolution;

    struct CloudData {
        std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud;
        std::function<mat4()> getTransform;
        std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> voxelizePipeline;
        unsigned int maximumPointCount;
    };

    void initPositionsAndVelocities();
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> renderPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> updatePipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> applyPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> clearVoxelsPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> eatPointsPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> generateVectorFieldPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;

    glengine::pipeline::wgpu::WrappedBuffer boidsBuffer;
    glengine::pipeline::wgpu::WrappedBuffer pointDensityBuffer;
    glengine::pipeline::wgpu::WrappedBuffer eatenPointBuffer;
    glengine::pipeline::wgpu::WrappedBuffer dummyPointBuffer;
    glengine::pipeline::wgpu::WrappedBuffer dummyPointCountBuffer;
    std::shared_ptr<glengine::pipeline::wgpu::GPUTexture> foodFieldTexture;
    WGPUSampler foodFieldSampler = nullptr;
    glengine::pipeline::wgpu::GPUPointer<VoxelParams> voxelParams;

    std::vector<CloudData> clouds;
    unsigned int boidsCount;
};

#endif //FISHMAPPER_BOIDSCOMPONENT_H
