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

const RenderSettings& MapRenderer::GetSettings() const{
    return settings_;
}

} //namespace renderer