#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>


using namespace std;

int main() {
    // Mở cổng serial
    HANDLE hSerial = CreateFile("COM3", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        cerr << "Loi mo cong serial!" << endl;
        return 1;
    }

    // Cấu hình thông số cổng serial
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        cerr << "Loi lay thong so cong!" << endl;
        CloseHandle(hSerial);
        return 1;
    }
    
    dcbSerialParams.BaudRate = CBR_57600; // số bit truyền đi trong 1s
    dcbSerialParams.ByteSize = 8; // số bit truyền đi trong một frame
    dcbSerialParams.StopBits = ONESTOPBIT; // số bit dừng để báo hiệu kết thúc một frame
    dcbSerialParams.Parity = NOPARITY; // kiểm tra lỗi trong truyền dữ liệu
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        cerr << "Loi thiet lap thong so cong!" << endl;
        CloseHandle(hSerial);
        return 1;
    }

    // Thiết lập timeout
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        cerr << "Loi thiet lap timeout!" << endl;
        CloseHandle(hSerial);
        return 1;
    }

    // Mở file để ghi dữ liệu
    ofstream file("test_collect.txt");
    if (!file.is_open()) {
        cerr << "Loi mo file!" << endl;
        CloseHandle(hSerial);
        return 1;
    }

    cout << "BAT DAU THU THAP DU LIEU!" << endl;
    
    vector<BYTE> buffer(256);
    DWORD bytesRead;
    vector<BYTE> lineBuffer;
    
    auto start = chrono::steady_clock::now();
    const int time_rec = 5; // Thời gian thu thập (giây)

    while (true) {
        // Kiểm tra thời gian
        auto end = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(end - start).count() >= time_rec) {
            break;
        }

        if (ReadFile(hSerial, buffer.data(), buffer.size(), &bytesRead, NULL)) {
            if (bytesRead > 0) {
                for (DWORD i = 0; i < bytesRead; ++i) {
                    BYTE c = buffer[i];
                    lineBuffer.push_back(c);
                    
                    // Kết thúc dòng khi gặp ký tự newline
                    if (c == '\n') {
                        // In giá trị thập phân ra màn hình
                        cout << "Nhan du lieu: [";
                        for (size_t j = 0; j < lineBuffer.size(); ++j) {
                            if (j > 0) cout << ", ";
                            cout << (int)lineBuffer[j];
                        }
                        cout << "]" << endl;

                        // Ghi dữ liệu vào file
                        for (BYTE val : lineBuffer) {
                            file << (int)val << " ";
                        }
                        file << endl;
                        
                        lineBuffer.clear();
                    }
                }
            }
        } else {
            cerr << "Loi doc du lieu!" << endl;
            break;
        }
    }

    cout << "HOAN THANH!" << endl;
    CloseHandle(hSerial);
    file.close();
    return 0;
}