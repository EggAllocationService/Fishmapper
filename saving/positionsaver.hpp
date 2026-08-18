//
// Created by Kyle Smith on 2026-08-18.
//
#pragma once
#include "../actors/PilotableCloud.h"

void loadPositions(std::vector<std::shared_ptr<PilotableCloud>>& clouds, std::vector<std::string>& serials);
void savePositions(std::vector<std::shared_ptr<PilotableCloud>>& clouds, std::vector<std::string>& serials);