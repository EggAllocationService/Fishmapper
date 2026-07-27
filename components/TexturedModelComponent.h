//
// Created by Kyle Smith on 2026-07-27.
//

#ifndef FISHMAPPER_TEXTUREDMODELCOMPONENT_H
#define FISHMAPPER_TEXTUREDMODELCOMPONENT_H
#include "3d/ActorSceneComponent.h"


class TexturedModelComponent : public glengine::world::ActorSceneComponent {
public:
    TexturedModelComponent(std::string_view model, std::string_view texture);
    void Render(const glengine::pipeline::wgpu::RenderBundle &bundle, glengine::MatrixStack &stack) override;

    static void RegisterPipelines(glengine::pipeline::wgpu::WGPURenderer* renderer);
private:
    std::shared_ptr<glengine::pipeline::wgpu::GPUMesh> mesh;
    std::shared_ptr<glengine::pipeline::wgpu::RenderPipeline> pipeline;
    std::shared_ptr<glengine::pipeline::wgpu::GPUTexture> texture;
};


#endif //FISHMAPPER_TEXTUREDMODELCOMPONENT_H
