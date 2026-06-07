// Library wrapper around gpsbabel's GdbFormat reader.
// Bootstraps the minimum global state, invokes the reader, drains the
// resulting waypoint/route/track lists into POD structs, and tears down.

#include "gdbparse.h"

#include <exception>

#include <QDateTime>
#include <QString>

#include "defs.h"
#include "gdb.h"
#include "session.h"

// gpsbabel publishes these globals in waypt.cc / route.cc; declared at TU
// scope so they don't get pulled into the gdbparse namespace.
extern WaypointList* global_waypoint_list;
extern RouteList*    global_route_list;
extern RouteList*    global_track_list;

namespace {

gdbparse::UrlLink convert_url(const UrlLink& src) {
    return gdbparse::UrlLink{
        src.url_.toStdString(),
        src.url_link_text_.toStdString(),
        src.url_link_type_.toStdString(),
    };
}

std::vector<gdbparse::UrlLink> convert_urls(const UrlList& src) {
    std::vector<gdbparse::UrlLink> out;
    out.reserve(src.size());
    for (const auto& u : src) {
        out.push_back(convert_url(u));
    }
    return out;
}

gdbparse::Waypoint convert_waypoint(const Waypoint& src) {
    gdbparse::Waypoint w;
    w.shortname   = src.shortname.toStdString();
    w.description = src.description.toStdString();
    w.notes       = src.notes.toStdString();
    w.icon_descr  = src.icon_descr.toStdString();
    w.latitude    = src.latitude;
    w.longitude   = src.longitude;

    if (src.altitude != unknown_alt) {
        w.altitude = src.altitude;
    }
    if (src.proximity_has_value()) {
        w.proximity = src.proximity_value();
    }
    if (src.depth_has_value()) {
        w.depth = src.depth_value();
    }
    if (src.temperature_has_value()) {
        w.temperature = src.temperature_value();
    }
    if (src.course_has_value()) {
        w.course = src.course_value();
    }
    if (src.speed_has_value()) {
        w.speed = src.speed_value();
    }
    if (src.geoidheight_has_value()) {
        w.geoidheight = src.geoidheight_value();
    }

    if (src.creation_time.QDateTime::isValid()) {
        w.creation_time_unix = src.creation_time.toSecsSinceEpoch();
    }

    w.urls = convert_urls(src.urls);
    return w;
}

gdbparse::Route convert_route(const route_head& src) {
    gdbparse::Route r;
    r.name        = src.rte_name.toStdString();
    r.description = src.rte_desc.toStdString();
    r.urls        = convert_urls(src.rte_urls);
    r.line_color_rgb = src.line_color.bbggrr;
    r.line_width     = src.line_width;
    r.waypoints.reserve(src.waypoint_list.count());
    for (const Waypoint* wpt : src.waypoint_list) {
        r.waypoints.push_back(convert_waypoint(*wpt));
    }
    return r;
}

// RAII guard so we always tear down global state, even if drain throws.
class GpsbabelSession {
public:
    GpsbabelSession() {
        waypt_init();
        route_init();
        session_init();
        // Defaults so anything reading global_opts during parse sees sane values.
        global_opts.debug_level = 0;
        global_opts.verbose_status = 0;
        global_opts.objective = wptdata;
        global_opts.masked_objective = WPTDATAMASK | RTEDATAMASK | TRKDATAMASK | POSNDATAMASK;
        global_opts.inifile = nullptr;
        global_opts.synthesize_shortnames = false;
        global_opts.smart_icons = false;
        global_opts.smart_names = false;
    }

    ~GpsbabelSession() {
        // Order mirrors gpsbabel's main.cc teardown.
        waypt_flush_all();
        route_flush_all_routes();
        route_flush_all_tracks();
        route_deinit();
        waypt_deinit();
        session_exit();
    }

    GpsbabelSession(const GpsbabelSession&) = delete;
    GpsbabelSession& operator=(const GpsbabelSession&) = delete;
};

} // namespace

namespace gdbparse {

GdbData parse_gdb(const std::string& path) {
    GdbData out;
    GpsbabelSession session;

    GdbFormat fmt;
    const QString qpath = QString::fromStdString(path);

    // Waypoint construction calls curr_session(), which fatal()s when no
    // session has been pushed. Register one before any reader code runs.
    start_session("gdb", qpath);

    try {
        fmt.rd_init(qpath);
    } catch (const std::exception& e) {
        throw ParseError(std::string("rd_init failed: ") + e.what());
    }

    try {
        fmt.read();
    } catch (const std::exception& e) {
        try { fmt.rd_deinit(); } catch (...) { /* swallow during error path */ }
        throw ParseError(std::string("read failed: ") + e.what());
    }

    try {
        fmt.rd_deinit();
    } catch (const std::exception& e) {
        throw ParseError(std::string("rd_deinit failed: ") + e.what());
    }

    // Drain global lists into POD result.
    out.waypoints.reserve(global_waypoint_list->count());
    for (const ::Waypoint* wpt : *global_waypoint_list) {
        out.waypoints.push_back(convert_waypoint(*wpt));
    }

    out.routes.reserve(global_route_list->count());
    for (const ::route_head* rte : *global_route_list) {
        out.routes.push_back(convert_route(*rte));
    }

    out.tracks.reserve(global_track_list->count());
    for (const ::route_head* trk : *global_track_list) {
        out.tracks.push_back(convert_route(*trk));
    }

    return out;
}

} // namespace gdbparse
