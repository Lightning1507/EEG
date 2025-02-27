#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std;

// Hàm đọc một byte từ cổng serial
BYTE ReadOneByte(HANDLE hSerial) {
    BYTE byteRead;
    DWORD bytesRead;
    if (!ReadFile(hSerial, &byteRead, 1, &bytesRead, NULL)) {
        cerr << "Loi doc du lieu tu cong serial!" << endl;
        return 0;
    }
    return byteRead;
}

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
    dcbSerialParams.BaudRate = CBR_57600;  // Tốc độ baud 57600
    dcbSerialParams.ByteSize = 8;          // 8 bit dữ liệu
    dcbSerialParams.StopBits = ONESTOPBIT; // 1 bit dừng
    dcbSerialParams.Parity = NOPARITY;     // Không sử dụng bit chẵn lẻ
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
    ofstream file("D:\\Project\\test_collect.txt");
    if (!file.is_open()) {
        cerr << "Loi mo file!" << endl;
        CloseHandle(hSerial);
        return 1;
    }

    cout << "BAT DAU THU THAP DU LIEU!" << endl;

    // Biến để lưu trữ dữ liệu
    int rawData = 0;
    byte payloadData[64] = {0};
    int payloadLength = 0;
    byte checksum = 0;
    byte generatedChecksum = 0;

    auto start = chrono::steady_clock::now();
    const int time_rec = 5;  // Thời gian thu thập (giây)

    while (true) {
        // Kiểm tra thời gian
        auto end = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(end - start).count() >= time_rec) {
            break;
        }

        // Tìm sync bytes (0xAA 0xAA)
        if (ReadOneByte(hSerial) == 0xAA) {
            if (ReadOneByte(hSerial) == 0xAA) {
                payloadLength = ReadOneByte(hSerial);  // Đọc độ dài payload
                if (payloadLength > 169) continue;     // Bỏ qua nếu payload quá dài

                generatedChecksum = 0;
                for (int i = 0; i < payloadLength; i++) {
                    payloadData[i] = ReadOneByte(hSerial);  // Đọc payload
                    generatedChecksum += payloadData[i];
                }

                checksum = ReadOneByte(hSerial);  // Đọc checksum
                generatedChecksum = 255 - generatedChecksum;  // Tính checksum

                if (checksum == generatedChecksum) {
                    // Phân tích payload
                    for (int i = 0; i < payloadLength; i++) {
                        switch (payloadData[i]) {
                            case 0x80:  // rawData
                                i++;
                                rawData = ((int)payloadData[i + 1] << 8) | payloadData[i + 2];  // Ghép 2 byte thành rawData
                                cout << "Raw Data: " << rawData << endl;  // In rawData ra màn hình
                                file << "Raw Data: " << rawData << endl;  // Ghi rawData vào file
                                i += 2;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    cout << "HOAN THANH!" << endl;
    CloseHandle(hSerial);
    file.close();
    return 0;
}
