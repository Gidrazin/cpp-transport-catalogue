#include "transport_catalogue.h"
#include "geo.h"

#include <stdexcept>

void TransportCatalogue::AddStop(const std::string& stop_name, const double latitude, const double longitude) {
	stops_index_list_.insert(stop_name);
	stops_[stops_index_list_.find(stop_name)->data()] = {stops_index_list_.find(stop_name)->data(), latitude, longitude, {} };
}

void TransportCatalogue::AddBus(const std::string& bus_name, const std::vector<std::string>& stop_names) {
	buses_index_list_.insert(bus_name);
	std::vector<std::string_view> sv_stop_names;
	
	for (const std::string& stop_name : stop_names) {
		sv_stop_names.push_back(stops_index_list_.find(stop_name)->data());
		stops_[stop_name].buses.insert(buses_index_list_.find(bus_name)->data());
	}

	double route_length = ComputeRouteDistance(sv_stop_names);
	buses_[buses_index_list_.find(bus_name)->data()] = {buses_index_list_.find(bus_name)->data(), move(sv_stop_names), route_length};
}

std::tuple<size_t, size_t, double> TransportCatalogue::GetBusInfo(const std::string& bus_name) const {
	if (!buses_index_list_.count(bus_name)) {
		return {0, 0, 0};
	}
	Bus bus = buses_.at(buses_index_list_.find(bus_name)->data());
	size_t uniq_stops_count = std::unordered_set(bus.stops.begin(), bus.stops.end()).size();
	
	return {bus.stops.size(), uniq_stops_count, bus.route_length};
}

double TransportCatalogue::ComputeRouteDistance(const std::vector<std::string_view>& stops_on_route) {
	double route_length = 0;
	double length_between_two_stops = 0;

	for (size_t pos = 1; pos < stops_on_route.size(); ++pos) {
		try {
			length_between_two_stops = route_lengths_.at({ stops_on_route[pos - 1], stops_on_route[pos] });
		}
		catch (...) {
			Coordinates stop_1 = { stops_.at(stops_on_route[pos - 1]).latitude, stops_.at(stops_on_route[pos - 1]).longitude };
			Coordinates stop_2 = { stops_.at(stops_on_route[pos]).latitude, stops_.at(stops_on_route[pos]).longitude };
			length_between_two_stops = ComputeDistance(stop_1, stop_2);
			route_lengths_[{ stops_on_route[pos - 1], stops_on_route[pos] }] = length_between_two_stops;
		}
		route_length += length_between_two_stops;
	}

	return route_length;
}

std::set<std::string_view> TransportCatalogue::GetStopInfo(const Stop& stop) const {
	return stop.buses;
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(const std::string& stop_name) const {
	try {
		return &stops_.at(stop_name);
	}
	 catch (...) {
		return nullptr;
	}
}