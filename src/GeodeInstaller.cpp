#include "GeodeInstaller.hpp"
#include <curl/curl.h>
#include <zip.h>
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <chrono>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    return fwrite(contents, size, nmemb, file);
}

GeodeInstaller::GeodeInstaller() : finder_() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::string GeodeInstaller::make_http_request(const std::string& url) const {
    CURL* curl;
    CURLcode res;
    std::string response;
    
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    res = curl_easy_perform(curl);
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("Curl request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    if (response_code != 200) {
        throw std::runtime_error("HTTP error code: " + std::to_string(response_code));
    }
    
    return response;
}

void GeodeInstaller::download_file(const std::string& url, const fs::path& output_path) const {
    CURL* curl;
    CURLcode res;
    FILE* file;
    
    file = fopen(output_path.string().c_str(), "wb");
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + output_path.string());
    }
    
    curl = curl_easy_init();
    if (!curl) {
        fclose(file);
        throw std::runtime_error("Failed to initialize curl");
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    
    res = curl_easy_perform(curl);
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        fs::remove(output_path);
        throw std::runtime_error("Download failed: " + std::string(curl_easy_strerror(res)));
    }
    
    if (response_code != 200) {
        fs::remove(output_path);
        throw std::runtime_error("HTTP error code: " + std::to_string(response_code));
    }
}

void GeodeInstaller::extract_zip(const fs::path& zip_path, const fs::path& destination) const {
    int err = 0;
    zip_t* archive = zip_open(zip_path.string().c_str(), 0, &err);
    
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::string error_msg = "Failed to open zip file: " + std::string(zip_error_strerror(&error));
        zip_error_fini(&error);
        throw std::runtime_error(error_msg);
    }
    
    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    if (num_entries < 0) {
        zip_close(archive);
        throw std::runtime_error("Failed to get number of entries in zip file");
    }
    
    for (zip_int64_t i = 0; i < num_entries; i++) {
        const char* name = zip_get_name(archive, i, 0);
        if (!name) {
            zip_close(archive);
            throw std::runtime_error("Failed to get entry name");
        }
        
        fs::path entry_path = destination / name;
        
        if (name[strlen(name) - 1] == '/') {
            fs::create_directories(entry_path);
            continue;
        }
        
        fs::create_directories(entry_path.parent_path());
        
        zip_file_t* file = zip_fopen_index(archive, i, 0);
        if (!file) {
            zip_close(archive);
            throw std::runtime_error("Failed to open file in zip: " + std::string(name));
        }
        
        std::ofstream output(entry_path, std::ios::binary);
        if (!output) {
            zip_fclose(file);
            zip_close(archive);
            throw std::runtime_error("Failed to create output file: " + entry_path.string());
        }
        
        char buffer[8192];
        zip_int64_t bytes_read;
        while ((bytes_read = zip_fread(file, buffer, sizeof(buffer))) > 0) {
            output.write(buffer, bytes_read);
        }
        
        if (bytes_read < 0) {
            zip_fclose(file);
            zip_close(archive);
            throw std::runtime_error("Failed to read from zip file");
        }
        
        output.close();
        zip_fclose(file);
    }
    
    zip_close(archive);
}

std::string GeodeInstaller::get_current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    return std::to_string(time_t);
}

std::string GeodeInstaller::get_hex_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::hex << time_t;
    return ss.str();
}

std::string GeodeInstaller::get_latest_geode_tag() const {
    std::string url = "https://api.geode-sdk.org/v1/loader/versions/latest";
    std::string response = make_http_request(url);
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse JSON response");
    }
    
    if (!root["error"].asString().empty()) {
        throw std::runtime_error("API error: " + root["error"].asString());
    }
    
    if (root["payload"].isNull()) {
        throw std::runtime_error("Payload is null");
    }
    
    return root["payload"]["tag"].asString();
}

std::string GeodeInstaller::get_download_url() const {
    std::string tag = get_latest_geode_tag();
    return "https://github.com/geode-sdk/geode/releases/download/" + tag + "/geode-" + tag + "-win.zip";
}

void GeodeInstaller::unzip_to_destination(const std::string& zip_url, const fs::path& destination_dir) const {
    fs::path zip_file_path = destination_dir / "geode_win.zip";
    fs::create_directories(destination_dir);

    std::cout << "Downloading geode_win.zip from " << zip_url << "...\n";
    
    download_file(zip_url, zip_file_path);
    extract_zip(zip_file_path, destination_dir);
    fs::remove(zip_file_path);
}

void GeodeInstaller::install_to_dir(const fs::path& destination_dir) const {
    std::string win_zip_release_link = get_download_url();

    std::cout << "Get ready to download Geode...\n";

    unzip_to_destination(win_zip_release_link, destination_dir);
}

void GeodeInstaller::patch_prefix_registry(const fs::path& reg_file_path) const {
    if (!fs::exists(reg_file_path)) {
        throw std::runtime_error("Registry file not found: " + reg_file_path.string());
    }
    
    std::ifstream file(reg_file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open registry file: " + reg_file_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    const std::string dll_overrides_section = "[Software\\\\Wine\\\\DllOverrides]";
    const std::string xinput_entry = "\"xinput1_4\"=\"native,builtin\"";
    
    if (content.find(dll_overrides_section) == std::string::npos) {
        // section doesn't exist, add it
        content += "\n\n" + dll_overrides_section + " " + get_current_timestamp() + "\n";
        content += "#time=" + get_hex_timestamp() + "\n";
        content += xinput_entry + "\n";
    } else {
        if (content.find("\"xinput1_4\"=") == std::string::npos) {
            size_t section_pos = content.find(dll_overrides_section);
            size_t next_section_pos = content.find("\n[", section_pos + dll_overrides_section.length());
            
            if (next_section_pos != std::string::npos) {
                content.insert(next_section_pos, xinput_entry + "\n");
            } else {
                content += "\n" + xinput_entry + "\n";
            }
        }
    }
    
    std::ofstream out_file(reg_file_path);
    if (!out_file.is_open()) {
        throw std::runtime_error("Failed to write to registry file: " + reg_file_path.string());
    }
    
    out_file << content;
    out_file.close();
}

void GeodeInstaller::install_geode_to_wine(const fs::path& prefix, const fs::path& gd_path) const {
    if (!fs::exists(prefix)) {
        throw std::runtime_error("Can't find prefix: " + prefix.string());
    }
    
    if (!fs::exists(gd_path)) {
        throw std::runtime_error("Can't find Geometry Dash: " + gd_path.string());
    }
    
    std::cout << "Installing Geode to: " << gd_path.string() << std::endl;
    install_to_dir(gd_path);
    
    std::cout << "Patching Wine registry..." << std::endl;
    fs::path user_reg = prefix / "user.reg";
    patch_prefix_registry(user_reg);
    
    std::cout << "Geode installation completed!" << std::endl;
}

void GeodeInstaller::install_geode_to_steam() const {
    if (!finder_.get_steam_root()) {
        throw std::runtime_error("Can't find Steam Root");
    }
    
    std::cout << "Steam root found at: " << finder_.get_steam_root()->string() << std::endl;
    
    // gd appid is 322170
    GameInfo gd_info = finder_.get_game_info("322170");
    
    if (!gd_info.found) {
        throw std::runtime_error("Can't find Geometry Dash.");
    }
    
    std::cout << "Geometry Dash found at: " << gd_info.game_path->string() << std::endl;
    
    if (!gd_info.proton_prefix) {
        throw std::runtime_error("Can't find Proton Prefix.");
    }
    
    std::cout << "Proton prefix found at: " << gd_info.proton_prefix->string() << std::endl;
    
    if (!fs::exists(*gd_info.game_path)) {
        throw std::runtime_error("Can't find Steam GD at " + gd_info.game_path->string());
    }
    
    install_geode_to_wine(*gd_info.proton_prefix, *gd_info.game_path);
}