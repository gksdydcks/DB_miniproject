#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <mysql/jdbc.h>
#include <locale>
#include <codecvt>
#include <fcntl.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")

const std::string DB_USER = "root";
const std::string DB_URL = "tcp://127.0.0.1:3306?useUnicode=true&characterEncoding=utf8mb4";

// UTF-8 문자열 → 유니코드 문자열 변환
std::wstring utf8ToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}


void handleClient(SOCKET clientSocket, const std::string& dbPassword, const std::string& dbName) {
    std::vector<char> buffer(2048);
    std::string loggedInUser;
    int user_id = -1;

    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(DB_URL, DB_USER, dbPassword));
        conn->setSchema(dbName);

        while (true) {
            int recvLen = recv(clientSocket, buffer.data(), buffer.size(), 0);
            if (recvLen <= 0) break;

            std::string msg(buffer.data(), recvLen);
            std::wcout << L"[RECV] " << utf8ToWstring(msg) << std::endl;

            if (msg.rfind("REGISTER:", 0) == 0) {
                size_t pos1 = msg.find(":", 9);
                size_t pos2 = msg.find(":", pos1 + 1);
                if (pos1 == std::string::npos || pos2 == std::string::npos) {
                    std::string error = "Invalid Format";
                    send(clientSocket, error.c_str(), error.length(), 0);
                    continue;
                }

                std::string username = msg.substr(9, pos1 - 9);
                std::string password = msg.substr(pos1 + 1, pos2 - pos1 - 1);

                auto checkUser = conn->prepareStatement("SELECT * FROM users WHERE username = ?");
                checkUser->setString(1, username);
                auto checkRes = std::unique_ptr<sql::ResultSet>(checkUser->executeQuery());

                std::string response;
                if (checkRes->next()) {
                    response = "ID already exists";
                }
                else {
                    auto insertUser = conn->prepareStatement("INSERT INTO users(username, password, status) VALUES (?, ?, 'active')");
                    insertUser->setString(1, username);
                    insertUser->setString(2, password);
                    insertUser->execute();
                    response = "Register Success";
                }
                send(clientSocket, response.c_str(), response.length(), 0);
            }

            else if (msg.rfind("LOGIN:", 0) == 0) {
                size_t pos1 = msg.find(":", 6);
                size_t pos2 = msg.find(":", pos1 + 1);
                if (pos1 == std::string::npos || pos2 == std::string::npos) {
                    std::string error = "Invalid Format";
                    send(clientSocket, error.c_str(), error.length(), 0);
                    continue;
                }

                std::string username = msg.substr(6, pos1 - 6);
                std::string password = msg.substr(pos1 + 1, pos2 - pos1 - 1);

                auto loginQuery = conn->prepareStatement("SELECT user_id FROM users WHERE username = ? AND password = ?");
                loginQuery->setString(1, username);
                loginQuery->setString(2, password);
                auto res = std::unique_ptr<sql::ResultSet>(loginQuery->executeQuery());

                std::string result;
                if (res->next()) {
                    result = "Login Success";
                    loggedInUser = username;
                    user_id = res->getInt("user_id");
                }
                else {
                    result = "Login Failed";
                }
                send(clientSocket, result.c_str(), result.length(), 0);
            }

            else if (msg.rfind("CHAT:", 0) == 0) {
                if (user_id == -1) {
                    std::string err = "Not logged in";
                    send(clientSocket, err.c_str(), err.length(), 0);
                    continue;
                }

                std::string content = msg.substr(5);
                std::string echo = "Echo: " + content;
                send(clientSocket, echo.c_str(), echo.length(), 0);

                auto chatInsert = conn->prepareStatement("INSERT INTO message_log(sender_id, content) VALUES (?, ?)");
                chatInsert->setInt(1, user_id);
                chatInsert->setString(2, content);
                chatInsert->execute();
            }

            else if (msg == "exit") {
                std::string bye = "[서버] 로그아웃 되었습니다.";
                send(clientSocket, bye.c_str(), bye.length(), 0);
                break;
            }

            else {
                std::string error = "Unknown Command";
                send(clientSocket, error.c_str(), error.length(), 0);
            }
        }

    }
    catch (sql::SQLException& e) {
        std::cerr << "[MySQL 에러] " << e.what() << std::endl;
        std::string err = "DB Error";
        send(clientSocket, err.c_str(), err.length(), 0);
    }

    closesocket(clientSocket);
}

int main() {
    // 콘솔 UTF-8 설정
    system("chcp 65001 > nul");
    _setmode(_fileno(stdout), _O_U16TEXT);

    std::string dbPassword, dbName;
    std::wcout << L"Password: ";
    std::getline(std::cin, dbPassword);
    std::wcout << L"DB Name: ";
    std::getline(std::cin, dbName);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    std::wcout << L"[Server start] Listen in 9000...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            std::wcout << L"[Client connect]\n";
            handleClient(clientSocket, dbPassword, dbName);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
