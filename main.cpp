#include <BoidShaders.h>

#include "Engine.h"
#include "3d/mesh/GizmoComponent.h"
#include "components/BoidsComponent.h"
#include "components/TexturedModelComponent.h"
#include "pointmapper/kinect2/Kinect2Device.h"
#include "pointmapper/pipeline/PointmapperPipeline.h"
#include "pointmapper/pipeline/nodes/CreatePointCloudNode.h"
#include "pointmapper/pipeline/nodes/DepthCameraNode.h"
#include "pointmapper/pipeline/nodes/RemoveBackgroundNode.h"
#include "pointmapper/pipeline/nodes/RemoveBlobsNode.h"
#include "pointmapper/rendering/pipelines.h"
#include "pointmapper/rendering/PointCloudComponent.h"
#include "BoidShaders.h"

class PointActor : public glengine::world::Actor {
public:
    PointActor(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud) {
        auto component = CreateComponent<BoidsComponent>(1024, cloud);

        //auto c = CreateComponent<PointCloudComponent>();
        //c->SetCloudNode(cloud);
        //c->GetTransform()->SetScale({8,8,8});
        //c->GetTransform()->SetPosition({0, 0, -13});

        CreateComponent<glengine::world::mesh::GizmoComponent>("/builtin/models/cube.obj")
            ->GetTransform()
            ->SetScale({10, 10, 10});
    }

    void Update(double deltaTime) override {
        // animation or other per-frame calculations happen here
    }
};

class Environment : public glengine::world::Actor {
public:
    Environment() {
        CreateComponent<TexturedModelComponent>("/models/env.obj", "/textures/sand.jpg");
    }
    void Update(double deltaTime) override {}
};

struct alignas(16) FogParams {
    float4 color;
    float start;
    float end;
    float density;
};

int main() {
    auto engine = new glengine::Engine("Fishmapper", {1280, 720});
    engine->SetAllowNonFocusedPawnInput(true);
    engine->GetResourceManager()->MountPak("/", embed_boidresources_pak, embed_boidresources_pak_length);

    auto fog = engine->GetRenderer()->CompilePostEffect(embed_fog_wgsl, sizeof(FogParams));
    auto idx = engine->PushPostEffect(fog);
    auto fogParams = new FogParams();
    engine->SetPostData(idx, fogParams);
    fogParams->color = float4(0, 0, 1, 1);
    fogParams->start = 1;
    fogParams->end = 15;
    fogParams->density = 1.5;

    registerBoids(engine->GetRenderer());
    TexturedModelComponent::RegisterPipelines(engine->GetRenderer());

    auto pipeline = new pointmapper::pipeline::PointmapperPipeline(engine->GetRenderer()->GetDevice(), wgpuDeviceGetQueue(engine->GetRenderer()->GetDevice()));
    auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device());
    auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();
    auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
    auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();


    mask->camera_params->Connect(cam->params);
    mask->frameData->Connect(cam->frameData);
    mask->inputDepthMap->Connect(cam->depth);

    blobs->frameData->Connect(cam->frameData);
    blobs->camera_params->Connect(cam->params);
    blobs->inputDepthMap->Connect(mask->depthMap);


    cloud->camera_params->Connect(cam->params);
    cloud->color->Connect(cam->color);
    cloud->frameData->Connect(cam->frameData);
    cloud->depth_map->Connect(blobs->depthMap);

    pipeline->Build();
    addPointmapperPipelines(engine->GetRenderer());
    engine->Update();
    engine->GetPossessedPawn()->GetTransform()->SetPosition({0, 0, -10});

    engine->SpawnActor<PointActor>(cloud->cloud);
    auto env = engine->SpawnActor<Environment>();
    env->GetTransform()->SetScale({15, 20, 15});
    env->GetTransform()->SetPosition({0, 0, 0});

    while (!glfwWindowShouldClose(engine->GetWindow())) {
        glfwPollEvents();
        pipeline->Process();
        engine->Update();
        engine->Render();
    }
    return 0;
}
