//
// Created by Kyle Smith on 2026-08-18.
//

#include "positionsaver.hpp"
#include "../external/tinyfiledialogs.h"
#include <iostream>
#include <fstream>

void loadPositions(std::vector<std::shared_ptr<PilotableCloud>> &clouds, std::vector<std::string> &serials) {
    if (clouds.size() != serials.size()) {
        tinyfd_messageBox("Error", "Invalid parameters passed to loadPositions", nullptr, nullptr, 0);
        return;
    }
    auto filename = tinyfd_openFileDialog("Select Saved Positions", nullptr, 0, nullptr, nullptr, 0);
    if (filename == nullptr) {
        return; // user cancelled
    }
    std::ifstream ifs(filename);

    int count = 0;
    ifs >> count;
    for (int i = 0; i < count; i++) {
        std::string serial;
        float3 position;
        float3 rotation;
        ifs >> serial >> position.x >> position.y >> position.z >> rotation.x >> rotation.y >> rotation.z;

        auto it = std::find(serials.begin(), serials.end(), serial);
        if (it == serials.end()) {
            continue; // that camera is not connected
        }
        auto index = (size_t) std::distance(serials.begin(), it);
        clouds[index]->GetTransform()->SetPosition(position);
        clouds[index]->GetTransform()->SetRotation(rotation);
    }
}

void savePositions(std::vector<std::shared_ptr<PilotableCloud>> &clouds, std::vector<std::string> &serials) {
    if (clouds.size() != serials.size()) {
        tinyfd_messageBox("Error", "Invalid parameters passed to savePositions", nullptr, nullptr, 0);
        return;
    }

    auto filename = tinyfd_saveFileDialog("Save Camera Positions", nullptr, 0, nullptr, nullptr);
    if (filename == nullptr) {
        return; // user cancelled
    }

    std::ofstream ofs(filename, std::ios::trunc);

    ofs << clouds.size() << std::endl;
    for (int i = 0; i < clouds.size(); i++) {
        ofs << serials[i] << " ";
        auto pos = clouds[i]->GetTransform()->GetPosition();
        ofs << pos.x << " " << pos.y << " " << pos.z << " ";
        auto rot = clouds[i]->GetTransform()->GetRotation();
        ofs << rot.x << " " << rot.y << " " << rot.z << std::endl;
    }
    ofs.close();
}
