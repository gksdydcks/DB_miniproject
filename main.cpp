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

// ��ȯ �Լ�
std::string wideToUtf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

std::wstring utf8ToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

int main() {
    system("chcp 65001 > nul");
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;
    std::vector<char> buffer(2048);  // �˳��ϰ� Ȯ��

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    std::wcout << L"������ ����Ǿ����ϴ�.\n";

    bool running = true;
    while (running) {
        std::wcout << L"\n=== �ʱ�ȭ�� ===\n1. ȸ������\n2. �α���\n3. ����\n> ";
        int menu;
        std::wcin >> menu;
        std::wcin.ignore();

        std::wstring wusername, wpassword;
        std::string message;

        if (menu == 1) {
            std::wcout << L"== ȸ������ ==\n���̵�: ";
            std::getline(std::wcin, wusername);
            std::wcout << L"��й�ȣ: ";
            std::getline(std::wcin, wpassword);
            message = "REGISTER:" + wideToUtf8(wusername) + ":" + wideToUtf8(wpassword) + ":";
        }
        else if (menu == 2) {
            std::wcout << L"== �α��� ==\n���̵�: ";
            std::getline(std::wcin, wusername);
            std::wcout << L"��й�ȣ: ";
            std::getline(std::wcin, wpassword);
            message = "LOGIN:" + wideToUtf8(wusername) + ":" + wideToUtf8(wpassword) + ":";
        }
        else if (menu == 3) {
            running = false;
            break;
        }
        else {
            std::wcout << L"�߸��� �����Դϴ�.\n";
            continue;
        }

        send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);

        int recvLen = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (recvLen > 0) {
            try {
                std::string response(buffer.data(), recvLen);
                std::wcout << L"[���� ����] " << utf8ToWstring(response) << L"\n";

                if (response == "Login Success") {
                    bool login = true;
                    while (login) {
                        std::wcout << L"\n=== �α��� �� �޴� ===\n1. ä���ϱ�\n2. �α׾ƿ�\n3. ����\n> ";
                        int sub;
                        std::wcin >> sub;
                        std::wcin.ignore();

                        if (sub == 1) {
                            std::wcout << L"[ä�� ���] 'exit' �Է� �� ä�� ����\n";
                            while (true) {
                                std::wstring wmsg;
                                std::getline(std::wcin, wmsg);

                                if (wmsg == L"exit") break;

                                std::string chatMsg = "CHAT:" + wideToUtf8(wmsg);
                                send(clientSocket, chatMsg.c_str(), static_cast<int>(chatMsg.length()), 0);

                                int chatRecv = recv(clientSocket, buffer.data(), buffer.size(), 0);
                                if (chatRecv > 0) {
                                    std::string chatResp(buffer.data(), chatRecv);
                                    std::wcout << L"[���� ����] " << utf8ToWstring(chatResp) << L"\n";
                                }
                                else {
                                    std::wcout << L"[���� ���� ���� ����]\n";
                                    break;
                                }
                            }
                        }
                        else if (sub == 2) {
                            std::string logoutMsg = "exit";
                            send(clientSocket, logoutMsg.c_str(), logoutMsg.length(), 0);
                            int outRecv = recv(clientSocket, buffer.data(), buffer.size(), 0);
                            if (outRecv > 0) {
                                std::string bye(buffer.data(), outRecv);
                                std::wcout << L"[���� ����] " << utf8ToWstring(bye) << L"\n";
                            }
                            login = false;
                        }
                        else if (sub == 3) {
                            running = false;
                            break;
                        }
                        else {
                            std::wcout << L"�߸��� �����Դϴ�.\n";
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[���� �߻�] " << e.what() << std::endl;
            }
        }
        else if (recvLen == 0) {
            std::wcout << L"[���� ���� ����]\n";
            break;
        }
        else {
            std::wcout << L"[���� ���� ���� ����]\n";
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
