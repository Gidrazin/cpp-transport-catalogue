#include <iostream>
#include <sstream>
#include <string>

#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_router.h"

int main() {
    json::JsonReader json_reader(std::cin); //Превращаем json из потока ввода в document_
    renderer::MapRenderer map_renderer(json_reader.ParseRenderSettings()); //Применяем настройки отрисовки
    StatRequestHandler handler(json_reader.MakeDB(), map_renderer); //Создаем обработчик запросов
    routing::RoutingSettings routing_settings(json_reader.ParseRoutingSettings()); //Парсинг настроек маршрутов
    routing::TransportRouter transport_router(json_reader.GetDB(), routing_settings, handler.GetVertexCount()); //Создание маршрутизатора
    auto map = handler.RenderMap(); //Обработчик генерирует карту
    std::ostringstream map_output;
    map.Render(map_output); //Отрисовка карты и вывод в строковый поток
    json_reader.PrintJson(std::cout, transport_router, map_output.str()); //Формирование выходного json документа с картой
}