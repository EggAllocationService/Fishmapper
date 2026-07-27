//
// Created by Kyle Smith on 2026-07-27.
//

#include "TexturedModelComponent.h"

#include <BoidShaders.h>

#include "Engine.h"
#include "3d/texture/StaticTexture2D.h"
#include "3d/mesh/StaticMesh.h"

TexturedModelComponent::TexturedModelComponent(std::string_view model, std::string_view texture) {
    auto m = GetEngine()->GetResourceManager()->GetResource<glengine::world::mesh::StaticMesh>(model);
    mesh = m->mesh;
    auto t = GetEngine()->GetResourceManager()->GetResource<glengine::world::texture::StaticTexture2D>(texture);
    this->texture = t->GetGPUTexture();
    this->pipeline = GetEngine()->GetRenderer()->GetRenderPipelineByName("textured")->CreateInstance();

    pipeline->SetBinding(1, 0, *this->texture);
    pipeline->CommitBindings();
}

void TexturedModelComponent::Render(const glengine::pipeline::wgpu::RenderBundle &bundle,
    glengine::MatrixStack &stack) {
    mat4 m = stack;
    pipeline->DrawMesh(bundle, *mesh, &m);
}

void TexturedModelComponent::RegisterPipelines(glengine::pipeline::wgpu::WGPURenderer *renderer) {
    auto entries = new WGPUBindGroupLayoutEntry[2] {
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
        WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT
    };
    entries[0].texture = {
        .nextInChain = nullptr,
        .sampleType = WGPUTextureSampleType_Float,
        .viewDimension = WGPUTextureViewDimension_2D,
        .multisampled = false
    };
    entries[0].visibility = WGPUShaderStage_Fragment;

    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].sampler = {
        .nextInChain = nullptr,
        .type = WGPUSamplerBindingType_Filtering
    };

    auto group = WGPUBindGroupLayoutDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .entryCount = 2,
        .entries = entries
    };
    auto extras = glengine::pipeline::wgpu::RenderPipelineExtras {
        .polygonMode = WGPUPolygonMode_Fill,
        .depthMode = WGPUCompareFunction_LessEqual,
        .cullMode = WGPUCullMode_None,
        .primitiveTopology = WGPUPrimitiveTopology_TriangleList
    };
    auto shaders = renderer->CompileShader(embed_texture_wgsl);
    auto pipeline = renderer->BuildRenderPipeline("textured", shaders, nullptr, {&group, 1}, sizeof(mat4), &extras);
    auto sDesc = WGPUSamplerDescriptor {
        .nextInChain = nullptr,
        .label = {},
        .addressModeU = WGPUAddressMode_Repeat,
        .addressModeV = WGPUAddressMode_Repeat,
        .addressModeW = WGPUAddressMode_Undefined,
        .magFilter = WGPUFilterMode_Linear,
        .minFilter = WGPUFilterMode_Linear,
        .mipmapFilter = WGPUMipmapFilterMode_Linear,
        .lodMinClamp = 0,
        .lodMaxClamp = 32,
        .compare = WGPUCompareFunction_Undefined,
        .maxAnisotropy = 1
    };
    auto entry = WGPUBindGroupEntry {
        .nextInChain = nullptr,
        .binding = 1,
        .buffer = nullptr,
        .offset = 0,
        .size = 0,
        .sampler = wgpuDeviceCreateSampler(renderer->GetDevice(), &sDesc),
        .textureView = nullptr
    };
    pipeline->SetBinding(1, entry);
}
