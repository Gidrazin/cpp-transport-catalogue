#pragma once

#include <memory>

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace json {

class JsonReader {
public:
    explicit JsonReader(std::istream& in);
    renderer::RenderSettings ParseRenderSettings() const;
    const TransportCatalogue& GetDB();
    void PrintJson(std::ostream& out, const std::string& map) const;

private:
    void AddStops(const Array& base_requests);
    void AddDistances(const Array& base_requests);
    void AddBuses(const Array& base_requests);
    
    TransportCatalogue db_;
    Dict document_;
};


}