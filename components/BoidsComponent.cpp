//
// Created by Kyle Smith on 2026-07-22.
//

#include "BoidsComponent.h"

#include "Engine.h"
#include "BoidShaders.h"
#include "GLMath.h"
#include "3d/mesh/StaticMesh.h"

void registerBoids(glengine::pipeline::wgpu::WGPURenderer *renderer) {
    auto kernels = renderer->CompileShader(embed_boids_wgsl);
    WGPUBindGroupLayoutEntry entries[2] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    entries[0].visibility = WGPUShaderStage_Compute;
    entries[1].visibility = WGPUShaderStage_Compute;

    entries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(SimParams)
    };
    entries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Storage,
        .hasDynamicOffset = false,
        .minBindingSize = 0
    };
    entries[1].binding = 1;
    WGPUBindGroupLayoutDescriptor desc = {
        .nextInChain = nullptr,
        .label = {},
        .entryCount = 2,
        .entries = &entries[0]
    };
    renderer->BuildComputePipeline("BoidsUpdate", kernels, "update_boids", {&desc, 1}, 0);
    renderer->BuildComputePipeline("BoidsApply", kernels, "apply_velocities", {&desc, 1}, sizeof(BoidImmediates));

    desc.entryCount = 1;
    desc.entries = &entries[1];
    entries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[1].visibility = WGPUShaderStage_Vertex;
    entries[1].binding = 0;

    auto shaders = renderer->CompileShader(embed_boidshaders_wgsl);
    renderer->BuildRenderPipeline("Boids", shaders, nullptr, {&desc, 1}, sizeof(mat4), nullptr);

}

BoidsComponent::BoidsComponent(unsigned int count) {
    auto renderer = GetEngine()->GetRenderer();
    boidsBuffer = renderer->CreateRawBuffer("boids data", WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, sizeof(Boid) * count);
    boidsCount = count;

    updatePipeline = renderer->GetComputePipelineByName("BoidsUpdate")->CreateInstance();
    applyPipeline = renderer->GetComputePipelineByName("BoidsApply")->CreateInstance();
    Params = renderer->AllocateObject<SimParams>(WGPUBufferUsage_Uniform);

    *Params = {
        .count = boidsCount,
        .turnFactor = 0.2,
        .visualRange = 1,
        .protectedRange = 0.5,
        .centeringFactor = 0.005,
        .avoidFactor = 0.2,
        .matchingFactor = 0.05,
        .maxSpeed = 4,
        .minSpeed = 1,
        .maxBias = 0.01,
        .biasIncrement = 0.004
    };

    auto paramsBinding = WGPUBindGroupEntry {
        .nextInChain = nullptr,
        .binding = 0,
        .buffer = Params,
        .offset = 0,
        .size = sizeof(SimParams),
        .sampler = nullptr,
        .textureView = nullptr
    };
    auto boidsBinding = WGPUBindGroupEntry {
        .nextInChain = nullptr,
        .binding = 1,
        .buffer = boidsBuffer,
        .offset = 0,
        .size = sizeof(Boid) * boidsCount,
        .sampler = nullptr,
        .textureView = nullptr
    };

    updatePipeline->SetBinding(0, boidsBinding);
    updatePipeline->SetBinding(0, paramsBinding);
    applyPipeline->SetBinding(0, boidsBinding);
    applyPipeline->SetBinding(0, paramsBinding);

    updatePipeline->CommitBindings();
    applyPipeline->CommitBindings();

    renderPipeline = renderer->GetRenderPipelineByName("Boids")->CreateInstance();
    boidsBinding.binding = 0;
    renderPipeline->SetBinding(1, boidsBinding);
    renderPipeline->CommitBindings();

    auto cube = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/models/goldfish.obj");

    mesh = cube->mesh;

    initPositionsAndVelocities();
}

void BoidsComponent::Update(double deltaTime) {
    Params.Commit();

    BoidImmediates immediates = {
        .deltaTime = (float)deltaTime
    };
    auto pass = GetEngine()->GetRenderer()->BeginComputePass();
    updatePipeline->DispatchWorkgroups(pass, (boidsCount + 63) / 64, 1, 1, nullptr);
    applyPipeline->DispatchWorkgroups(pass, boidsCount, 1, 1, &immediates);
    GetEngine()->GetRenderer()->CommitComputePass(pass);
}

void BoidsComponent::Render(const glengine::pipeline::wgpu::RenderBundle &bundle, glengine::MatrixStack &stack) {
    mat4 m = stack;

    renderPipeline->DrawMeshInstanced(bundle, *mesh, boidsCount, &m);
}

void BoidsComponent::initPositionsAndVelocities() {
    auto boids = new Boid[boidsCount];
    for (int i = 0; i < boidsCount; i++) {
        boids[i].position = {
            glengine::math::frand(-5, 5),
            glengine::math::frand(-5, 5),
            glengine::math::frand(-5, 5),
            0
        };
        boids[i].velocity = {
            glengine::math::frand(-1, 1),
            glengine::math::frand(-1, 1),
            glengine::math::frand(-1, 1),
            0
        };
    }

    GetEngine()->GetRenderer()->GetTransferManager()->Transfer(boidsBuffer, 0, boids, sizeof(Boid) * boidsCount);
    delete[] boids;
}
