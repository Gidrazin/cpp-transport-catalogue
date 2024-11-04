
#include "json_reader.h"

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


const TransportCatalogue& JsonReader::GetDB() {
    AddStops(document_["base_requests"s].AsArray());
    AddDistances(document_["base_requests"s].AsArray());
    AddBuses(document_["base_requests"s].AsArray());
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

void JsonReader::PrintJson(std::ostream& out, const std::string& map) const{
    json::Array result;
    for (const auto& request : document_.at("stat_requests"s).AsArray()) {
        if (request.AsMap().at("type"s).AsString() == "Bus"s ){
            auto [stops_count, unique_stops_count, route_length, curvature] = db_.GetBusInfo(request.AsMap().at("name"s).AsString());
            if (!stops_count) {
                result.emplace_back(json::Dict({{"request_id"s, request.AsMap().at("id"s).AsInt()}, {"error_message"s, "not found"s}}));
            } else {
                result.emplace_back(
                    json::Dict({
                        {"curvature"s, curvature},
                        {"request_id"s, request.AsMap().at("id"s).AsInt()},
                        {"route_length"s, route_length},
                        {"stop_count"s, static_cast<int>(stops_count)},
                        {"unique_stop_count"s, static_cast<int>(unique_stops_count)},
                    })
                );
            }
        } else if (request.AsMap().at("type"s).AsString() == "Stop"s ) {
            auto stop = db_.FindStop(request.AsMap().at("name"s).AsString());
            if (stop == nullptr){
                result.emplace_back(json::Dict({{"request_id"s, request.AsMap().at("id"s).AsInt()}, {"error_message"s, "not found"s}}));
                continue;
            }
            auto buses_on_stop = db_.GetBusesOnStop(request.AsMap().at("name"s).AsString());
            json::Array buses;
            for (auto bus : buses_on_stop){
                buses.emplace_back(std::string(bus));
            }
            result.emplace_back(
                json::Dict({
                    {"request_id"s, request.AsMap().at("id"s).AsInt()},
                    {"buses"s, buses}
                })
            );
        } else {
            result.emplace_back(
                json::Dict({
                    {"request_id"s, request.AsMap().at("id"s).AsInt()},
                    {"map"s, map}
                })
            );
        }
    }
    json::Document doc(result);
    json::Print(doc, out);
}

}