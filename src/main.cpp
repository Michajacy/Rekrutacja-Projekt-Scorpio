/*  Program must be run with admin privileges  */ 


#include    <vector>
#include    <iostream>
#include    <chrono>
#include    <thread>
#include    <fstream>
#include    <sstream>

#include    <filesystem>

#include    <unistd.h>

#include    <cstring>

#include    <net/if.h>
#include    <netinet/in.h>
#include    <sys/ioctl.h>
#include    <sys/socket.h>
#include    <arpa/inet.h>
#include    <libnet.h>
#include    <ifaddrs.h>

#include    <cstdlib>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int CPU_STATES {10};

typedef struct CPU_Usage
{
    std::string cpu;
    size_t times[CPU_STATES];
}
CPU_Usage;

enum  CPUStates
{
        S_USER = 0,
        S_NICE,
        S_SYSTEM,
        S_IDLE,
        S_IOWAIT,
        S_IRQ,
        S_SOFTIRQ,
        S_STEAL, 
        S_GUEST, 
        S_GUEST_NICE          

};

size_t Get_Idle_Time(const CPU_Usage & e)
{
    return  e.times[S_IDLE] +
            e.times[S_IOWAIT];
}

size_t Get_Active_Time(const CPU_Usage & e)
{
    return  e.times[S_USER] +
            e.times[S_NICE] +
            e.times[S_SYSTEM] +
            e.times[S_IRQ] +
            e.times[S_SOFTIRQ] +
            e.times[S_STEAL] +
            e.times[S_GUEST] +
            e.times[S_GUEST_NICE];
}

void Print_Cpu_Usage (const std::vector<CPU_Usage> & entries1, const std::vector<CPU_Usage> & entries2)
{
    const size_t NUM_ENTRIES = entries1.size();

    std::ofstream ofs ("../data/system_data_readings.txt");

    std::cout << "CPU Usage\n";

    ofs << "CPU_Usage\n";

    for(size_t i{1}; i < NUM_ENTRIES; i++)
    {
        const CPU_Usage & e1 = entries1[i];
        const CPU_Usage & e2 = entries2[i];


        if (e1.cpu == "tot")
            return;

        std::cout << "Core " << e1.cpu << "] ";

        const float Active_Time = static_cast<float>(Get_Active_Time(e2) - Get_Active_Time(e1));
        const float Idle_Time = static_cast<float>(Get_Idle_Time(e2) - Get_Idle_Time(e1));
        const float Total_Time = Active_Time + Idle_Time;

        const float Usage = Active_Time / Total_Time * 100;

        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(1);
        std::cout.width(8);
        std::cout << Usage <<"%\n";
        

        ofs.setf(std::ios::fixed, std::ios::floatfield);
        ofs.precision(1);
        ofs.width(8);
        ofs << "Core" << e1.cpu << "]   " << Usage << "%\n";
    }

    ofs.close();
}

void Read_Cpu_Usage(std::vector<CPU_Usage> & entries)
{
    std::ifstream ifs("/proc/stat");

    std::string line;

    const std::string STR_CPU("cpu");
    const size_t LEN_STR_CPU = STR_CPU.size();
    const std::string STR_TOT("tot");

    while(std::getline(ifs, line))
    {
        if(!line.compare(0, LEN_STR_CPU, STR_CPU))
        {
            std::istringstream ss(line);

            entries.emplace_back(CPU_Usage());
            CPU_Usage & entry = entries.back();

            ss >> entry.cpu;

            if(entry.cpu.size() > LEN_STR_CPU)
                entry.cpu.erase(0, LEN_STR_CPU);
            else   
                entry.cpu = STR_TOT; 

            for(int i{1}; i <= CPU_STATES; i++)
                ss >> entry.times[i];                       
        }
    }
    
    ifs.close();
    
}

//niżej RAM, miejsce na dysku, interfejsy sieciowe

void RAM_Info()
{
    std::string line;
    double Total_RAM {0}, Used_RAM {0}, Free_RAM {0};

    std::ifstream ifs("/proc/meminfo");

        while (std::getline(ifs, line))
        {
            std::istringstream iss(line);
            std::string key;
            double value {0};

            if (iss >> key >> value)
            {
                if(key == "MemTotal:")
                    Total_RAM = value / 1024;

                else if (key == "MemAvailable:")
                    Free_RAM = value / 1024;
            }

            Used_RAM = Total_RAM - Free_RAM;
        }

    ifs.close();

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(0);
        std::cout.width(8);
        std::cout << "\nRAM\n" << "Total]\t" << Total_RAM << " MB\nUse]\t" << Used_RAM << " MB\nFree]\t" << Free_RAM << " MB  \n";

    std::ofstream ofs("../data/system_data_readings.txt", std::ios::app);

    ofs.setf(std::ios::fixed, std::ios::floatfield);
        ofs.precision(0);
        ofs.width(8);
        ofs << "\nRAM\n" << "Total]    " << Total_RAM << " MB\nUse]    " << Used_RAM << " MB\nFree]    " << Free_RAM << " MB    \n";

    ofs.close();
}

void Free_Disk()
{
    std::filesystem::space_info tmp = std::filesystem::space("/tmp"); 

    int Byte_To_GB {1073741824};

    double Free_Space = tmp.free;
    Free_Space = Free_Space / Byte_To_GB;

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(2);
        std::cout.width(8);
        std::cout << "\nDisc\n" << "Free]    " << Free_Space << " GB\n";

    std::ofstream ofs("../data/system_data_readings.txt", std::ios::app);

    ofs.setf(std::ios::fixed, std::ios::floatfield);
        ofs.precision(2);
        ofs.width(8);
        ofs << "\nDisc\n" << "Free]\t" << Free_Space << " GB\n";

    ofs.close();   
}

void Network_Status()
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    bool Status_Obtained;
    std::string Status, Activity;

    std::ofstream ofs("../data/system_data_readings.txt", std::ios::app);

    if (getifaddrs(&ifap) == -1) 
    {
        perror("getifaddrs");
        exit(1);
    }

    std::cout << "\nNetwork\n";
    ofs << "\nNetwork\n";

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) 
            continue;


        //find IP
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ-1);

        if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) 
        {
            perror("ioctl");
            exit(1);
        }

        sa = (struct sockaddr_in *) ifa->ifa_addr;
        addr = inet_ntoa(sa->sin_addr);


        //check status
        Status_Obtained = false;
        struct ifreq ifru_flags;
        memset(&ifru_flags, 0, sizeof(ifru_flags));
        strncpy(ifru_flags.ifr_name, ifa->ifa_name, IFNAMSIZ-1);
        
        if (ioctl(fd, SIOCGIFFLAGS, &ifru_flags) == 0) 
        {
            if (ifru_flags.ifr_flags & IFF_UP) 
            {
                Status = "UP\n";
                if (ifru_flags.ifr_flags & IFF_RUNNING) 
                    Status_Obtained = 1;
            }
            else 
                Status = "DOWN\n";
            
        }
        if (!Status_Obtained)
            Status = "UNKNOWN\n";


        //Network activity
        
        /*struct if_data *ifd = (struct if_data *)ifa->ifa_data;
        if (ifd != NULL) 
        {
            std::cout << "\tSent:\t\t" << ifd->ifi_obytes << " bytes" << std::endl;
            std::cout << "\tReceived:\t" << ifd->ifi_ibytes << " bytes" << std::endl;
        } 
        else 
        {
            struct if_data64 *ifd64 = (struct if_data64 *)ifa->ifa_data;
            if (ifd64 != NULL) 
            {
                std::cout << "\tSent:\t\t" << ifd64->ifi_obytes << " bytes" << std::endl;
                std::cout << "\tReceived:\t" << ifd64->ifi_ibytes << " bytes" << std::endl;
            }
        }
        */
        
        //Print
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.width(8);
        std::cout << ifa->ifa_name << "]\t" << addr << '\t' << Status;

        ofs.setf(std::ios::fixed, std::ios::floatfield);
        ofs.width(8);
        ofs << ifa->ifa_name << "]\t" << addr << '\t' << Status;


        close(fd);
    }

    freeifaddrs(ifap);
    ofs.close();
}

void autostart()
{
    std::ifstream ifs("../data/main.desktop.txt");
    std::ofstream ofs("../data/main.desktop.txt", std::ios::app);

    std::string Autostart_File_Path = std::string(getenv("HOME"))+ "/.config/autostart/info.desktop"; 

    std::ofstream dest(Autostart_File_Path);
    std::string line;

    std::string current_path = std::filesystem::current_path(); std::cout << current_path << '\n';

    while (std::getline(ifs, line))
 
        if (line == "Exec=" + current_path + "/main")
            ofs.close();

    ifs.close();

    ofs << "\nExec=" << current_path << "/main";

    std::ifstream ifs_2("../data/main.desktop.txt");

    while (std::getline(ifs_2, line))
        dest << line << '\n';


    ofs.close();

    dest.close();

}

void copy()
{
    std::ifstream ifs("../data/system_data_readings.txt");
    std::ofstream ofs("/var/www/html/system_data_readings.txt");
    std::string line;

    while (std::getline(ifs, line))
        ofs << line << '\n';

    ofs.close();
    ifs.close();
}

int main (int argc, char * argv[])
{
    std::vector<CPU_Usage> entries1;
    std::vector<CPU_Usage> entries2;

    autostart();

    while(true)
    {
        Read_Cpu_Usage(entries1);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));    

        Read_Cpu_Usage(entries2);

        system("clear");

        Print_Cpu_Usage(entries1, entries2);    //użycie procesora

                                                //temp procesora (w ubuntu 22.04 nie ma tego w plikach)

        RAM_Info();                             //RAM

        Free_Disk();                            //wolne miejsce na dysku

        Network_Status();

        copy();                                 //przenosi system_data_readings.txt do /var/www/html

        sleep(2);
        
    }

    
    return 0;

}
