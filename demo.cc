// Minimal smoke-test / usage example for libgdbparse.
//
//     gdbparse_demo path/to/file.gdb

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include "gdbparse.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <file.gdb>\n", argv[0]);
        return 2;
    }

    try {
        gdbparse::GdbData data = gdbparse::parse_gdb(argv[1]);

        std::printf("waypoints: %zu\n", data.waypoints.size());
        std::printf("routes:    %zu\n", data.routes.size());
        std::printf("tracks:    %zu\n", data.tracks.size());

        for (std::size_t i = 0; i < data.waypoints.size(); ++i) {
            const auto& w = data.waypoints[i];
            std::printf("  W[%zu] %s  lat=%.6f lon=%.6f%s%s\n",
                        i,
                        w.shortname.c_str(),
                        w.latitude,
                        w.longitude,
                        w.altitude ? "  alt=" : "",
                        w.altitude ? std::to_string(*w.altitude).c_str() : "");
        }
        for (std::size_t i = 0; i < data.routes.size(); ++i) {
            const auto& r = data.routes[i];
            std::printf("  R[%zu] %s  (%zu pts)\n",
                        i, r.name.c_str(), r.waypoints.size());
        }
        for (std::size_t i = 0; i < data.tracks.size(); ++i) {
            const auto& t = data.tracks[i];
            std::printf("  T[%zu] %s  (%zu pts)\n",
                        i, t.name.c_str(), t.waypoints.size());
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "parse error: %s\n", e.what());
        return 1;
    }
    return 0;
}
