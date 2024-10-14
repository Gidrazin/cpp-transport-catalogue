#pragma once

#include <forward_list>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

class TransportCatalogue {

	struct PairHash {
		std::size_t operator()(
			std::pair<std::string_view, std::string_view> pair) const;
	};

	struct PairEqual {
		std::size_t operator()(
			std::pair<std::string_view, std::string_view> lhs,
			std::pair<std::string_view, std::string_view> rhs) const;
	};

	struct Stop{
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<std::string_view> stops;
		int route_length = 0;
	};

	struct BusInfo{
		size_t stops_on_route = 0;
		size_t unique_stops = 0;
		int route_length = 0;
		double curvature = 0;
	};

public:
	void AddStop(const Stop& stop);
	void AddBus(const Bus& bus);
	void AddDistance(std::string_view stop_name, const std::vector<std::pair<std::string_view, int>>& stop_names);
	int GetDistance(std::string_view from_stop_name, std::string_view to_stop_name) const;
	BusInfo GetBusInfo(const std::string_view bus_name) const;
	const Stop* FindStop(const std::string_view stop_name) const;
	std::set<std::string_view> GetBusesOnStop(const std::string_view stop_name) const;

private:

	std::forward_list<Stop> stops_index_list_;
	std::forward_list<Bus> buses_index_list_;
	std::unordered_map<std::string_view, const Stop*> stops_;
	std::unordered_map<std::string_view, const Bus*> buses_;
	std::unordered_map<std::string_view, std::set<std::string_view>> buses_on_stop_;
	//Кэш расстояний между остановками
	std::unordered_map<std::pair<std::string_view, std::string_view>, double, PairHash, PairEqual> route_lengths_by_coords_;
	std::unordered_map<std::pair<std::string_view, std::string_view>, int, PairHash, PairEqual> route_lengths_;

	//Функция для вычисления длины маршрута. Вызывается при добавлении автобуса.
	double ComputeRouteDistanceByCoords(const std::vector<std::string_view>& stops_on_route) const;
	int ComputeRouteDistance(const std::vector<std::string_view>& stops_on_route) const;
};