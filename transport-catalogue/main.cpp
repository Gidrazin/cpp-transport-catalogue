#include <iostream>
#include <sstream>
#include <string>

#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"

int main() {
    json::JsonReader json_reader(std::cin); //Превращаем json из потока ввода в document_
    json_reader.MakeDB(); //Заполняем базу транспортного каталога
    renderer::MapRenderer map_renderer(json_reader.ParseRenderSettings()); //Применяем настройки отрисовки
    StatRequestHandler handler(json_reader.GetDB(), map_renderer); //Создаем обработчик запросов
    auto map = handler.RenderMap(); //Обработчик генерирует карту
    std::ostringstream map_output;
    map.Render(map_output); //Отрисовка карты и вывод в строковый поток
    json_reader.PrintJson(std::cout, map_output.str()); //Формирование выходного json документа с картой
}