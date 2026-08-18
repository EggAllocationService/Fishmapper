//
// Created by Kyle Smith on 2026-08-17.
//

#include "PilotableCloud.h"

#include "Colors.h"
#include "Engine.h"
#include "InputManager.h"
#include "GLMath.h"
#include "3d/components/AxesComponent.h"

PilotableCloud::PilotableCloud(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud) {
    auto pointCloud = CreateComponent<PointCloudComponent>();
    pointCloud->SetCloudNode(cloud);

    auto hue = (int)glengine::math::frand(0, 360);
    pointCloud->SetTint(Colors::hsv(hue, 1.f, 1.f));
    pointCloud->SetMaxDepth(4);
    CreateComponent<glengine::world::components::AxesComponent>()->GetTransform()->SetScale({0.3, 0.3, 0.3});

    GetActiveCamera()->FoV = 70; // roughly depth fov
}

void PilotableCloud::Update(double deltaTime) {
}

void PilotableCloud::OnPossess(glengine::input::InputManager *manager) {
    // mouse look
    manager->AddMouseAxis([this](float2 delta) {
        auto rotation = this->GetTransform()->GetRotation();
        rotation.xy += float2(-delta.y, delta.x) * 0.0005;
        this->GetTransform()->SetRotation(rotation);
    });

    // w to go forward, s to go back
    manager->AddAxis('w', 's', [this](float amount) {
        auto position = this->GetTransform()->GetPosition();
        auto forward = this->GetTransform()->GetForwardVector();
        position = position + float3(forward.x, 0.0, forward.z).norm() * amount * 2;
        this->GetTransform()->SetPosition(position);
    });

    // d to go right, a to go left
    manager->AddAxis('d', 'a', [this](float amount) {
        auto position = this->GetTransform()->GetPosition();
        position = position + this->GetTransform()->GetRightVector() * amount * 2;
        this->GetTransform()->SetPosition(position);
    });

    // space to go up, c to go down
    manager->AddAxis(' ', 'c', [this](float amount) {
        auto position = this->GetTransform()->GetPosition();
        position = position + float3(0, 1, 0) * amount;
        this->GetTransform()->SetPosition(position);
    });

    // look left/right with keyboard
    manager->AddAxis(KEY_ARROW_RIGHT, KEY_ARROW_LEFT, [this](float amount) {
        auto rotation = this->GetTransform()->GetRotation();
        rotation.xy += float2(0, amount);
        this->GetTransform()->SetRotation(rotation);
    });

    // look up/down with keyboard
    manager->AddAxis(KEY_ARROW_UP, KEY_ARROW_DOWN, [this](float amount) {
        auto rotation = this->GetTransform()->GetRotation();
        rotation.xy += float2(-amount, 0);
        this->GetTransform()->SetRotation(rotation);
    });

    manager->AddAxis('q', 'e', [this](float amount) {
        auto rotation = this->GetTransform()->GetRotation();
        rotation.z += amount;
        this->GetTransform()->SetRotation(rotation);
    });
}

void PilotableCloud::OnUnpossess() {
    // intentionally empty: this pawn stays in the world when not possessed
}
