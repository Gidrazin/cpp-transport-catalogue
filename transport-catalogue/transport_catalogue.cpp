#include "transport_catalogue.h"
#include "geo.h"

#include <stdexcept>

void TransportCatalogue::AddStop(const Stop& stop) {
	auto iter = stops_index_list_.insert(std::string(stop.name));
	stops_[iter.first->data()] = { iter.first->data(), stop.coordinates, stop.buses };
}

void TransportCatalogue::AddBus(const Bus& bus) {
	auto iter = buses_index_list_.insert(std::string(bus.name));
	std::vector<std::string_view> sv_stop_names;
	
	for (const std::string_view& stop_name : bus.stops) {
		sv_stop_names.push_back(stops_index_list_.find(std::string(stop_name))->data());
		stops_[stop_name].buses.insert(iter.first->data());
	}

	double route_length = ComputeRouteDistance(sv_stop_names);
	buses_[iter.first->data()] = {iter.first->data(), move(sv_stop_names), route_length};
}

std::tuple<size_t, size_t, double> TransportCatalogue::GetBusInfo(const std::string_view& bus_name) const {
	if (!buses_.count(bus_name)) {
		return {0, 0, 0};
	}
	Bus bus = buses_.at(bus_name);
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
			length_between_two_stops = geo::ComputeDistance(
				stops_.at(stops_on_route[pos - 1]).coordinates,
				stops_.at(stops_on_route[pos]).coordinates);
			route_lengths_[{ stops_on_route[pos - 1], stops_on_route[pos] }] = length_between_two_stops;
		}
		route_length += length_between_two_stops;
	}

	return route_length;
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(const std::string_view& stop_name) const {
	try {
		return &stops_.at(stop_name);
	}
	 catch (...) {
		return nullptr;
	}
}