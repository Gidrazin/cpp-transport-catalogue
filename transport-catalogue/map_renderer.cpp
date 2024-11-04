#include "map_renderer.h"

namespace renderer {

//-------------------SphereProjector-----------------------

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

//-------------------MapRenderer---------------------------

MapRenderer::MapRenderer(RenderSettings&& settings)
:settings_(std::move(settings))
{}

std::vector<svg::Point> MapRenderer::MakeCoords(const std::vector<geo::Coordinates>& coords, const SphereProjector& proj) const{
    std::vector<svg::Point> result;
    // Проецируем и выводим координаты
    for (const auto &geo_coord: coords) {
        result.emplace_back(proj(geo_coord));
    }
    return result;
}

void MapRenderer::SetProj(const TransportCatalogue& db){
    //Создание переводчика координат
    std::vector<geo::Coordinates> all_coords;
    for (auto [bus_name, bus_ptr] : db.GetBuses()) {
        if (!bus_ptr->stops.empty()) {
            for (std::string_view stop_name : bus_ptr->stops) {
                all_coords.emplace_back(db.FindStop(stop_name)->coordinates);
            }
        }
    }
    proj_ = {
        all_coords.begin(), all_coords.end(), settings_.width, settings_.height, settings_.padding
    };
}

void MapRenderer::DrawLines(svg::Document& document, const TransportCatalogue& db) const {
    size_t colors_num = settings_.color_palette.size();
    size_t curr_color = 0;

    //Отрисовка Линий
    for (auto [bus_name, bus_ptr] : db.GetBuses()){
        if (!bus_ptr->stops.empty()){
            std::vector<geo::Coordinates> coords;
            for (std::string_view stop_name : bus_ptr->stops){
                coords.emplace_back(db.FindStop(stop_name)->coordinates);
            }
            std::vector<svg::Point> projected_coords = MakeCoords(coords, proj_.value());
            svg::Polyline route;
            route.SetStrokeColor(settings_.color_palette[curr_color % colors_num]);
            route.SetFillColor(svg::NoneColor);
            route.SetStrokeWidth(settings_.line_width);
            route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            for (const svg::Point& point : projected_coords){
                route.AddPoint(point);
            }
            document.Add(std::move(route));
            ++curr_color;
        }
    }
}

void MapRenderer::DrawBusNames(svg::Document& document, const TransportCatalogue& db) const {
    size_t colors_num = settings_.color_palette.size();
    size_t curr_color = 0;

        //Отрисовка названий маршрутов
    std::vector<svg::Text> bus_names;
    curr_color = 0;
    for (auto [bus_name, bus_ptr] : db.GetBuses()){
        if (!bus_ptr->stops.empty()){
            //координаты первой остановки
            geo::Coordinates coords_start = (db.FindStop(bus_ptr->stops[0]))->coordinates;
            geo::Coordinates coords_end = coords_start;
            if (!bus_ptr->is_circle){
                coords_end = (db.FindStop(bus_ptr->stops[bus_ptr->stops.size() / 2]))->coordinates;
            }

            std::vector<svg::Point> projected_coords = MakeCoords({coords_start, coords_end}, proj_.value());
            //Название начала с подложкой
            svg::Text route_name;
            route_name.SetPosition(projected_coords[0]).
            SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]}).
            SetFontSize(settings_.bus_label_font_size).
            SetFontFamily("Verdana"s).
            SetFontWeight("bold"s).
            SetData(std::string(bus_name)).SetFillColor(settings_.color_palette[curr_color % colors_num]);
            
            svg::Text route_underlayer = route_name;
            route_underlayer.SetFillColor(settings_.underlayer_color).
            SetStrokeColor(settings_.underlayer_color).
            SetStrokeWidth(settings_.underlayer_width).
            SetStrokeLineCap(svg::StrokeLineCap::ROUND).
            SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            bus_names.push_back(route_underlayer);
            bus_names.push_back(route_name);

            //Добавляем название к конечной при некольцевом маршруте
            if (!bus_ptr->is_circle && coords_start != coords_end){
                route_name.SetPosition(projected_coords[1]);
                route_underlayer.SetPosition(projected_coords[1]);
                bus_names.push_back(route_underlayer);
                bus_names.push_back(route_name);
            }
            ++curr_color;
        }
    }

    for (auto bus_name : bus_names){
        document.Add(std::move(bus_name));
    }
}

void MapRenderer::DrawCircles(svg::Document& document, const TransportCatalogue& db) const {
    std::set<std::string_view> all_stops;
    for (auto [bus_name, bus_ptr] : db.GetBuses()) {
        if (!bus_ptr->stops.empty()) {
            for (std::string_view stop_name : bus_ptr->stops) {
                all_stops.insert(stop_name); // здесь O(N) log N ! Но сортировать всё равно нужно
            }
        }
    }
    //Отрисовка символов остановки и названий
    std::vector<svg::Circle> circles;
    std::vector<svg::Text> stop_names;
    for (const std::string_view stop_name : all_stops){
        auto coords = MakeCoords({db.FindStop(stop_name)->coordinates}, proj_.value());

        svg::Circle stop;
        stop.SetCenter(coords[0]).SetRadius(settings_.stop_radius).SetFillColor("white"s);
        circles.push_back(std::move(stop));

        svg::Text stop_name_text;

        stop_name_text.SetPosition(coords[0]).
        SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]}).
        SetFontSize(settings_.stop_label_font_size).
        SetFontFamily("Verdana"s).
        SetData(std::string(stop_name)).SetFillColor("black"s);
        svg::Text stop_name_underlayer = stop_name_text;
        stop_name_underlayer.SetFillColor(settings_.underlayer_color).
        SetStrokeColor(settings_.underlayer_color).
        SetStrokeWidth(settings_.underlayer_width).
        SetStrokeLineCap(svg::StrokeLineCap::ROUND).
        SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        stop_names.push_back(std::move(stop_name_underlayer));
        stop_names.push_back(std::move(stop_name_text));
    }

    for (auto circle : circles){
        document.Add(std::move(circle));
    }

    for (auto stop_name : stop_names){
        document.Add(std::move(stop_name));
    }
}

} //namespace renderer