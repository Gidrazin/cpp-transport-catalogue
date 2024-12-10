#pragma once

#include <memory>

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace json {

class JsonReader {
public:
    explicit JsonReader(std::istream& in);
    renderer::RenderSettings ParseRenderSettings() const;
    routing::RoutingSettings ParseRoutingSettings() const;
    const TransportCatalogue& MakeDB();
    const TransportCatalogue& GetDB() const;
    void PrintJson(std::ostream& out, const routing::TransportRouter& transport_router, std::string map) const;

private:
    void AddStops(const Array& base_requests);
    void AddDistances(const Array& base_requests);
    void AddBuses(const Array& base_requests);

    TransportCatalogue db_;
    Dict document_;
};


}