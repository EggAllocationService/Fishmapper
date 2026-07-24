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
    WGPUBindGroupLayoutEntry pointEntries[2] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    entries[0].visibility = WGPUShaderStage_Compute;
    entries[1].visibility = WGPUShaderStage_Compute;
    entries[1].binding = 1;
    pointEntries[0].visibility = WGPUShaderStage_Compute;
    pointEntries[1].visibility = WGPUShaderStage_Compute;
    pointEntries[1].binding = 1;

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

    pointEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(uint32_t) * 4
    };
    pointEntries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = 0
    };

    WGPUBindGroupLayoutDescriptor desc[2] = {
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 2,
            .entries = &entries[0]
        },
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 2,
            .entries = &pointEntries[0]
        }
    };
    renderer->BuildComputePipeline("BoidsUpdate", kernels, "update_boids", {desc, 2}, 0);
    renderer->BuildComputePipeline("BoidsApply", kernels, "apply_velocities", {desc, 1}, sizeof(BoidImmediates));

    desc[0].entryCount = 1;
    desc[0].entries = &entries[1];
    entries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[1].visibility = WGPUShaderStage_Vertex;
    entries[1].binding = 0;

    auto shaders = renderer->CompileShader(embed_boidshaders_wgsl);
    renderer->BuildRenderPipeline("Boids", shaders, nullptr, {desc, 1}, sizeof(mat4), nullptr);

}

BoidsComponent::BoidsComponent(unsigned int count, std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud) {
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
        .protectedRange = 0.8,
        .centeringFactor = 0.01,
        .avoidFactor = 0.09,
        .matchingFactor = 0.005,
        .maxSpeed = 2,
        .minSpeed = 1,
        .maxBias = 0.01,
        .biasIncrement = 0.004,
        .targetFactor = 0.03,
        .jitterRadius = 0.4,
        .frame = 0,
        .pad1 = 0,
        .pad2 = 0
    };

    updatePipeline->SetBinding(0, 0, Params);
    updatePipeline->SetBinding(0, 1, boidsBuffer);
    updatePipeline->SetBinding(1, 0, (*cloud)->pointCount);
    updatePipeline->SetBinding(1, 1, (*cloud)->points);

    applyPipeline->SetBinding(0, 0, Params);
    applyPipeline->SetBinding(0, 1, boidsBuffer);

    updatePipeline->CommitBindings();
    applyPipeline->CommitBindings();

    renderPipeline = renderer->GetRenderPipelineByName("Boids")->CreateInstance();
    renderPipeline->SetBinding(1, 0, boidsBuffer);
    renderPipeline->CommitBindings();

    auto cube = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/models/goldfish.obj");

    mesh = cube->mesh;

    initPositionsAndVelocities();
}

void BoidsComponent::Update(double deltaTime) {
    (*Params).frame++;
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
        boids[i].goal = {0, 0, 0, 0};
    }

    GetEngine()->GetRenderer()->GetTransferManager()->Transfer(boidsBuffer, 0, boids, sizeof(Boid) * boidsCount);
    delete[] boids;
}
