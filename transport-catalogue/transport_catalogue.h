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

	//Хэшер пары string_view через хэш конкатенации string
	struct PairHash {
		std::size_t operator()(std::pair<std::string_view, std::string_view> pair) const {
			return std::hash<std::string>()(std::string(pair.first) + std::string(pair.second));
		}
	};

	struct PairEqual {
		std::size_t operator()(
			std::pair<std::string_view, std::string_view> lhs,
			std::pair<std::string_view, std::string_view> rhs) const {
			return lhs == rhs;
		}
	};

	struct Stop{
		std::string_view name;
		geo::Coordinates coordinates;
		std::set<std::string_view> buses;
	};

	struct Bus {
		std::string_view name;
		std::vector<std::string_view> stops;
		double route_length = 0;
	};

public:
	void AddStop(const Stop& stop);
	void AddBus(const Bus& bus);
	std::tuple<size_t, size_t, double> GetBusInfo(const std::string_view& bus_name) const;
	const Stop* FindStop(const std::string_view& stop_name) const;

private:
	//Сохраняем сет индексов маршрутов и автобусов.
	//Далее будем использовать легковесные string_view
	//в качестве ключей к хеш таблицам
	std::unordered_set<std::string> stops_index_list_;
	std::unordered_set<std::string> buses_index_list_;
	std::unordered_map<std::string_view, Stop> stops_;
	std::unordered_map<std::string_view, Bus> buses_;
	//Кэш расстояний между остановками
	std::unordered_map<std::pair<std::string_view, std::string_view>, double, PairHash, PairEqual> route_lengths_;

	//Функция для вычисления длины маршрута. Вызывается при добавлении автобуса.
	double ComputeRouteDistance(const std::vector<std::string_view>& stops_on_route);
};