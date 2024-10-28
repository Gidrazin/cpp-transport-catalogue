#include "json_reader.h"
#include "request_handler.h"

using namespace std::literals;


//-------------STAT_REQUEST_HANDLER------------------


StatRequestHandler::StatRequestHandler(const TransportCatalogue& db, renderer::MapRenderer& renderer)
: db_(db)
, renderer_(renderer)
{}

svg::Document StatRequestHandler::RenderMap() const {
    svg::Document document;
    auto settings = renderer_.GetSettings();
    std::set<std::string_view> all_stops; // для отрисовки символов остановок

    //Создание переводчика координат
    std::vector<geo::Coordinates> all_coords;
    for (auto [bus_name, bus_ptr] : db_.GetBuses()) {
        if (!bus_ptr->stops.empty()) {
            for (std::string_view stop_name : bus_ptr->stops) {
                all_coords.emplace_back(db_.FindStop(stop_name)->coordinates);
                all_stops.insert(stop_name); // здесь O(N) log N ! Но сортировать всё равно нужно
            }
        }
    }
    renderer::SphereProjector proj{
        all_coords.begin(), all_coords.end(), settings.width, settings.height, settings.padding
    };
    

    size_t colors_num = settings.color_palette.size();
    size_t curr_color = 0;

    //Отрисовка Линий
    for (auto [bus_name, bus_ptr] : db_.GetBuses()){
        if (!bus_ptr->stops.empty()){
            std::vector<geo::Coordinates> coords;
            for (std::string_view stop_name : bus_ptr->stops){
                coords.emplace_back(db_.FindStop(stop_name)->coordinates);
            }
            std::vector<svg::Point> projected_coords = renderer_.MakeCoords(coords, proj);
            svg::Polyline route;
            route.SetStrokeColor(settings.color_palette[curr_color % colors_num]);
            route.SetFillColor(svg::NoneColor);
            route.SetStrokeWidth(settings.line_width);
            route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            for (const svg::Point& point : projected_coords){
                route.AddPoint(point);
            }
            document.Add(std::move(route));
            ++curr_color;
        }
    }

    //Отрисовка названий маршрутов
    std::vector<svg::Text> bus_names;
    curr_color = 0;
    for (auto [bus_name, bus_ptr] : db_.GetBuses()){
        if (!bus_ptr->stops.empty()){
            //координаты первой остановки
            geo::Coordinates coords_start = (db_.FindStop(bus_ptr->stops[0]))->coordinates;
            geo::Coordinates coords_end = coords_start;
            if (!bus_ptr->is_circle){
                coords_end = (db_.FindStop(bus_ptr->stops[bus_ptr->stops.size() / 2]))->coordinates;
            }

            std::vector<svg::Point> projected_coords = renderer_.MakeCoords({coords_start, coords_end}, proj);
            //Название начала с подложкой
            svg::Text route_name;
            route_name.SetPosition(projected_coords[0]).
            SetOffset({settings.bus_label_offset[0], settings.bus_label_offset[1]}).
            SetFontSize(settings.bus_label_font_size).
            SetFontFamily("Verdana"s).
            SetFontWeight("bold"s).
            SetData(std::string(bus_name)).SetFillColor(settings.color_palette[curr_color % colors_num]);
            
            svg::Text route_underlayer = route_name;
            route_underlayer.SetFillColor(settings.underlayer_color).
            SetStrokeColor(settings.underlayer_color).
            SetStrokeWidth(settings.underlayer_width).
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

    //Отрисовка символов остановки и названий
    std::vector<svg::Circle> circles;
    std::vector<svg::Text> stop_names;
    for (const std::string_view stop_name : all_stops){
        auto coords = renderer_.MakeCoords({db_.FindStop(stop_name)->coordinates}, proj);

        svg::Circle stop;
        stop.SetCenter(coords[0]).SetRadius(settings.stop_radius).SetFillColor("white"s);
        circles.push_back(std::move(stop));

        svg::Text stop_name_text;

        stop_name_text.SetPosition(coords[0]).
        SetOffset({settings.stop_label_offset[0], settings.stop_label_offset[1]}).
        SetFontSize(settings.stop_label_font_size).
        SetFontFamily("Verdana"s).
        SetData(std::string(stop_name)).SetFillColor("black"s);
        svg::Text stop_name_underlayer = stop_name_text;
        stop_name_underlayer.SetFillColor(settings.underlayer_color).
        SetStrokeColor(settings.underlayer_color).
        SetStrokeWidth(settings.underlayer_width).
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

    return document;
}