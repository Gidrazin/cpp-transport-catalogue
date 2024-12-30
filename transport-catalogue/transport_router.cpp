#include "transport_router.h"


namespace routing {

TransportRouter::TransportRouter(const TransportCatalogue& db, const RoutingSettings& settings)
: settings_(settings)
, router_(std::nullopt)
, db_(db)
{   
    graph::DirectedWeightedGraph<double> tmp_graph(std::distance(db_.GetStopsList().begin(), db_.GetStopsList().end()) * 2);
    graph_ = std::move(tmp_graph);
    AddStops();
    AddBuses();
    router_.emplace(graph::Router<double>(graph_));
}

void TransportRouter::AddStops(){
    graph::VertexId vertex_id = 0;
    for (const auto& stop: db_.GetStopsList()){
        stop_vertex_[stop.name] = {vertex_id, vertex_id + 1};
        graph::EdgeId id = graph_.AddEdge({vertex_id, vertex_id + 1, static_cast<double>(settings_.bus_wait_time)});
        stop_edges_[id] = { stop.name, settings_.bus_wait_time };
        vertex_id += 2;
    }
}

void TransportRouter::AddBuses() {
    for (const auto& [bus_name, bus]: db_.GetBuses()){
        size_t stops_count = bus->stops.size();
        for (size_t i = 0; i < stops_count; ++i) {
            for (size_t j = i + 1; j < stops_count; ++j) {
                int total_distance = 0;
                for (size_t k = i; k < j; ++k) {
                    total_distance += db_.GetDistance(bus->stops[k], bus->stops[k + 1]);
                }
                double total_travel_time = total_distance * 60.0 / (settings_.bus_velocity * 1000);
                graph::EdgeId id = graph_.AddEdge({ stop_vertex_[bus->stops[i]].end, stop_vertex_[bus->stops[j]].begin, total_travel_time});
                bus_edges_[id] = { bus->name, static_cast<int>(j - i), total_travel_time};
            }
        }
    }
}

std::pair<double, std::vector<std::variant<StopEdge, BusEdge>>> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    std::optional<graph::Router<double>::RouteInfo> route_info = router_.value().BuildRoute(stop_vertex_.at(from).begin, stop_vertex_.at(to).begin);
    if (!route_info.has_value()) {
        return {-1, {}};
    }
    std::vector<std::variant<StopEdge, BusEdge>> items;
    items.reserve(route_info.value().edges.size());
    for (graph::EdgeId edge_id : route_info.value().edges) {
        if (stop_edges_.count(edge_id)) {
            items.push_back(stop_edges_.at(edge_id));
        } else {
            items.push_back(bus_edges_.at(edge_id));
        }
    }
    return { route_info.value().weight, items };
}

} //namespace routing