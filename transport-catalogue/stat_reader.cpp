#include <iostream>
#include <iomanip>

#include "stat_reader.h"

using namespace std::literals;

void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    if (request[0] == 'B') {
        std::string bus_name(request.substr(4));
        auto [stops_count, unique_stops_count, route_length] = transport_catalogue.GetBusInfo(bus_name);
        if (!stops_count) {
            output << "Bus "s << bus_name << ": not found\n"s;
            return;
        }
        output << "Bus "s << bus_name << ": "s << stops_count << " stops on route, "s;
        output << unique_stops_count << " unique stops, "s << std::setprecision(6) << route_length << " route length\n"s;
    }
    else if (request[0] == 'S') {
        std::string_view stop_name(request.substr(5));
        auto stop = transport_catalogue.FindStop(stop_name);
        output << "Stop "s << stop_name << ": "s;
        if (stop == nullptr) {
            output << "not found\n"s;
            return;
        }
        auto buses_on_stop = transport_catalogue.GetBusesOnStop(stop_name);
        if (buses_on_stop.empty()) {
            output << "no buses"s;
        }
        else {
            output << "buses"s;
            for (const std::string_view bus_name : buses_on_stop) {
                output << ' ' << bus_name;
            }
        }
        output << '\n';
    }
}