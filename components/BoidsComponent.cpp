//
// Created by Kyle Smith on 2026-07-22.
//

#include "BoidsComponent.h"

#include "Engine.h"
#include "BoidShaders.h"
#include "GLMath.h"
#include "3d/mesh/StaticMesh.h"

void registerBoids(glengine::pipeline::wgpu::WGPURenderer *renderer) {
    WGPUBindGroupLayoutEntry boidEntries[2] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    boidEntries[0].visibility = WGPUShaderStage_Compute;
    boidEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(SimParams)
    };
    boidEntries[1].binding = 1;
    boidEntries[1].visibility = WGPUShaderStage_Compute;
    boidEntries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Storage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(Boid)
    };

    WGPUBindGroupLayoutEntry foodFieldEntries[3] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    foodFieldEntries[0].visibility = WGPUShaderStage_Compute;
    foodFieldEntries[0].texture = {
        .nextInChain = nullptr,
        .sampleType = WGPUTextureSampleType_Float,
        .viewDimension = WGPUTextureViewDimension_3D,
        .multisampled = false
    };
    foodFieldEntries[1].binding = 1;
    foodFieldEntries[1].visibility = WGPUShaderStage_Compute;
    foodFieldEntries[1].sampler = {
        .nextInChain = nullptr,
        .type = WGPUSamplerBindingType_Filtering
    };
    foodFieldEntries[2].binding = 2;
    foodFieldEntries[2].visibility = WGPUShaderStage_Compute;
    foodFieldEntries[2].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(VoxelParams)
    };

    WGPUBindGroupLayoutDescriptor boidLayouts[2] = {
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 2,
            .entries = boidEntries
        },
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 3,
            .entries = foodFieldEntries
        }
    };
    auto boidKernels = renderer->CompileShader(embed_boids_wgsl);
    renderer->BuildComputePipeline("BoidsUpdate", boidKernels, "update_boids", {boidLayouts, 2}, 0);
    renderer->BuildComputePipeline("BoidsApply", boidKernels, "apply_velocities", {boidLayouts, 1}, sizeof(BoidImmediates));

    WGPUBindGroupLayoutEntry pointEntries[2] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    pointEntries[0].visibility = WGPUShaderStage_Compute;
    pointEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(float) * 4
    };
    pointEntries[1].binding = 1;
    pointEntries[1].visibility = WGPUShaderStage_Compute;
    pointEntries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(uint32_t) * 4
    };

    WGPUBindGroupLayoutEntry voxelEntries[3] = {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    voxelEntries[0].visibility = WGPUShaderStage_Compute;
    voxelEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Storage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(uint32_t)
    };
    voxelEntries[1].binding = 1;
    voxelEntries[1].visibility = WGPUShaderStage_Compute;
    voxelEntries[1].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Storage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(uint32_t)
    };
    voxelEntries[2].binding = 2;
    voxelEntries[2].visibility = WGPUShaderStage_Compute;
    voxelEntries[2].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(VoxelParams)
    };

    WGPUBindGroupLayoutEntry voxelBoidEntries[1] = {WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT};
    voxelBoidEntries[0].visibility = WGPUShaderStage_Compute;
    voxelBoidEntries[0].buffer = {
        .nextInChain = nullptr,
        .type = WGPUBufferBindingType_ReadOnlyStorage,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(Boid)
    };

    WGPUBindGroupLayoutEntry vectorFieldEntries[1] = {WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT};
    vectorFieldEntries[0].visibility = WGPUShaderStage_Compute;
    vectorFieldEntries[0].storageTexture = {
        .nextInChain = nullptr,
        .access = WGPUStorageTextureAccess_WriteOnly,
        .format = WGPUTextureFormat_RGBA16Float,
        .viewDimension = WGPUTextureViewDimension_3D
    };

    WGPUBindGroupLayoutDescriptor voxelLayouts[4] = {
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 2,
            .entries = pointEntries
        },
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 3,
            .entries = voxelEntries
        },
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 1,
            .entries = voxelBoidEntries
        },
        {
            .nextInChain = nullptr,
            .label = {},
            .entryCount = 1,
            .entries = vectorFieldEntries
        }
    };
    auto voxelKernels = renderer->CompileShader(embed_voxels_wgsl);
    renderer->BuildComputePipeline("ClearVoxels", voxelKernels, "clear_voxels", {voxelLayouts, 4}, 0);
    renderer->BuildComputePipeline("VoxelizePoints", voxelKernels, "voxelize_points", {voxelLayouts, 4}, sizeof(VoxelizeImmediates));
    renderer->BuildComputePipeline("EatPoints", voxelKernels, "eat_points", {voxelLayouts, 4}, 0);
    renderer->BuildComputePipeline("GenerateVectorField", voxelKernels, "generate_vector_field", {voxelLayouts, 4}, 0);

    WGPUBindGroupLayoutDescriptor renderLayout = {
        .nextInChain = nullptr,
        .label = {},
        .entryCount = 1,
        .entries = &boidEntries[1]
    };
    boidEntries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    boidEntries[1].visibility = WGPUShaderStage_Vertex;
    boidEntries[1].binding = 0;

    auto shaders = renderer->CompileShader(embed_boidshaders_wgsl);
    renderer->BuildRenderPipeline("Boids", shaders, nullptr, {&renderLayout, 1}, sizeof(mat4), nullptr);
}

BoidsComponent::BoidsComponent(unsigned int count, const std::vector<CloudInput>& cloudInputs) {
    auto renderer = GetEngine()->GetRenderer();
    boidsBuffer = renderer->CreateRawBuffer("boids data", WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, sizeof(Boid) * count);
    pointDensityBuffer = renderer->CreateRawBuffer("voxel density", WGPUBufferUsage_Storage, sizeof(uint32_t) * VoxelCount);
    eatenPointBuffer = renderer->CreateRawBuffer("eaten voxel density", WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, sizeof(uint32_t) * VoxelCount);
    foodFieldTexture = renderer->CreateTexture("food vector field", WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding, WGPUTextureFormat_RGBA16Float, VoxelResolution, VoxelResolution, VoxelResolution);

    dummyPointBuffer = renderer->CreateRawBuffer("dummy points", WGPUBufferUsage_Storage, sizeof(float) * 4);
    dummyPointCountBuffer = renderer->CreateRawBuffer("dummy point count", WGPUBufferUsage_Storage, sizeof(uint32_t) * 4);
    boidsCount = count;

    WGPUSamplerDescriptor samplerDescriptor = WGPU_SAMPLER_DESCRIPTOR_INIT;
    samplerDescriptor.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.magFilter = WGPUFilterMode_Linear;
    samplerDescriptor.minFilter = WGPUFilterMode_Linear;
    foodFieldSampler = wgpuDeviceCreateSampler(renderer->GetDevice(), &samplerDescriptor);

    updatePipeline = renderer->GetComputePipelineByName("BoidsUpdate")->CreateInstance();
    applyPipeline = renderer->GetComputePipelineByName("BoidsApply")->CreateInstance();
    clearVoxelsPipeline = renderer->GetComputePipelineByName("ClearVoxels")->CreateInstance();
    eatPointsPipeline = renderer->GetComputePipelineByName("EatPoints")->CreateInstance();
    generateVectorFieldPipeline = renderer->GetComputePipelineByName("GenerateVectorField")->CreateInstance();
    Params = renderer->AllocateObject<SimParams>(WGPUBufferUsage_Uniform);
    voxelParams = renderer->AllocateObject<VoxelParams>(WGPUBufferUsage_Uniform);

    *Params = {
        .count = boidsCount,
        .turnFactor = 0.2,
        .visualRange = 1,
        .protectedRange = 0.8,
        .centeringFactor = 0.05,
        .avoidFactor = 0.09,
        .matchingFactor = 0.05,
        .maxSpeed = 2,
        .minSpeed = 1,
        .maxBias = 0.01,
        .biasIncrement = 0.004,
        .foodSeekingFactor = 2.0
    };
    *voxelParams = {
        .bounds = 10.0,
        .resolution = VoxelResolution,
        .foodPerBoid = 1,
        .padding = 0
    };
    Params.Commit();
    voxelParams.Commit();

    WGPUBindGroupEntry samplerEntry = WGPU_BIND_GROUP_ENTRY_INIT;
    samplerEntry.binding = 1;
    samplerEntry.sampler = foodFieldSampler;
    updatePipeline->SetBinding(0, 0, Params);
    updatePipeline->SetBinding(0, 1, boidsBuffer);
    updatePipeline->SetBinding(1, 0, *foodFieldTexture);
    updatePipeline->SetBinding(1, samplerEntry);
    updatePipeline->SetBinding(1, 2, voxelParams);
    updatePipeline->CommitBindings();

    applyPipeline->SetBinding(0, 0, Params);
    applyPipeline->SetBinding(0, 1, boidsBuffer);
    applyPipeline->CommitBindings();

    auto bindSharedVoxelBindings = [&](const std::shared_ptr<glengine::pipeline::wgpu::ComputePipeline>& pipeline) {
        pipeline->SetBinding(0, 0, dummyPointBuffer);
        pipeline->SetBinding(0, 1, dummyPointCountBuffer);
        pipeline->SetBinding(1, 0, pointDensityBuffer);
        pipeline->SetBinding(1, 1, eatenPointBuffer);
        pipeline->SetBinding(1, 2, voxelParams);
        pipeline->SetBinding(2, 0, boidsBuffer);
        pipeline->SetBinding(3, 0, *foodFieldTexture);
        pipeline->CommitBindings();
    };
    bindSharedVoxelBindings(clearVoxelsPipeline);
    bindSharedVoxelBindings(eatPointsPipeline);
    bindSharedVoxelBindings(generateVectorFieldPipeline);

    auto baseVoxelizePipeline = renderer->GetComputePipelineByName("VoxelizePoints");
    for (const auto& input : cloudInputs) {
        CloudData data;
        data.cloud = input.cloud;
        data.getTransform = input.getTransform;
        data.voxelizePipeline = baseVoxelizePipeline->CreateInstance();
        data.maximumPointCount = (*input.cloud)->maximumPointCount;

        data.voxelizePipeline->SetBinding(0, 0, (*input.cloud)->points);
        data.voxelizePipeline->SetBinding(0, 1, (*input.cloud)->pointCount);
        data.voxelizePipeline->SetBinding(1, 0, pointDensityBuffer);
        data.voxelizePipeline->SetBinding(1, 1, eatenPointBuffer);
        data.voxelizePipeline->SetBinding(1, 2, voxelParams);
        data.voxelizePipeline->SetBinding(2, 0, boidsBuffer);
        data.voxelizePipeline->SetBinding(3, 0, *foodFieldTexture);
        data.voxelizePipeline->CommitBindings();

        clouds.push_back(std::move(data));
    }

    renderPipeline = renderer->GetRenderPipelineByName("Boids")->CreateInstance();
    renderPipeline->SetBinding(1, 0, boidsBuffer);
    renderPipeline->CommitBindings();

    auto cube = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>("/models/goldfish.obj");
    mesh = cube->mesh;

    std::vector<uint32_t> emptyEatenGrid(VoxelCount, 0);
    renderer->GetTransferManager()->Transfer(eatenPointBuffer, 0, emptyEatenGrid.data(), sizeof(uint32_t) * VoxelCount);
    initPositionsAndVelocities();
}

BoidsComponent::~BoidsComponent() {
    if (foodFieldSampler != nullptr) {
        wgpuSamplerRelease(foodFieldSampler);
    }
}

void BoidsComponent::Update(double deltaTime) {
    Params.Commit();

    BoidImmediates immediates = {
        .deltaTime = static_cast<float>(deltaTime)
    };
    auto pass = GetEngine()->GetRenderer()->BeginComputePass();
    clearVoxelsPipeline->DispatchWorkgroups(pass, (VoxelCount + 63) / 64, 1, 1, nullptr);
    for (const auto& cloud : clouds) {
        VoxelizeImmediates voxelImmediates = {
            .transform = cloud.getTransform()
        };
        cloud.voxelizePipeline->DispatchWorkgroups(pass, (cloud.maximumPointCount + 63) / 64, 1, 1, &voxelImmediates);
    }
    eatPointsPipeline->DispatchWorkgroups(pass, (boidsCount + 63) / 64, 1, 1, nullptr);
    generateVectorFieldPipeline->DispatchWorkgroups(pass, (VoxelResolution + 3) / 4, (VoxelResolution + 3) / 4, (VoxelResolution + 3) / 4, nullptr);
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
        boids[i].destination = float4(0, 0, 0, 0);
        boids[i].counter = 0;
    }

    GetEngine()->GetRenderer()->GetTransferManager()->Transfer(boidsBuffer, 0, boids, sizeof(Boid) * boidsCount);
    delete[] boids;
}
