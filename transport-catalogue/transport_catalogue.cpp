#include "transport_catalogue.h"
#include "geo.h"

#include <stdexcept>


std::size_t TransportCatalogue::PairHash::operator()(
	std::pair<std::string_view, std::string_view> pair) const {
	return std::hash<std::string>()(std::string(pair.first) + std::string(pair.second));
}

std::size_t TransportCatalogue::PairEqual::operator()(
	std::pair<std::string_view, std::string_view> lhs,
	std::pair<std::string_view, std::string_view> rhs) const {
	return lhs == rhs;
}

void TransportCatalogue::AddStop(const Stop& stop) {
	stops_index_list_.push_front(stop);
	stops_[stops_index_list_.front().name] = &stops_index_list_.front();
}

void TransportCatalogue::AddBus(const Bus& bus) {
	std::vector<std::string_view> sv_stop_names;
	
	for (const std::string_view stop_name : bus.stops) {
		sv_stop_names.push_back(stops_[stop_name]->name);
	}
	int route_length = ComputeRouteDistance(sv_stop_names);
	buses_index_list_.push_front({bus.name, move(sv_stop_names), route_length});
	buses_[buses_index_list_.front().name] = &buses_index_list_.front();

	for (const std::string_view stop_name : bus.stops) {
		buses_on_stop_[stops_[stop_name]->name].insert(buses_index_list_.front().name);
	}
}

void TransportCatalogue::AddDistance (std::string_view stop_name, const std::vector<std::pair<std::string_view, int>>& stop_names){
	if (stop_names.empty()){
		return;
	}
	auto it = stops_.find(stop_name);
	if (it == stops_.end()){
		return;
	}
	std::string_view stop_name_at = it->second->name;
	for (auto elem: stop_names){
		std::string_view stop_name_to = stops_[elem.first]->name;
		route_lengths_[{stop_name_at, stop_name_to}] = elem.second;
		route_lengths_by_coords_[{stop_name_at, stop_name_to}] = geo::ComputeDistance(
																stops_[stop_name_at]->coordinates,
																stops_[stop_name_to]->coordinates);
	}
}

int TransportCatalogue::GetDistance(std::string_view from_stop_name, std::string_view to_stop_name) const {
	auto it = route_lengths_.find({from_stop_name, to_stop_name});
	if (it == route_lengths_.end()){
		it = route_lengths_.find({to_stop_name, from_stop_name});
		if (it == route_lengths_.end()) {
			return 0;
		}
	}
	return it->second;
}

TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(const std::string_view bus_name) const {
	Bus bus;
	try {
		bus = *buses_.at(bus_name);
	} catch (...){
		return {0, 0, 0};
	}

	double curvature = bus.route_length / ComputeRouteDistanceByCoords(bus.stops);
	
	size_t uniq_stops_count = std::unordered_set(bus.stops.begin(), bus.stops.end()).size();
	
	return {bus.stops.size(), uniq_stops_count, bus.route_length, curvature};
}

double TransportCatalogue::ComputeRouteDistanceByCoords(const std::vector<std::string_view>& stops_on_route) const {
	double route_length = 0;
	for (size_t pos = 1; pos < stops_on_route.size(); ++pos) {
		try {
			route_length += route_lengths_by_coords_.at({ stops_on_route[pos - 1], stops_on_route[pos] });
		}
		catch (...) {
			route_length += route_lengths_by_coords_.at({ stops_on_route[pos], stops_on_route[pos - 1] });
		}
	}

	return route_length;
}

int TransportCatalogue::ComputeRouteDistance(const std::vector<std::string_view>& stops_on_route) const{
	int route_length = 0;
	for (size_t pos = 1; pos < stops_on_route.size(); ++pos) {
		route_length += GetDistance(stops_on_route[pos - 1], stops_on_route[pos]);
	}
	return route_length;
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(const std::string_view stop_name) const {
	try {
		return stops_.at(stop_name);
	}
	catch (...) {
		return nullptr;
	}
}

std::set<std::string_view> TransportCatalogue::GetBusesOnStop(const std::string_view stop_name) const {
	try {
		return buses_on_stop_.at(stop_name);
	}
	catch (...){
		return {};
	}
}