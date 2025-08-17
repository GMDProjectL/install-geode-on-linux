#include "GeodeInstaller.hpp"
#include <exception>
#include <iostream>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

int main() {
    // Clear screen

    system("clear");

    std::cout << BOLD << YELLOW << "======================================" << RESET << std::endl;
    std::cout << BOLD << YELLOW << "       Geode Installer for Linux  " << RESET << std::endl;
    std::cout << BOLD << YELLOW << "======================================" << RESET << std::endl << std::endl;
    
    std::cout << BOLD << WHITE << "Select an action:" << RESET << std::endl << std::endl;
    std::cout << BOLD << BLUE << "1." << RESET << " Install to " << BLUE << "Steam" << RESET << std::endl;
    std::cout << BOLD << MAGENTA << "2." << RESET << " Install to " << MAGENTA << "Wine" << RESET << " prefix" << std::endl;
    std::cout << BOLD << RED << "0." << RESET << " Quit" << std::endl << std::endl;
    std::cout << BOLD << WHITE << "What do you want to do: " << RESET;

    GeodeInstaller installer;
    std::string input;
    std::getline(std::cin, input);

    int choice;

    try {
        choice = std::stoi(input);
    } catch (const std::invalid_argument& e) {
        std::cout << BOLD << RED << "❌ Invalid input. Please enter a number." << RESET << std::endl;
        return 1;
    }

    try {
        switch (choice) {
            case 1: {
                std::cout << BOLD << BLUE << "🎮 Installing to Steam..." << RESET << std::endl;
                installer.install_geode_to_steam();
                break;
            }
            
            case 2: {
                std::cout << BOLD << MAGENTA << "🍷 Wine Installation" << RESET << std::endl;
                std::cout << YELLOW << "Enter your Geometry Dash path: " << RESET;
                std::string gd_path;
                std::getline(std::cin, gd_path);

                std::cout << YELLOW << "Enter your " << MAGENTA << "Wine" << RESET << YELLOW << " prefix path: " << RESET;
                std::string wine_prefix;
                std::getline(std::cin, wine_prefix);

                installer.install_geode_to_wine(wine_prefix, gd_path);
                break;
            }
            
            case 0: {
                std::cout << BOLD << YELLOW << "👋 Exiting..." << RESET << std::endl;
                return 0;
            }
            
            default: {
                std::cout << BOLD << RED << "❌ Invalid choice. Please try again." << RESET << std::endl;
                return 1;
            }
        }

        std::cout << std::endl << BOLD << GREEN << "✅ Geode has been successfully installed!" << RESET << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << std::endl << BOLD << RED << "❌ An error occurred: " << RESET << RED << e.what() << RESET << std::endl;
        return 1;
    }

    return 0;
}