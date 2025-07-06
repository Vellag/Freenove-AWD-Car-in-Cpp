#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <map>

bool check_and_install(const std::string& package);
bool apt_install(const std::string& package);
bool custom_install(const std::string& command);
int get_raspberry_pi_version();
void update_config_file(const std::string& file_path, const std::string& command, const std::string& value);
void config_camera_to_config_txt(const std::string& file_path, const std::string& command, const std::string& value = "");
void backup_file(const std::string& file_path);
void config_file();
std::string strip(const std::string& s);
bool startswith(const std::string& full, const std::string& prefix);

int main()
{
    std::map<std::string, bool> install_status = 
    {
        {"qt5-default qtbase5-dev qttools5-dev-tools", false},
        {"WiringPi", false},
        {"cmake", false},
        {"rpi-ws281x", false}
    };

    std::system("sudo apt-get update");
    install_status["qt5-default qtbase5-dev qttools5-dev-tools"] = apt_install("qt5-default qtbase5-dev qttools5-dev-tools");
    //install_status["WiringPi"] = finish later
    install_status["cmake"] = check_and_install("cmake");
    install_status["rpi-ws281x"] = custom_install("rpi-ws281x");
    
    return 0;
}

bool apt_install(const std::string& package)
{
    std::string install_command = "sudo apt-get install -y " + package;

        if (std::system(install_command.c_str()) == 0)
            return true;
        printf("Failed to install %s via apt-get.\n", package.c_str());
        return false;            
}

bool check_and_install(const std::string& package)
{
    std::string tmp = "dpkg -s " + package + " > /dev/null 2>&1";
    if (std::system(tmp.c_str()) == 0)
    {
        printf("%s is already installed\n", package.c_str());
        return true;
    }
    else
    {
        printf("%s not found. Attempting to install...\n", package.c_str());
        tmp = "sudo apt-get update && sudo apt-get install -y " + package;
        if(std::system(tmp.c_str()) == 0)
        {
            printf("Successfully installed %s.\n", package.c_str());
            return true;
        }
        else
        {
            printf("Failed to install %s.\n", package.c_str());
            return false;
        }
    }
}

bool custom_install(const std::string& command)
{
    if(std::system(command.c_str()) == 0)
        return true;
    printf("Failed to execute custom command: %s\n", command.c_str());
    return false;
}

void config_file()
{
    int pi_version = get_raspberry_pi_version();
    std::string file_path = "/boot/firmware/config.txt";
    backup_file(file_path);
    update_config_file(file_path, "dtparam=spi", "on");
    update_config_file(file_path, "camera_auto_detect", "0");
    std::string camera_model, camera_port;
    while(1)
    {
        char buf[100];
        printf("\nEnter the camera model (e.g., ov5687 or imx219): ");
        scanf("%99s", buf);
        for (int i = 0; i < std::strlen(buf); ++i)
            buf[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(buf[i])));
        camera_model = strip(buf);
        if (camera_model != "ov5647" || camera_model != "imx219")
            printf("Invalid input. Please enter either ov5647 or imx219.");
        else   
            break;
    }
    if (pi_version == 3)
    {
        printf("Setting up for Raspberry Pi 5\n");
        while(1)
        {
            char buf[100];
            printf("You have a Raspberry Pi 5. Which camera port is the camera conencted to? cam0 or cam1: ");
            scanf("%99s", buf);
            for (int i = 0; i < std::strlen(buf); ++i)
                buf[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(buf[i])));
            camera_port = strip(buf);
            if (camera_port != "cam0" || camera_port != "cam1")
                printf("Invalid input. Please enter either cam0 or cam1\n");
            else
                break;
        }
        config_camera_to_config_txt(file_path, camera_model, camera_port);
    }
    else if (pi_version == 2)
    {
        printf("Setting up for Raspberry Pi 3\n");
        update_config_file(file_path, "dtparam=audio", "off");
        config_camera_to_config_txt(file_path, camera_model); 
    }
    else
        config_camera_to_config_txt(file_path, camera_model); 
}
int get_raspberry_pi_version()
{
    printf("Getting Raspberry Pi version...\n");
    std::ifstream infile("/sys/firmware/devicetree/base/model");
    if(!infile)
    {
        printf("Failed to get Raspberry Pi version\n");
        return -1;
    }

    std::string model;
    std::getline(infile, model);
    infile.close();

    model.erase(std::remove(model.begin(), model.end(), "\0"), model.end());

    if (model.find("Raspberry Pi 5") != std::string::npos)
    {
        printf("Detected Raspberry Pi 5\n");
        return 3;
    }
    else if (model.find("Raspberry Pi 3") != std::string::npos)
    {
        printf("Detected Raspberry Pi 3\n");
        return 2;
    }
    else
    {
        printf("Detected Raspberry Pi %s\n", model.c_str());
        return 1;
    }
}

void backup_file(const std::string& file_path)
{
    std::string config_path = file_path;
    std::string backup_path = config_path + ".bak";
    printf("Backing up %s\n", backup_path.c_str());

    try
    {
        std::ifstream src_file(config_path, std::ios::binary);
        std::ofstream dst_file(backup_path, std::ios::binary);

        if(!src_file || !dst_file)
            throw std::runtime_error("Unable to open source or destination file.");
        
        dst_file << src_file.rdbuf();

        printf("Backup of %s created at %s\n", config_path.c_str(), backup_path.c_str());
    }
    catch(const std::exception& e)
    {
        printf("Error backing up %s: %s\n", config_path.c_str(), e.what());
    }
    
}

void update_config_file(const std::string& file_path, const std::string& command, const std::string& value)
{
    std::vector <std::string> new_content;
    bool command_found = false;
    std::ifstream infile(file_path);
    if(!infile)
    {
        printf("Unable to open file: %s\n", file_path.c_str());
        return;
    }
    std::string line;
    while(std::getline(infile, line))
    {
        std::string stripped_line = strip(line);
        if(startswith(stripped_line, command) || startswith(stripped_line, "#" + command))
        {
            command_found = true;
            new_content.push_back(command + "=" + value);
        }
        else
            new_content.push_back(line);
    }
    infile.close();

    if (!command_found)
        new_content.push_back("\n" + command + "=" + value + "\n");
    std::ofstream outfile(file_path);
    if(!outfile)
    {
        printf("Unable to open file: %s\n", file_path.c_str());
        return;
    }
    for (const std::string& lines : new_content)
        outfile << lines << "\n";
    outfile.close();
    printf("Updated %s with %s=%s\n", file_path, command, value);
}
std::string strip(const std::string& s)
{
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();

    if (start >= end)
        return "";
    
    return std::string(start, end);
}
bool startswith(const std::string& full, const std::string& prefix)
{   return full.compare(0, prefix.size(), prefix) == 0; }