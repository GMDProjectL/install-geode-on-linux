#pragma once

#include <filesystem>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace fs = std::filesystem;

struct GameInfo {
    std::string app_id;
    std::optional<fs::path> game_path;
    std::optional<fs::path> proton_prefix;
    std::optional<fs::path> library_path;
    bool found = false;
};

class SteamGameFinder {
public:
    SteamGameFinder();
    
    std::optional<std::pair<fs::path, fs::path>> find_game_by_appid(const std::string& app_id) const;
    std::optional<fs::path> find_proton_prefix(const std::string& app_id, 
                                               const std::optional<fs::path>& library_path = std::nullopt) const;
    GameInfo get_game_info(const std::string& app_id) const;
    
    const std::optional<fs::path>& get_steam_root() const { return steam_root_; }
    const std::vector<fs::path>& get_library_folders() const { return library_folders_; }

private:
    std::optional<fs::path> find_steam_root() const;
    std::vector<fs::path> initialize_library_folders() const;
    std::unordered_map<std::string, std::string> parse_vdf_file(const fs::path& file_path) const;
    void parse_vdf_recursive(const std::string& content, size_t& pos, 
                             std::unordered_map<std::string, std::string>& result, 
                             const std::string& prefix = "") const;
    
    std::optional<fs::path> steam_root_;
    std::vector<fs::path> library_folders_;
};