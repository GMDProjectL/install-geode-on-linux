#include "SteamGameFinder.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>

SteamGameFinder::SteamGameFinder() {
    steam_root_ = find_steam_root();
    library_folders_ = initialize_library_folders();
}

std::optional<fs::path> SteamGameFinder::find_steam_root() const {
    std::vector<fs::path> possible_paths = {
        fs::path(getenv("HOME")) / ".steam" / "steam",
        fs::path(getenv("HOME")) / ".steam" / "root", 
        fs::path(getenv("HOME")) / ".local" / "share" / "Steam",
        fs::path("/usr/share/steam")
    };
    
    for (const auto& path : possible_paths) {
        if (fs::exists(path) && fs::exists(path / "steamapps")) {
            return path;
        }
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::string> SteamGameFinder::parse_vdf_file(const fs::path& file_path) const {
    std::unordered_map<std::string, std::string> result;
    
    if (!fs::exists(file_path)) {
        return result;
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    size_t pos = 0;
    parse_vdf_recursive(content, pos, result);
    
    return result;
}

void SteamGameFinder::parse_vdf_recursive(const std::string& content, size_t& pos, 
                                         std::unordered_map<std::string, std::string>& result, 
                                         const std::string& prefix) const {
    while (pos < content.length()) {
        while (pos < content.length() && (std::isspace(content[pos]) || content[pos] == '\n' || content[pos] == '\r')) {
            pos++;
        }
        
        if (pos >= content.length()) break;
        
        if (pos + 1 < content.length() && content.substr(pos, 2) == "//") {
            while (pos < content.length() && content[pos] != '\n') {
                pos++;
            }
            continue;
        }
        
        if (content[pos] == '}') {
            pos++;
            return;
        }
        
        if (content[pos] == '{') {
            pos++;
            continue;
        }
        
        if (content[pos] == '"') {
            pos++;
            
            std::string key;
            while (pos < content.length() && content[pos] != '"') {
                key += content[pos];
                pos++;
            }
            pos++;
            
            while (pos < content.length() && std::isspace(content[pos])) {
                pos++;
            }
            
            if (pos < content.length() && content[pos] == '"') {
                pos++;
                std::string value;
                while (pos < content.length() && content[pos] != '"') {
                    value += content[pos];
                    pos++;
                }
                pos++;
                
                std::string full_key = prefix.empty() ? key : prefix + "." + key;
                result[full_key] = value;
            } else if (pos < content.length() && content[pos] == '{') {
                pos++;
                std::string new_prefix = prefix.empty() ? key : prefix + "." + key;
                parse_vdf_recursive(content, pos, result, new_prefix);
            }
        } else {
            pos++;
        }
    }
}

std::vector<fs::path> SteamGameFinder::initialize_library_folders() const {
    if (!steam_root_) {
        return {};
    }
    
    std::vector<fs::path> folders = { *steam_root_ / "steamapps" };
    
    fs::path library_file = *steam_root_ / "steamapps" / "libraryfolders.vdf";
    if (fs::exists(library_file)) {
        auto data = parse_vdf_file(library_file);
        
        for (const auto& [key, value] : data) {
            if (key.find("libraryfolders.") == 0 && key.find(".path") != std::string::npos) {
                fs::path path = fs::path(value) / "steamapps";
                if (fs::exists(path)) {
                    folders.push_back(path);
                }
            } else if (key.find(".path") != std::string::npos) {

                // legacy format

                fs::path path = fs::path(value) / "steamapps";
                if (fs::exists(path)) {
                    folders.push_back(path);
                }
            }
        }
    }
    
    // remove dups
    std::unordered_set<std::string> unique_paths;
    std::vector<fs::path> unique_folders;
    
    for (const auto& folder : folders) {
        std::string path_str = folder.string();
        if (unique_paths.find(path_str) == unique_paths.end()) {
            unique_paths.insert(path_str);
            unique_folders.push_back(folder);
        }
    }
    
    return unique_folders;
}

std::optional<std::pair<fs::path, fs::path>> SteamGameFinder::find_game_by_appid(const std::string& app_id) const {
    for (const auto& library_path : library_folders_) {
        fs::path acf_file = library_path / ("appmanifest_" + app_id + ".acf");
        
        if (fs::exists(acf_file)) {
            auto acf_data = parse_vdf_file(acf_file);
            
            auto it = acf_data.find("AppState.installdir");
            if (it != acf_data.end()) {
                fs::path game_path = library_path / "common" / it->second;
                
                if (fs::exists(game_path)) {
                    return std::make_pair(game_path, library_path);
                }
            }
        }
    }
    
    return std::nullopt;
}

std::optional<fs::path> SteamGameFinder::find_proton_prefix(const std::string& app_id, 
                                                           const std::optional<fs::path>& library_path) const {
    if (library_path) {
        fs::path compatdata_path = *library_path / "compatdata" / app_id / "pfx";
        if (fs::exists(compatdata_path)) {
            return compatdata_path;
        }
    }
    
    for (const auto& lib_path : library_folders_) {
        fs::path compatdata_path = lib_path / "compatdata" / app_id / "pfx";
        if (fs::exists(compatdata_path)) {
            return compatdata_path;
        }
    }
    
    return std::nullopt;
}

GameInfo SteamGameFinder::get_game_info(const std::string& app_id) const {
    GameInfo result;
    result.app_id = app_id;
    
    auto game_info = find_game_by_appid(app_id);
    if (game_info) {
        result.game_path = game_info->first;
        result.library_path = game_info->second;
        result.found = true;
        
        auto proton_prefix = find_proton_prefix(app_id, game_info->second);
        if (proton_prefix) {
            result.proton_prefix = *proton_prefix;
        }
    }
    
    return result;
}