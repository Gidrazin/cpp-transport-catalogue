#pragma once

#include <string>
#include <vector>

#include "geo.h"


struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<std::string_view> stops;
	int route_length = 0;
	bool is_circle = false;
};

struct BusInfo {
	size_t stops_on_route = 0;
	size_t unique_stops = 0;
	int route_length = 0;
	double curvature = 0;
};