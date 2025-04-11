#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// 변환 함수
std::string wideToUtf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

std::wstring utf8ToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

SOCKET connectToServer(SOCKADDR_IN& serverAddr) {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    return clientSocket;
}

int main() {
    system("chcp 65001 > nul");
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKADDR_IN serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    std::vector<char> buffer(2048);
    SOCKET clientSocket = connectToServer(serverAddr);
    std::wcout << L"서버에 연결되었습니다.\n";

    bool running = true;
    while (running) {
        std::wcout << L"\n=== 초기화면 ===\n1. 회원가입\n2. 로그인\n3. 종료\n> ";
        int menu;
        std::wcin >> menu;
        std::wcin.ignore();

        std::wstring wusername, wpassword;
        std::string message;

        if (menu == 1) {
            std::wcout << L"== 회원가입 ==\n아이디: ";
            std::getline(std::wcin, wusername);
            std::wcout << L"비밀번호: ";
            std::getline(std::wcin, wpassword);
            message = "REGISTER:" + wideToUtf8(wusername) + ":" + wideToUtf8(wpassword) + ":";
        }
        else if (menu == 2) {
            std::wcout << L"== 로그인 ==\n아이디: ";
            std::getline(std::wcin, wusername);
            std::wcout << L"비밀번호: ";
            std::getline(std::wcin, wpassword);
            message = "LOGIN:" + wideToUtf8(wusername) + ":" + wideToUtf8(wpassword) + ":";
        }
        else if (menu == 3) {
            running = false;
            break;
        }
        else {
            std::wcout << L"잘못된 선택입니다.\n";
            continue;
        }

        send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
        int recvLen = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);

        if (recvLen > 0) {
            std::string response(buffer.data(), recvLen);
            std::wcout << L"[서버 응답] " << utf8ToWstring(response) << L"\n";

            if (response == "Login Success") {
                bool login = true;
                while (login) {
                    std::wcout << L"\n=== 로그인 후 메뉴 ===\n1. 채팅하기\n2. 로그아웃\n3. 종료\n> ";
                    int sub;
                    std::wcin >> sub;
                    std::wcin.ignore();

                    if (sub == 1) {
                        std::wcout << L"[채팅 모드] 'exit' 입력 시 채팅 종료\n";
                        while (true) {
                            std::wstring wmsg;
                            std::getline(std::wcin, wmsg);

                            if (wmsg == L"exit") break;

                            std::string chatMsg = "CHAT:" + wideToUtf8(wmsg);
                            send(clientSocket, chatMsg.c_str(), static_cast<int>(chatMsg.length()), 0);

                            int chatRecv = recv(clientSocket, buffer.data(), buffer.size(), 0);
                            if (chatRecv > 0) {
                                std::string chatResp(buffer.data(), chatRecv);
                                std::wcout << L"[서버 응답] " << utf8ToWstring(chatResp) << L"\n";
                            }
                            else {
                                std::wcout << L"[서버 응답 수신 실패]\n";
                                break;
                            }
                        }
                    }
                    else if (sub == 2 || sub == 3) {
                        std::string logoutMsg = "exit";
                        send(clientSocket, logoutMsg.c_str(), static_cast<int>(logoutMsg.length()), 0);
                        recv(clientSocket, buffer.data(), buffer.size(), 0);
                        std::wcout << L"[서버 응답] 로그아웃\n";
                        closesocket(clientSocket);  // 소켓 종료
                        clientSocket = connectToServer(serverAddr);  // 소켓 재연결
                        login = false;
                        if (sub == 3) running = false;
                    }
                }
            }
        }
        else {
            std::wcout << L"[서버 응답 수신 실패]\n";
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
