// gdbparse — minimal C++ library that parses Garmin MapSource .gdb files.
//
// Wraps the GdbFormat reader from gpsbabel 1.9.0. Produces POD-style
// result structs using only stdlib types (no Qt types leaked through the API).
//
// Usage:
//     #include "gdbparse.h"
//     gdbparse::GdbData data = gdbparse::parse_gdb("track.gdb");
//
// On parse failure, throws gdbparse::ParseError (derived from std::runtime_error).
// Thread-safety: the underlying gpsbabel reader relies on global state,
// so parse_gdb() is NOT thread-safe. Serialize calls externally.

#ifndef GDBPARSE_H_INCLUDED_
#define GDBPARSE_H_INCLUDED_

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gdbparse {

struct UrlLink {
    std::string url;
    std::string url_link_text;
    std::string url_link_type;
};

struct Waypoint {
    std::string shortname;
    std::string description;
    std::string notes;
    std::string icon_descr;

    double latitude  = 0.0;
    double longitude = 0.0;

    // Altitude in meters. unset if missing in the source.
    std::optional<double> altitude;

    // Optional fields (set only when present in source).
    std::optional<double> proximity;     // meters
    std::optional<double> depth;         // meters
    std::optional<float>  temperature;   // celsius
    std::optional<float>  course;        // degrees true
    std::optional<float>  speed;         // m/s
    std::optional<double> geoidheight;   // meters

    // Unix epoch seconds (UTC). 0 if no creation time was supplied.
    int64_t creation_time_unix = 0;

    std::vector<UrlLink> urls;
};

struct Route {
    std::string name;
    std::string description;
    std::vector<UrlLink> urls;

    // line color packed as 0x00RRGGBB, or -1 if not set.
    int32_t line_color_rgb = -1;
    // line width in pixels, or -1 if unknown.
    int32_t line_width = -1;

    std::vector<Waypoint> waypoints;
};

// Tracks share the same shape as Routes in gpsbabel's data model.
using Track = Route;

struct GdbData {
    std::vector<Waypoint> waypoints;
    std::vector<Route>    routes;
    std::vector<Track>    tracks;
};

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Parse a .gdb file at the given path. Throws ParseError on failure.
GdbData parse_gdb(const std::string& path);

} // namespace gdbparse

#endif // GDBPARSE_H_INCLUDED_
