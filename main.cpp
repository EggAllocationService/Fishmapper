#include <BoidShaders.h>

#include "Engine.h"
#include "3d/mesh/GizmoComponent.h"
#include "actors/PilotableCloud.h"
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
#include "3d/components/AxesComponent.h"
#ifdef REALSENSE_SUPPORT
#include "pointmapper/realsense/RealsenseDevice.h"
#endif
#include "saving/positionsaver.hpp"

class SimulationActor : public glengine::world::Actor {
public:
    SimulationActor(const std::vector<CloudInput>& clouds) {
        boids = CreateComponent<BoidsComponent>(1024, clouds);

        bounds = CreateComponent<glengine::world::mesh::GizmoComponent>("/builtin/models/cube.obj");
        bounds->GetTransform()->SetScale({10, 10, 10});

        axes = CreateComponent<glengine::world::components::AxesComponent>();
    }

    void SetDebugViz(bool val) {
        bounds->SetEnabled(val);
        axes->SetEnabled(val);
        boids->SetEnabled(!val);
    }

    void Update(double deltaTime) override {
        // animation or other per-frame calculations happen here
    }

private:
    std::shared_ptr<BoidsComponent> boids;
    std::shared_ptr<glengine::world::mesh::GizmoComponent> bounds;
    std::shared_ptr<glengine::world::components::AxesComponent> axes;
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

bool debug = false;
bool background = true;

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
    std::vector<std::string> devices = Kinect2Device::EnumerateDevices();
#ifdef REALSENSE_SUPPORT
    std::vector<std::string> realsenseDevices = RealsenseDevice::EnumerateDevices();
#endif

    std::vector<std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>>> cloudOutputs;
    std::vector<std::shared_ptr<PilotableCloud>> pilotableClouds;

    for (size_t i = 0; i < devices.size(); i++) {
        auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new Kinect2Device(devices[i]));
        auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
        auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();
        auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();

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

        cloudOutputs.push_back(cloud->cloud);
    }

#ifdef REALSENSE_SUPPORT
    for (size_t i = 0; i < realsenseDevices.size(); i++) {
        auto cam = pipeline->CreateRoot<pointmapper::pipeline::DepthCameraNode>(new RealsenseDevice(realsenseDevices[i]));
        auto mask = pipeline->CreateNode<pointmapper::pipeline::RemoveBackgroundNode>();
        auto blobs = pipeline->CreateNode<pointmapper::pipeline::RemoveBlobsNode>();
        auto cloud = pipeline->CreateNode<pointmapper::pipeline::CreatePointCloudNode>();

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

        cloudOutputs.push_back(cloud->cloud);

        devices.push_back(realsenseDevices[i]);
    }
#endif

    pipeline->Build();
    addPointmapperPipelines(engine->GetRenderer());
    engine->Update();
    engine->GetPossessedPawn()->GetTransform()->SetPosition({0, 0, -10});

    std::vector<CloudInput> cloudInputs;
    for (size_t i = 0; i < cloudOutputs.size(); i++) {
        auto pc = engine->SpawnActor<PilotableCloud>(cloudOutputs[i]);
        pc->GetComponent<PointCloudComponent>()->GetTransform()->SetScale({8, 8, 8});
        pc->GetTransform()->SetPosition({static_cast<float>(i) * 5.0f, 0, -13});
        pc->SetEnabled(false);
        pilotableClouds.push_back(pc);

        cloudInputs.push_back({
            cloudOutputs[i],
            [pc]() { return pc->GetTransform()->GetAbsoluteMatrix() * pc->GetComponent<PointCloudComponent>()->GetTransform()->GetAbsoluteMatrix(); }
        });
    }

    auto sim = engine->SpawnActor<SimulationActor>(cloudInputs);
    sim->SetDebugViz(false);

    auto env = engine->SpawnActor<Environment>();
    env->GetTransform()->SetScale({15, 20, 15});
    env->GetTransform()->SetPosition({0, 0, 0});

    int possessedCloudIndex = 0;
    bool possessingCloud = false;
    auto possessCloud = [&](int index) {
        if (pilotableClouds.empty()) return;
        possessedCloudIndex = index;
        if (possessedCloudIndex < 0) possessedCloudIndex = static_cast<int>(pilotableClouds.size()) - 1;
        if (possessedCloudIndex >= static_cast<int>(pilotableClouds.size())) {
            engine->Possess(nullptr);
            return;
        }
        engine->Possess(pilotableClouds[possessedCloudIndex]);
        possessingCloud = true;
    };

    auto enterFreeCamera = [&] {
        engine->Possess(nullptr);
        possessingCloud = false;
        engine->GetPossessedPawn()->GetTransform()->SetPosition({0, 0, -10});
    };

    engine->GetInputManager()->AddAction(',', [&] {
        if (!debug) return;
        if (!possessingCloud) {
            possessCloud(0);
        } else {
            possessCloud(possessedCloudIndex - 1);
        }
    });
    engine->GetInputManager()->AddAction('.', [&] {
        if (!debug) return;
        if (!possessingCloud) {
            possessCloud(0);
        } else {
            possessCloud(possessedCloudIndex + 1);
        }
    });

    engine->GetInputManager()->AddAction('g', [=, &possessedCloudIndex, &possessingCloud] {
        debug = !debug;
        sim->SetDebugViz(debug);
        for (auto& pc : pilotableClouds) {
            pc->SetEnabled(debug);
        }
        if (debug) {
            possessingCloud = false;
            env->SetEnabled(false);
            engine->SetPostEffectEnabled(idx, false);
        } else {
            enterFreeCamera();
            env->SetEnabled(background);
            engine->SetPostEffectEnabled(idx, background);
        }
    });

    engine->GetInputManager()->AddAction('b', [=] {
        background = !background;
        if (!debug) {
            env->SetEnabled(background);
            engine->SetPostEffectEnabled(idx, background);
        }
    });

    engine->GetInputManager()->AddAction('f', [=] {
        if (!possessingCloud) {
            auto t = engine->GetPossessedPawn()->GetActiveCamera()->GetTransform();
            t->SetScale(t->GetScale() * float3(-1, 1, 1));
        }
    });

    engine->GetInputManager()->AddAction('o', [&] {
       savePositions(pilotableClouds, devices);
    });
    engine->GetInputManager()->AddAction('l', [&] {
       loadPositions(pilotableClouds, devices);
    });

    while (!glfwWindowShouldClose(engine->GetWindow())) {
        glfwPollEvents();
        pipeline->Process();
        engine->Update();
        engine->Render();
    }
    return 0;
}
