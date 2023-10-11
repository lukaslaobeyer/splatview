#include "logging.h"

#include <iostream>

namespace logging {
void print_progress(double progress) {
    constexpr int bar_width = 70;

    std::cout << "[";
    const int pos = bar_width * progress;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";

    if (progress >= 1.0) {
        std::cout << std::endl;
    }

    std::cout.flush();
}
} // namespace logging
