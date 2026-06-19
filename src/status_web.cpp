/**
 * @file status_web.cpp
 * @brief Implementation of the StatusWeb class for local web interface
 */
#include "status_web.hpp"
#include "httplib.h"

#include <sstream>
#include <string>

void StatusWeb::update(double moisturePct, bool needsWater) {
	
	std::lock_guard<std::mutex> lock(mutex_);
	moisturePct_ = moisturePct;
	needsWater_ = needsWater;
}

void StatusWeb::start(int port) {
	
	httplib::Server server;

	// Generate status page when user visits "/"
	server.Get("/", [this](const httplib::Request&, httplib::Response& res) {
		
		double moisturePct;
		bool needsWater;

		// Copy the latest values under lock
		{
			std::lock_guard<std::mutex> lock(mutex_);
			moisturePct = moisturePct_;
			needsWater = needsWater_;
		}

		// Generate HTML page with current status
		std::ostringstream html;
		html << R"(
			<!DOCTYPE html>
			<html>
			<head>
			<meta charset="UTF-8">
			<title>Plant Soil Status</title>
			<meta http-equiv="refresh" content="3">

			<style>
			body { font-family: Arial; margin: 40px; background: #f5f7f2; }
			.card { max-width: 420px; background: white; padding: 24px; border-radius: 12px; }
			.healthy { color: green; font-weight: bold; }
			.warning { color: red; font-weight: bold; }
			</style>
			</head>

			<body>
			<div class="card">
			<h1>Plant Soil Status</h1>
			)";

		html << "<p><strong>Moisture Level:</strong> "
		     << static_cast<int>(moisturePct)
		     << "%</p>";

		html << "<p><strong>Status:</strong> <span class=\""
		     << (needsWater ? "warning" : "healthy")
		     << "\">"
             	     << (needsWater ? "Needs Attention" : "Healthy")
            	     << "</span></p>";

	        html << R"(
			<p><em>Page refreshes automatically every 3 seconds.</em></p>
		</div>
		</body>
		</html>
		)";

		res.set_content(html.str(), "text/html");
	});

	server.listen("0.0.0.0", port);
}

