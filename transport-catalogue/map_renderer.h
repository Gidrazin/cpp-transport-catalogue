#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "geo.h"
#include "domain.h"
#include "svg.h"
#include "transport_catalogue.h"

using namespace std::literals;

namespace renderer {

struct RenderSettings {
    double width = 1200.0;
    double height = 1200.0;

    double padding = 50.0;

    double line_width = 14.0;
    double stop_radius = 5.0;

    int bus_label_font_size = 20;
    std::array<double, 2> bus_label_offset = { 7.0, 15.0 };

    int stop_label_font_size = 20;
    std::array<double, 2> stop_label_offset = { 7.0, -3.0 };

    svg::Color underlayer_color = "white"s;
    double underlayer_width = 3.0;

    std::vector<svg::Color> color_palette = {"red"s, "orange"s, "yellow"s, "green"s, "blue"s, "purple"s};
};

inline const double EPSILON = 1e-6;


inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    explicit MapRenderer(RenderSettings&&);
    std::vector<svg::Point> MakeCoords(const std::vector<geo::Coordinates>& coords, const SphereProjector& proj) const;
    void SetProj(const TransportCatalogue& db); //Создание переводчика координат
    void DrawLines(svg::Document& document, const TransportCatalogue& db) const;
    void DrawBusNames(svg::Document& document, const TransportCatalogue& db) const;
    void DrawCircles(svg::Document& document, const TransportCatalogue& db) const;
private:
    RenderSettings settings_;
    std::optional<SphereProjector> proj_;
};

} //namespace renderer
