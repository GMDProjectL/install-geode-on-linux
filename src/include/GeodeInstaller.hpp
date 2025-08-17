#pragma once

#include "SteamGameFinder.hpp"
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class GeodeInstaller {
public:
    GeodeInstaller();
    
    std::string get_latest_geode_tag() const;
    
    std::string get_download_url() const;
    
    void unzip_to_destination(const std::string& zip_url, const fs::path& destination_dir) const;
    
    void install_to_dir(const fs::path& destination_dir) const;
    
    void patch_prefix_registry(const fs::path& reg_file_path) const;
    
    void install_geode_to_wine(const fs::path& prefix, const fs::path& gd_path) const;
    
    void install_geode_to_steam() const;

private:
    SteamGameFinder finder_;
    
    std::string make_http_request(const std::string& url) const;
    
    void download_file(const std::string& url, const fs::path& output_path) const;
    
    void extract_zip(const fs::path& zip_path, const fs::path& destination) const;
    
    std::string get_current_timestamp() const;
    
    std::string get_hex_timestamp() const;
};