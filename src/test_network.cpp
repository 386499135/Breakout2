#include "Network.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    NetworkManager net;
    
    std::cout << "=== 网络测试 ===" << std::endl;
    std::cout << "1. 创建主机" << std::endl;
    std::cout << "2. 连接主机" << std::endl;
    std::cout << "选择: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice == 1) {
        if (net.StartHost(12345)) {
            std::cout << "主机已启动，等待客户端..." << std::endl;
        } else {
            std::cout << "启动主机失败" << std::endl;
        }
    } else {
        std::string ip;
        std::cout << "输入主机IP (默认127.0.0.1): ";
        std::cin >> ip;
        if (ip.empty()) ip = "127.0.0.1";
        
        if (net.ConnectToHost(ip, 12345)) {
            std::cout << "已连接到主机" << std::endl;
        } else {
            std::cout << "连接失败" << std::endl;
        }
    }
    
    std::cout << "按 Enter 退出..." << std::endl;
    std::cin.ignore();
    std::cin.get();
    
    return 0;
}
