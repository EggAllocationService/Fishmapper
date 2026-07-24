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
    float4 goal;
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
    float targetFactor;
    float jitterRadius;
    unsigned int frame;
    float pad1;
    float pad2;
};

struct BoidImmediates {
    float deltaTime;
};

void registerBoids(glengine::pipeline::wgpu::WGPURenderer* renderer);

class BoidsComponent : public glengine::world::ActorSceneComponent {
public:
    BoidsComponent(unsigned int count, std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud);

    void Update(double deltaTime) override;
    void Render(const glengine::pipeline::wgpu::RenderBundle &, glengine::MatrixStack &stack) override;

    glengine::pipeline::wgpu::GPUPointer<SimParams> Params;
private:
    void initPositionsAndVelocities();
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> renderPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> updatePipeline;
    std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline> applyPipeline;
    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;

    glengine::pipeline::wgpu::WrappedBuffer boidsBuffer;

    unsigned int boidsCount;
};


#endif //FISHMAPPER_BOIDSCOMPONENT_H
