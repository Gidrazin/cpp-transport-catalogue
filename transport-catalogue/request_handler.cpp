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

    renderer_.SetProj(db_); //Создаем переводчик координат
    renderer_.DrawLines(document, db_); //Заполняем линии
    renderer_.DrawBusNames(document, db_); //Названия маршрутов
    renderer_.DrawCircles(document, db_);//Отрисовка символов остановки и названий

    return document;
}

std::optional<BusInfo> StatRequestHandler::GetBusInfo(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

const std::unordered_set<const Bus*> StatRequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    std::unordered_set<const Bus*> result;
    auto buses = db_.GetBuses();
    for (auto bus_name : db_.GetBusesOnStop(stop_name)){
        result.insert(buses.at(bus_name));
    }
    return result;
}

size_t StatRequestHandler::GetVertexCount() const{
    return std::distance(db_.GetStopsList().begin(), db_.GetStopsList().end()) * 2;
}