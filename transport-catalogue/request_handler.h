#pragma once

#include <optional>
#include <unordered_set>

#include "json.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

class StatRequestHandler {
public:
    StatRequestHandler(const TransportCatalogue& db, renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusInfo> GetBusInfo(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::unordered_set<const Bus*> GetBusesByStop(const std::string_view& stop_name) const;

    svg::Document RenderMap() const;

    size_t GetVertexCount() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    renderer::MapRenderer& renderer_;
};
