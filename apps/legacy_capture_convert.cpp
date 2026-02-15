#include <cstdlib>
#include <iostream>

#include "ultrasound/replay.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: uss_legacy_capture_convert <legacy_file_or_dir> <output.csv>\n";
        return EXIT_FAILURE;
    }

    const auto status = ultrasound::convert_legacy_capture_to_replay_csv(argv[1], argv[2]);
    if (!status.is_ok()) {
        std::cerr << "Conversion failed: " << status.message << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "Conversion completed: " << argv[2] << "\n";
    return EXIT_SUCCESS;
}
