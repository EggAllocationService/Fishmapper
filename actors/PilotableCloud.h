//
// Created by Kyle Smith on 2026-08-17.
//

#ifndef FISHMAPPER_PILOTABLECLOUD_H
#define FISHMAPPER_PILOTABLECLOUD_H
#include "3d/Pawn.h"
#include "pointmapper/pipeline/types/CommonTypes.h"
#include "pointmapper/rendering/PointCloudComponent.h"


class PilotableCloud : public glengine::world::Pawn {
public:
    PilotableCloud(std::shared_ptr<pointmapper::pipeline::Output<pointmapper::pipeline::GPUPointCloud>> cloud);
    void OnPossess(glengine::input::InputManager *inputManager) override;
};


#endif //FISHMAPPER_PILOTABLECLOUD_H
