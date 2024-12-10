#pragma once

#include <unordered_map>
#include <forward_list>
#include <map>
#include <optional>
#include <variant>

#include "graph.h"
#include "router.h"
#include "domain.h"
#include "transport_catalogue.h"

namespace routing {

struct RoutingSettings {
    int bus_wait_time = 6;
    int bus_velocity = 40;
};

class TransportRouter {
    struct StopVertex {
        graph::VertexId begin;
        graph::VertexId end;
    };
public:
    struct StopEdge {
        std::string_view stop_name;
        int time;
    };
    struct BusEdge {
        std::string_view bus;
        int span_count;
        double time;
    };
    explicit TransportRouter(const TransportCatalogue& db, const RoutingSettings& settings, size_t vertex_count);
    std::pair<double, std::vector<std::variant<StopEdge, BusEdge>>> BuildRoute(std::string_view from, std::string_view to) const;
private:

    RoutingSettings settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::optional<graph::Router<double>> router_;
    std::unordered_map<std::string_view, StopVertex>  stop_vertex_;
    std::unordered_map<graph::EdgeId, StopEdge> stop_edges_;
    std::unordered_map<graph::EdgeId, BusEdge> bus_edges_;
    const TransportCatalogue& db_;

    void AddStops();
    void AddBuses();
};

} //namespace routing

