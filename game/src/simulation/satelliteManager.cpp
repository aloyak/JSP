#include "simulation/satelliteManager.h"
#include "engine/utils/logger.h"

#include <sys/stat.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <ctime>

using json = nlohmann::json;

namespace {

    int tleChecksum(const std::string& line) {
        int total = 0;
        for (char c : line) {
            if (std::isdigit(c)) total += c - '0';
            else if (c == '-')   total += 1;
        }
        return total % 10;
    }

    // Format mean motion first derivative: " .NNNNNNNN" or "-.NNNNNNNN"
    std::string formatMmDot(double val) {
        char buf[16];
        char sign = (val >= 0.0) ? ' ' : '-';
        double absVal = std::abs(val);
        std::snprintf(buf, sizeof(buf), "%c.%08.0f", sign, absVal * 1e8);
        return std::string(buf);
    }

    // Format BSTAR drag term: " NNNNN-N" (packed decimal)
    std::string formatBstar(double val) {
        if (val == 0.0) return " 00000-0";
        char sign = (val >= 0.0) ? ' ' : '-';
        double absVal = std::abs(val);
        int exp = (int)std::floor(std::log10(absVal)) + 1;
        long mantissa = std::llround(absVal / std::pow(10.0, exp - 5));
        char expSign = (exp >= 0) ? '+' : '-';
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%c%05ld%c%d", sign, mantissa, expSign, std::abs(exp));
        return std::string(buf);
    }

    // Parse ISO epoch "2026-06-09T18:30:49.920768" -> (2-digit year, day-of-year with fraction)
    std::pair<int, double> parseEpoch(const std::string& epochStr) {
        int year, month, day, hour, minute;
        double second;
        std::sscanf(epochStr.c_str(), "%d-%d-%dT%d:%d:%lf",
                    &year, &month, &day, &hour, &minute, &second);

        static const int daysPerMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        int dayOfYear = day;
        for (int m = 0; m < month - 1; ++m) {
            dayOfYear += daysPerMonth[m];
            if (m == 1 && leap) dayOfYear++;
        }

        double fracDay = (hour * 3600.0 + minute * 60.0 + second) / 86400.0;
        return { year % 100, dayOfYear + fracDay };
    }

    // Parse OBJECT_ID "1998-067A" -> "98067A  " (8-char international designator)
    std::string formatIntlDesig(const std::string& objectId) {
        auto dash = objectId.find('-');
        if (dash == std::string::npos || objectId.size() < dash + 2)
            return "00000A  ";
        std::string yr   = objectId.substr(dash - 2, 2);
        std::string rest = objectId.substr(dash + 1);
        std::string result = yr + rest;
        result.resize(8, ' ');
        return result;
    }

    std::pair<std::string, std::string> generateTleStrings(const json& item) {
        unsigned int norad  = item.value("NORAD_CAT_ID", 0u);
        std::string  epoch  = item.value("EPOCH", "2000-01-01T00:00:00");
        double mmDot        = item.value("MEAN_MOTION_DOT", 0.0);
        double bstar        = item.value("BSTAR", 0.0);
        std::string objId   = item.value("OBJECT_ID", "00000A");
        unsigned int rev    = item.value("REV_AT_EPOCH", 0u);

        double inclination  = item.value("INCLINATION", 0.0);
        double raan         = item.value("RA_OF_ASC_NODE", 0.0);
        double eccentricity = item.value("ECCENTRICITY", 0.0);
        double argPerigee   = item.value("ARG_OF_PERICENTER", 0.0);
        double meanAnomaly  = item.value("MEAN_ANOMALY", 0.0);
        double meanMotion   = item.value("MEAN_MOTION", 0.0);

        auto [yr2, dayFrac] = parseEpoch(epoch);
        std::string intlDesig = formatIntlDesig(objId);
        std::string mmDotStr  = formatMmDot(mmDot);
        std::string bstarStr  = formatBstar(bstar);

        char eccBuf[16];
        std::snprintf(eccBuf, sizeof(eccBuf), "%07.0f", eccentricity * 1e7);

        // Line 1 
        char l1buf[80];
        std::snprintf(l1buf, sizeof(l1buf),
            "1 %05uU %-8s %02d%012.8f %s  00000-0 %s 0  999",
            norad, intlDesig.c_str(), yr2, dayFrac,
            mmDotStr.c_str(), bstarStr.c_str());
        std::string line1(l1buf, 68);
        line1 += std::to_string(tleChecksum(line1));

        // Line 2
        char l2buf[80];
        std::snprintf(l2buf, sizeof(l2buf),
            "2 %05u %8.4f %8.4f %s %8.4f %8.4f %11.8f%05u",
            norad, inclination, raan, eccBuf,
            argPerigee, meanAnomaly, meanMotion, rev);
        std::string line2(l2buf, 68);
        line2 += std::to_string(tleChecksum(line2));

        return { line1, line2 };
    }

} // namespace


void SatelliteManager::initialize() {
    Logger::info("Initializing SatelliteManager");
    if (isCacheValid()) {
        Logger::info("Cache is valid for a refresh attempt");
        if (!fetchSatelliteData()) {
            Logger::warn("Failed to fetch fresh data, attempting to load from cache");
            loadFromCache();
        }
    } else {
        Logger::info("Cache is fresh, loading from cache directly");
        loadFromCache();
    }
}

bool SatelliteManager::isCacheValid() {
    struct stat result;
    std::string resolvedCachePath = Path::resolve(m_cachePath).string();

    if (stat(resolvedCachePath.c_str(), &result) == 0) {
        auto currentTime = std::chrono::system_clock::now();
        auto fileTime    = std::chrono::system_clock::from_time_t(result.st_mtime);
        auto duration    = std::chrono::duration_cast<std::chrono::hours>(currentTime - fileTime);
        Logger::info("Cache file found, age in hours: {}", duration.count());
        return duration.count() >= 24;
    }
    Logger::warn("Cache file not found");
    return true;
}

bool SatelliteManager::fetchSatelliteData() {
    Logger::info("Attempting to fetch satellite data from celestrak.org");
    httplib::Client cli("https://celestrak.org");
    if (auto res = cli.Get("/NORAD/elements/gp.php?GROUP=active&FORMAT=json")) {
        Logger::info("HTTP request completed with status code: {}", res->status);
        if (res->status == 200) {
            std::string resolvedCachePath = Path::resolve(m_cachePath).string();
            std::filesystem::create_directories(
                std::filesystem::path(resolvedCachePath).parent_path());
            std::ofstream outFile(resolvedCachePath);
            if (outFile.is_open()) {
                outFile << res->body;
                outFile.close();
                Logger::info("Successfully saved fetched data to cache");
                parseJson(res->body);
                return true;
            } else {
                Logger::error("Failed to open cache file for writing");
            }
        }
    } else {
        Logger::error("HTTP GET request failed completely");
    }
    return false;
}

void SatelliteManager::loadFromCache() {
    Logger::info("Attempting to load satellite data from cache file");
    std::string resolvedCachePath = Path::resolve(m_cachePath).string();
    std::ifstream inFile(resolvedCachePath);
    if (inFile.is_open()) {
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        parseJson(buffer.str());
    } else {
        Logger::error("Failed to open cache file for reading");
    }
}

void SatelliteManager::parseJson(const std::string& jsonData) {
    Logger::info("Starting to parse JSON data");
    m_satellites.clear();
    try {
        auto jArr = json::parse(jsonData);
        Logger::info("JSON array parsed, item count: {}", jArr.size());
        for (const auto& item : jArr) {
            if (!item.contains("OBJECT_NAME") || !item.contains("NORAD_CAT_ID")) {
                continue;
            }

            Satellite sat;
            sat.name         = item["OBJECT_NAME"].get<std::string>();
            sat.noradID      = item.value("NORAD_CAT_ID", 0u);
            sat.bstar        = item.value("BSTAR", 0.0);
            sat.inclination  = item.value("INCLINATION", 0.0);
            sat.raan         = item.value("RA_OF_ASC_NODE", 0.0);
            sat.eccentricity = item.value("ECCENTRICITY", 0.0);
            sat.argOfPerigee = item.value("ARG_OF_PERICENTER", 0.0);
            sat.meanAnomaly  = item.value("MEAN_ANOMALY", 0.0);
            sat.meanMotion   = item.value("MEAN_MOTION", 0.0);

            auto [line1, line2] = generateTleStrings(item);
            sat.line1 = line1;
            sat.line2 = line2;

            m_satellites.push_back(sat);
        }
        Logger::info("Successfully loaded {} satellites into manager", m_satellites.size());
    } catch (const std::exception& e) {
        Logger::error("JSON parsing encountered an exception: {}", e.what());
    }
}