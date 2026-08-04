//
// Created by Kyle Smith on 2026-07-22.
//

#ifndef FISHMAPPER_BOIDSCOMPONENT_H
#define FISHMAPPER_BOIDSCOMPONENT_H
#include "Vectors.h"
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

void registerBoids(glengine::pipeline::wgpu::WGPURenderer* renderer);

class BoidsComponent : public glengine::world::ActorSceneComponent {
public:
    BoidsComponent(unsigned int count, std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud);
    ~BoidsComponent() override;

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    glengine::pipeline::wgpu::GPUPointer<SimParams> Params;
private:
    static constexpr unsigned int VoxelResolution = 32;
    static constexpr unsigned int VoxelCount = VoxelResolution * VoxelResolution * VoxelResolution;

    void initPositionsAndVelocities();
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> renderPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> updatePipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> applyPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> clearVoxelsPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> voxelizePointsPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> eatPointsPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> blurDensityXPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> blurDensityYPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> blurDensityZPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> generateVectorFieldPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;

    glengine::pipeline::wgpu::WrappedBuffer boidsBuffer;
    glengine::pipeline::wgpu::WrappedBuffer pointDensityBuffer;
    glengine::pipeline::wgpu::WrappedBuffer eatenPointBuffer;
    glengine::pipeline::wgpu::WrappedBuffer blurredDensityXBuffer;
    glengine::pipeline::wgpu::WrappedBuffer blurredDensityYBuffer;
    glengine::pipeline::wgpu::WrappedBuffer blurredDensityZBuffer;
    std::shared_ptr<glengine::pipeline::wgpu::GPUTexture> foodFieldTexture;
    WGPUSampler foodFieldSampler = nullptr;
    glengine::pipeline::wgpu::GPUPointer<VoxelParams> voxelParams;

    unsigned int boidsCount;
    unsigned int pointCapacity;
};


#endif //FISHMAPPER_BOIDSCOMPONENT_H
