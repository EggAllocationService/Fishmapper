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

struct Boid {
    float4 position;
    float4 velocity;
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
};

struct BoidImmediates {
    float deltaTime;
};

void registerBoids(glengine::pipeline::wgpu::WGPURenderer* renderer);

class BoidsComponent : public glengine::world::ActorSceneComponent {
public:
    BoidsComponent(unsigned int count);

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
