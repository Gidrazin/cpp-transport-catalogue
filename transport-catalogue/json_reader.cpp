#include <functional>

#include "json_reader.h"
#include "json_builder.h"

svg::Color ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return svg::Color(node.AsString());
    } else {
        if (node.AsArray().size() == 3) {
            return svg::Color(svg::Rgb(
                static_cast<uint8_t>(node.AsArray()[0].AsInt()),
                static_cast<uint8_t>(node.AsArray()[1].AsInt()),
                static_cast<uint8_t>(node.AsArray()[2].AsInt())
            ));
        } else {
            return svg::Color(svg::Rgba(
                static_cast<uint8_t>(node.AsArray()[0].AsInt()),
                static_cast<uint8_t>(node.AsArray()[1].AsInt()),
                static_cast<uint8_t>(node.AsArray()[2].AsInt()),
                node.AsArray()[3].AsDouble()
            ));
        }
    }
}

std::vector<svg::Color> ParseColor(const json::Array& nodes) {
    std::vector<svg::Color> colors;
    for (const json::Node& node : nodes) {
        colors.emplace_back(ParseColor(node));
    }
    return colors;
}

namespace json {

using namespace std::literals;

JsonReader::JsonReader(std::istream& in)
: document_(Load(in).GetRoot().AsMap())
{}


const TransportCatalogue& JsonReader::MakeDB() {
    AddStops(document_["base_requests"s].AsArray());
    AddDistances(document_["base_requests"s].AsArray());
    AddBuses(document_["base_requests"s].AsArray());
    return db_;
}

const TransportCatalogue& JsonReader::GetDB() const {
    return db_;
}

void JsonReader::AddStops(const Array& base_requests){
    for (auto item : base_requests){
        auto item_as_map = item.AsMap();
        if (item_as_map.at("type"s).AsString() == "Stop"s){
            db_.AddStop({item_as_map.at("name"s).AsString(), {item_as_map.at("latitude"s).AsDouble(), item_as_map.at("longitude"s).AsDouble()}});
        }
    }
}

void JsonReader::AddDistances(const Array& base_requests){
    for (auto item : base_requests) {
        auto item_as_map = item.AsMap();
        if (item_as_map.at("type"s).AsString() == "Stop"s) {
            std::vector<std::pair<std::string_view, int>> distances;
            for (auto [name, distance] : item_as_map.at("road_distances"s).AsMap()) {
                distances.push_back({ db_.FindStop(name)->name, distance.AsInt()});
            }
            db_.AddDistance( item_as_map.at("name"s).AsString(), distances );
        }
    }
}

void JsonReader::AddBuses(const Array& base_requests){
    for (auto item : base_requests) {
        auto item_as_map = item.AsMap();
        if (item_as_map.at("type"s).AsString() == "Bus"s) {
            std::vector<std::string_view> stops;
            for (auto stop_name : item_as_map.at("stops"s).AsArray()) {
                stops.emplace_back(db_.FindStop(stop_name.AsString())->name);
            }
            if (!item_as_map.at("is_roundtrip"s).AsBool()) {
                std::vector<std::string_view> results(stops.begin(), stops.end());
                results.insert(results.end(), std::next(stops.rbegin()), stops.rend());
                stops = std::move(results);
            }
            db_.AddBus({ item_as_map.at("name"s).AsString(), stops , 0,  item_as_map.at("is_roundtrip"s).AsBool() });
        }
    }
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    renderer::RenderSettings settings;
    const Dict render_settings = document_.at("render_settings"s).AsMap();
    settings.width = render_settings.at("width"s).AsDouble();
    settings.height = render_settings.at("height"s).AsDouble();
    settings.padding = render_settings.at("padding"s).AsDouble();
    settings.line_width = render_settings.at("line_width"s).AsDouble();
    settings.stop_radius = render_settings.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
    settings.bus_label_offset =  {
        render_settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
        render_settings.at("bus_label_offset"s).AsArray()[1].AsDouble()
    };
    settings.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
    settings.stop_label_offset = {
        render_settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
        render_settings.at("stop_label_offset"s).AsArray()[1].AsDouble()
    };
    settings.underlayer_color = ParseColor(render_settings.at("underlayer_color"));
    settings.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
    settings.color_palette = ParseColor(render_settings.at("color_palette"s).AsArray());
    return settings;
}

routing::RoutingSettings JsonReader::ParseRoutingSettings() const {
    routing::RoutingSettings settings;
    const Dict routing_settings = document_.at("routing_settings"s).AsMap();
    settings.bus_wait_time = routing_settings.at("bus_wait_time").AsInt();
    settings.bus_velocity = routing_settings.at("bus_velocity"s).AsInt();
    return settings;
}


//Структура которая хранит ссылки на всё необходимое для отрисовки выходного JSON файла
struct PrintJsonSource {
    json::Builder& result;
    const TransportCatalogue& db;
    const json::Node& request;
    const std::string& map;
    const routing::TransportRouter& transport_router;
};

void StatRequestBus(PrintJsonSource source) {
    auto [stops_count, unique_stops_count, route_length, curvature] = source.db.GetBusInfo(source.request.AsMap().at("name"s).AsString());
    if (!stops_count) {
        source.result.StartDict().
            Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
            Key("error_message"s).Value("not found"s).
            EndDict();
    }
    else {
        source.result.StartDict().
            Key("curvature"s).Value(curvature).
            Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
            Key("route_length"s).Value(route_length).
            Key("stop_count"s).Value(static_cast<int>(stops_count)).
            Key("unique_stop_count"s).Value(static_cast<int>(unique_stops_count)).
            EndDict();
    }
}

void StatRequestStop(PrintJsonSource source) {
    auto stop = source.db.FindStop(source.request.AsMap().at("name"s).AsString());
    if (stop == nullptr) {
        source.result.StartDict().
            Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
            Key("error_message"s).Value("not found"s).
            EndDict();
        return;
    }
    auto buses_on_stop = source.db.GetBusesOnStop(source.request.AsMap().at("name"s).AsString());
    json::Array buses;
    for (auto bus : buses_on_stop) {
        buses.emplace_back(std::string(bus));
    }
    source.result.StartDict().
        Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
        Key("buses"s).Value(buses).
        EndDict();
}

void StatRequestRoute(PrintJsonSource source){
    std::string_view from = source.request.AsMap().at("from"s).AsString();
    std::string_view to = source.request.AsMap().at("to"s).AsString();
    std::pair<double, std::vector<std::variant<routing::StopEdge, routing::BusEdge>>> info =
        source.transport_router.BuildRoute(from, to);
    if (info.first == -1) { // если маршрута между указанными остановками нет
        source.result.StartDict().
            Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
            Key("error_message"s).Value("not found"s).
            EndDict();
        return;
    }
    source.result.StartDict().
        Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
        Key("total_time"s).Value(info.first).
        Key("items"s).StartArray();

    for (const auto& item : info.second){
        source.result.StartDict();
        if (std::holds_alternative<routing::StopEdge>(item)){
            source.result.
            Key("type"s).Value("Wait"s).
            Key("stop_name"s).Value(std::string(std::get<routing::StopEdge>(item).stop_name)).
            Key("time"s).Value(std::get<routing::StopEdge>(item).time);
        } else {
            source.result.
            Key("type"s).Value("Bus"s).
            Key("bus"s).Value(std::string(std::get<routing::BusEdge>(item).bus)).
            Key("span_count"s).Value(std::get<routing::BusEdge>(item).span_count).
            Key("time"s).Value(std::get<routing::BusEdge>(item).time);
        }
        source.result.EndDict();
    }
    source.result.EndArray();
    source.result.EndDict();
}

void StatRequestMap(PrintJsonSource source){
    source.result.StartDict().
    Key("request_id"s).Value(source.request.AsMap().at("id"s).AsInt()).
    Key("map"s).Value(source.map).
    EndDict();
}

void JsonReader::PrintJson(std::ostream& out, const routing::TransportRouter& transport_router, const std::string map) const{
    json::Builder result;
    result.StartArray();
    std::unordered_map<std::string, std::function<void(PrintJsonSource)>> request_types;
    request_types["Bus"s] = StatRequestBus;
    request_types["Stop"s] = StatRequestStop;
    request_types["Route"s] = StatRequestRoute;
    request_types["Map"s] = StatRequestMap;
    for (const auto& request : document_.at("stat_requests"s).AsArray()) {
        request_types[request.AsMap().at("type"s).AsString()]({result, db_, request, map, transport_router});
    }
    result.EndArray();
    json::Document doc(result.Build());
    json::Print(doc, out);
}

}