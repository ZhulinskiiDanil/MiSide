#include "Model.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    try {
        mita3d::Model model(argv[1]);
        std::filesystem::create_directories(argv[2]);
        for (int frame = 0; frame <= 84; ++frame) {
            float time = frame / 30.f;
            model.pose(time, 0, {}, {}, time);
            model.exportPose(std::filesystem::path(argv[2]) / (std::to_string(frame) + ".pose"));
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
