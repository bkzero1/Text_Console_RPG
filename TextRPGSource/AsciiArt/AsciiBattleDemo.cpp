#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>

#include "AsciiBattleDemo.h"
#include "SceneConfig.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iostream>
#include <memory>
#include <map>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace
{
// ===== ASCII 아트 설정 =====
/*
값은 모두 0~1000 범위입니다.

항목                    0                   500                 1000
해상도                  최소 16 픽셀        가로 500 픽셀      가로 1000 픽셀
세로 비율               약 0.1배            원본 비율           약 1.9배
대비                    중간 회색에 가까움  원본 대비 1.0배     원본 대비 2.0배

단, 해상도는 현재 콘솔 창 안에 들어가는 최대 크기로 자동 제한됩니다.
*/


constexpr int kSliderWidth = 28;
constexpr short kSliderLabelWidth = 12;
constexpr short kControlWidth = kSliderLabelWidth + kSliderWidth + 10;
constexpr WORD kTextColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
bool gAnsiColorSupported = false;

// 마지막으로 그린 정지 배경의 실제 콘솔 범위입니다.
// 메뉴 문구를 콘솔 전체가 아니라 이 이미지의 가운데에 맞추는 데 사용합니다.
short gStaticImageLeft = 0;
short gStaticImageWidth = 0;
short gStaticImageTop = 0;
short gStaticImageHeight = 0;
bool gHasStaticImageBounds = false;

// 상점처럼 같은 장면을 계속 갱신할 때, 이전 프레임과 달라진 행만 다시 출력합니다.
// 창 크기나 AA 해상도가 바뀌면 전체 출력이 필요하므로 그때만 캐시를 비웁니다.
struct LayeredFrameCache
{
    std::vector<std::wstring> lines;
    short left = 0;
    short top = 0;
    short width = 0;
    bool valid = false;
};

LayeredFrameCache gLayeredFrameCache;
// 이번 전투에서 실제 등장한 각 몬스터 개체의 배치값입니다.
// 같은 슬라임 두 마리도 서로 다른 id를 가지므로 독립적으로 움직이고 크기를 조절할 수 있습니다.
std::map<std::string, MonsterVisualProfile> gBattleMonsterInstanceProfiles;
std::map<std::string, int> gBattleMonsterInstanceTypes;

struct RenderSettings
{
    int outputPixelWidth = 360;
    int characterHeightScaleValue = 500;
    int contrastValue = 630;
    int darknessBoostValue = 0;
    bool useOrderedDithering = true;
    bool useAnsiColor = true;
    int colorMode = 0;
};

enum class ESlider
{
    OUTPUT_WIDTH,
    HEIGHT_SCALE,
    CONTRAST,
    NONE,
};

enum class EObjectType
{
    HERO,
    HERO2,
    TANK,
    TANK_SHIELD,
    MONSTER,
    WARRIOR_WEAPON,
    MAGE_WEAPON,
    HIT_EFFECT,
    HEAL_EFFECT,
    POWER_BUFF_EFFECT,
};

struct SliderLayout
{
    ESlider slider = ESlider::NONE;
    short x = 2;
    short y = 0;
    short sliderStartX = 2 + kSliderLabelWidth;
};

struct ControlPanelLayout
{
    SliderLayout sliders[3];
    short artStartY = 0;
};

struct SceneObject
{
    EObjectType type;
    float x;
    float y;
    float width;
    float height;
    int layer;
    float breatheX;
    float breatheY;
    float phase;
    bool visible = true;
};

struct AttackAnimation
{
    bool playing = false;
    bool monsterAttacking = false;
    bool playerUsingPotion = false;
    bool playerUsingPowerPotion = false;
    int attackerIndex = 0;
    int targetIndex = 0;
    int hitEffectVariant = 0;
    std::chrono::steady_clock::time_point startedAt{};
};

int PickRandomHitEffectVariant()
{
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(0, 2);
    return distribution(generator);
}

enum class EPlacementTarget
{
    NONE,
    HERO,
    HERO2,
    TANK,
    MONSTER,
    MONSTER2,
    MONSTER3,
    MONSTER4,
    WARRIOR_WEAPON,
    TANK_SHIELD,
    MAGE_WEAPON,
    HEAL_EFFECT,
    POWER_BUFF_EFFECT,
};

struct PlacementMode
{
    bool active = false;
    bool dragging = false;
    EPlacementTarget selected = EPlacementTarget::NONE;
    SceneConfig beforeEditing;
    float grabOffsetX = 0.0f;
    float grabOffsetY = 0.0f;
    // MONSTER 선택 중에는 종류 공용 프로필이 아니라, 이번 전투의 실제 개체 id를 편집합니다.
    std::string monsterInstanceId;
    std::wstring monsterDisplayName;
};

struct ArtResolution
{
    int pixelWidth = 16;
    int pixelHeight = 4;
};

int ClampSetting(int value)
{
    return std::clamp(value, 0, 1000);
}

int GetOutputPixelWidth(const RenderSettings& settings)
{
    const int width = std::max(16, ClampSetting(settings.outputPixelWidth));
    return width % 2 == 0 ? width : width - 1;
}

double GetCharacterHeightScale(const RenderSettings& settings)
{
    // 0은 0.1배, 500은 1.0배, 1000은 1.9배입니다.
    return 0.1 + ClampSetting(settings.characterHeightScaleValue) * 0.0018;
}

ArtResolution CalculateArtResolution(Gdiplus::Bitmap& image, const RenderSettings& settings, int maximumWidth)
{
    ArtResolution resolution;
    resolution.pixelWidth = std::min(GetOutputPixelWidth(settings), maximumWidth);
    resolution.pixelWidth = std::max(16, resolution.pixelWidth - resolution.pixelWidth % 2);
    resolution.pixelHeight = static_cast<int>(
        image.GetHeight() * (static_cast<double>(resolution.pixelWidth) / image.GetWidth()) * GetCharacterHeightScale(settings));
    resolution.pixelHeight = std::max(4, resolution.pixelHeight - resolution.pixelHeight % 4);
    return resolution;
}

float Clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float SmoothStep(float value)
{
    value = Clamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

double GetElapsedSeconds(const std::chrono::steady_clock::time_point& startedAt)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - startedAt).count();
}

unsigned char CalculateBrightness(const Gdiplus::Color& color)
{
    return static_cast<unsigned char>(
        0.2126 * color.GetRed() + 0.7152 * color.GetGreen() + 0.0722 * color.GetBlue());
}

unsigned char ApplyContrast(unsigned char brightness, const RenderSettings& settings)
{
    const double result = 128.0 + (static_cast<double>(brightness) - 128.0) * (ClampSetting(settings.contrastValue) / 500.0);
    return static_cast<unsigned char>(std::clamp(result, 0.0, 255.0));
}

bool ShouldDrawDot(unsigned char brightness, int x, int y, const RenderSettings& settings)
{
    if (!settings.useOrderedDithering)
    {
        return brightness < 128;
    }

    constexpr int kBayer[4][4] = {
        {0, 8, 2, 10},
        {12, 4, 14, 6},
        {3, 11, 1, 9},
        {15, 7, 13, 5},
    };

    const int darkness = std::clamp(
        255 - static_cast<int>(brightness) + settings.darknessBoostValue,
        0,
        255
    );
    return darkness > kBayer[y % 4][x % 4] * 16 + 8;
}

UINT MapToSourceCoordinate(int outputCoordinate, int outputLength, UINT sourceLength)
{
    if (outputLength <= 1 || sourceLength <= 1)
    {
        return 0;
    }

    return outputCoordinate * (sourceLength - 1) / (outputLength - 1);
}

struct BrailleCell
{
    wchar_t character = L' ';
    Gdiplus::Color color = Gdiplus::Color(255, 255, 255, 255);
};

BrailleCell CreateBrailleCell(
    Gdiplus::Bitmap& image,
    int cellX,
    int cellY,
    int outputPixelWidth,
    int outputPixelHeight,
    const RenderSettings& settings)
{
    constexpr int kDotMask[4][2] = {
        {0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80},
    };

    int pattern = 0;
    int redTotal = 0;
    int greenTotal = 0;
    int blueTotal = 0;
    int visibleDotCount = 0;
    for (int dotY = 0; dotY < 4; ++dotY)
    {
        for (int dotX = 0; dotX < 2; ++dotX)
        {
            const int outputX = cellX * 2 + dotX;
            const int outputY = cellY * 4 + dotY;
            Gdiplus::Color color;
            image.GetPixel(
                MapToSourceCoordinate(outputX, outputPixelWidth, image.GetWidth()),
                MapToSourceCoordinate(outputY, outputPixelHeight, image.GetHeight()),
                &color);

            if (ShouldDrawDot(ApplyContrast(CalculateBrightness(color), settings), outputX, outputY, settings))
            {
                pattern |= kDotMask[dotY][dotX];
                redTotal += color.GetRed();
                greenTotal += color.GetGreen();
                blueTotal += color.GetBlue();
                ++visibleDotCount;
            }
        }
    }

    BrailleCell cell;
    cell.character = static_cast<wchar_t>(0x2800 + pattern);
    if (visibleDotCount > 0)
    {
        cell.color = Gdiplus::Color(
            255,
            redTotal / visibleDotCount,
            greenTotal / visibleDotCount,
            blueTotal / visibleDotCount);
    }
    return cell;
}

Gdiplus::Color ApplyColorMode(const Gdiplus::Color& color, int colorMode)
{
    const int mode = (colorMode % 4 + 4) % 4;
    if (mode == 1) // 차가운 파랑 계열
    {
        return Gdiplus::Color(255, color.GetRed() / 2, color.GetGreen(), std::min(255, color.GetBlue() + 70));
    }
    if (mode == 2) // 초록 계열
    {
        return Gdiplus::Color(255, color.GetRed() / 2, std::min(255, color.GetGreen() + 80), color.GetBlue() / 2);
    }
    if (mode == 3) // 따뜻한 주황 계열
    {
        return Gdiplus::Color(255, std::min(255, color.GetRed() + 70), color.GetGreen() * 2 / 3, color.GetBlue() / 2);
    }
    return color; // 원본 색
}

std::wstring CreateAnsiColorCode(const Gdiplus::Color& color, int colorMode)
{
    const Gdiplus::Color adjusted = ApplyColorMode(color, colorMode);
    return L"\x1b[38;2;" + std::to_wstring(adjusted.GetRed()) + L";" + std::to_wstring(adjusted.GetGreen()) + L";" +
           std::to_wstring(adjusted.GetBlue()) + L"m";
}

std::vector<std::wstring> CreateBrailleLines(Gdiplus::Bitmap& image, const RenderSettings& settings, int maximumOutputPixelWidth)
{
    const ArtResolution resolution = CalculateArtResolution(image, settings, maximumOutputPixelWidth);
    const int outputWidth = resolution.pixelWidth;
    const int outputHeight = resolution.pixelHeight;

    std::vector<std::wstring> lines;
    for (int cellY = 0; cellY < outputHeight / 4; ++cellY)
    {
        std::wstring line;
        Gdiplus::Color previousColor;
        bool hasPreviousColor = false;
        for (int cellX = 0; cellX < outputWidth / 2; ++cellX)
        {
            const BrailleCell cell = CreateBrailleCell(image, cellX, cellY, outputWidth, outputHeight, settings);
            if (settings.useAnsiColor && cell.character != static_cast<wchar_t>(0x2800))
            {
                if (!hasPreviousColor || cell.color.GetValue() != previousColor.GetValue())
                {
                    line += CreateAnsiColorCode(cell.color, settings.colorMode);
                    previousColor = cell.color;
                    hasPreviousColor = true;
                }
            }
            line += cell.character;
        }
        if (settings.useAnsiColor)
        {
            line += L"\x1b[0m";
        }
        lines.push_back(line);
    }
    return lines;
}

wchar_t GetLandscapeAsciiCharacter(
    Gdiplus::Bitmap& image,
    int x,
    int y,
    int outputWidth,
    int outputHeight,
    const RenderSettings& settings)
{
    const UINT sourceX = MapToSourceCoordinate(x, outputWidth, image.GetWidth());
    const UINT sourceY = MapToSourceCoordinate(y, outputHeight, image.GetHeight());

    Gdiplus::Color centerColor;
    image.GetPixel(sourceX, sourceY, &centerColor);
    const int centerBrightness = ApplyContrast(CalculateBrightness(centerColor), settings);
    const int darkness = std::clamp(
        255 - centerBrightness + settings.darknessBoostValue,
        0,
        255);

    // 밝은 하늘이나 안개는 공백으로 두어, 화면이 점으로 가득 차지 않게 합니다.
    if (darkness < 24)
    {
        return L' ';
    }

    const int leftX = std::max(0, x - 1);
    const int rightX = std::min(outputWidth - 1, x + 1);
    const int upY = std::max(0, y - 1);
    const int downY = std::min(outputHeight - 1, y + 1);

    Gdiplus::Color leftColor;
    Gdiplus::Color rightColor;
    Gdiplus::Color upColor;
    Gdiplus::Color downColor;
    image.GetPixel(MapToSourceCoordinate(leftX, outputWidth, image.GetWidth()), sourceY, &leftColor);
    image.GetPixel(MapToSourceCoordinate(rightX, outputWidth, image.GetWidth()), sourceY, &rightColor);
    image.GetPixel(sourceX, MapToSourceCoordinate(upY, outputHeight, image.GetHeight()), &upColor);
    image.GetPixel(sourceX, MapToSourceCoordinate(downY, outputHeight, image.GetHeight()), &downColor);

    const int gradientX = static_cast<int>(CalculateBrightness(rightColor)) - static_cast<int>(CalculateBrightness(leftColor));
    const int gradientY = static_cast<int>(CalculateBrightness(downColor)) - static_cast<int>(CalculateBrightness(upColor));
    const int absoluteX = std::abs(gradientX);
    const int absoluteY = std::abs(gradientY);

    // 명암 변화가 큰 곳은 산 능선, 절벽, 강둑처럼 보이도록 선 문자로 표시합니다.
    if (std::max(absoluteX, absoluteY) > 72)
    {
        if (absoluteX > absoluteY * 2)
        {
            return L'|';
        }
        if (absoluteY > absoluteX * 2)
        {
            return L'-';
        }
        return gradientX * gradientY >= 0 ? L'/' : L'\\';
    }

    constexpr std::wstring_view kBrightnessRamp = L" .,:;irsXA253hMHGS#9B&@";
    const size_t rampIndex = std::min(
        kBrightnessRamp.size() - 1,
        static_cast<size_t>(darkness) * (kBrightnessRamp.size() - 1) / 255);
    return kBrightnessRamp[rampIndex];
}

std::vector<std::wstring> CreateLandscapeAsciiLines(
    Gdiplus::Bitmap& image,
    const RenderSettings& settings,
    int maximumCharacterWidth,
    int maximumCharacterHeight)
{
    // 콘솔 문자는 세로가 더 길므로, 원본 비율을 보정해 줄 수를 계산합니다.
    const double imageAspect = static_cast<double>(image.GetWidth()) / image.GetHeight();
    const int outputWidth = std::max(16, std::min(
        maximumCharacterWidth,
        static_cast<int>(maximumCharacterHeight * 2.0 * imageAspect)));
    const int outputHeight = std::max(4, std::min(
        maximumCharacterHeight,
        static_cast<int>(outputWidth / (2.0 * imageAspect))));

    std::vector<std::wstring> lines;
    lines.reserve(outputHeight);
    for (int y = 0; y < outputHeight; ++y)
    {
        std::wstring line;
        line.reserve(outputWidth);
        for (int x = 0; x < outputWidth; ++x)
        {
            line += GetLandscapeAsciiCharacter(image, x, y, outputWidth, outputHeight, settings);
        }
        lines.push_back(line);
    }
    return lines;
}

void WriteAt(HANDLE handle, short x, short y, const std::wstring& text)
{
    SetConsoleCursorPosition(handle, {x, y});
    SetConsoleTextAttribute(handle, kTextColor);
    DWORD written = 0;
    WriteConsoleW(handle, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void ClearConsole(HANDLE handle)
{
    if (gAnsiColorSupported)
    {
        DWORD written = 0;
        WriteConsoleW(handle, L"\x1b[0m", 4, &written, nullptr);
    }

    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(handle, &info);
    const DWORD count = info.dwSize.X * info.dwSize.Y;
    DWORD written = 0;
    FillConsoleOutputCharacterW(handle, L' ', count, {0, 0}, &written);
    FillConsoleOutputAttribute(handle, 0, count, {0, 0}, &written);
    SetConsoleCursorPosition(handle, {0, 0});
    gLayeredFrameCache = {};
}

void WriteLayeredFrame(
    HANDLE output,
    const std::vector<std::wstring>& artLines,
    short left,
    short top,
    short width,
    short forceRefreshFirstLine = 0,
    short forceRefreshLastLine = -1)
{
    const bool mustRedrawAll = !gLayeredFrameCache.valid ||
        gLayeredFrameCache.left != left ||
        gLayeredFrameCache.top != top ||
        gLayeredFrameCache.width != width ||
        gLayeredFrameCache.lines.size() != artLines.size();

    // 처음 그릴 때, 창 크기가 달라졌을 때, 해상도가 달라졌을 때만 전체를 정리합니다.
    if (mustRedrawAll)
    {
        ClearConsole(output);
    }

    for (short lineIndex = 0; lineIndex < static_cast<short>(artLines.size()); ++lineIndex)
    {
        const bool forceRefresh = forceRefreshLastLine >= forceRefreshFirstLine &&
            lineIndex >= forceRefreshFirstLine && lineIndex <= forceRefreshLastLine;
        const bool lineChanged = mustRedrawAll ||
            forceRefresh ||
            gLayeredFrameCache.lines[static_cast<size_t>(lineIndex)] != artLines[static_cast<size_t>(lineIndex)];

        if (lineChanged)
        {
            WriteAt(output, left, static_cast<short>(top + lineIndex), artLines[static_cast<size_t>(lineIndex)]);
        }
    }

    gLayeredFrameCache.lines = artLines;
    gLayeredFrameCache.left = left;
    gLayeredFrameCache.top = top;
    gLayeredFrameCache.width = width;
    gLayeredFrameCache.valid = true;
}

COORD GetVisibleConsoleSize(HANDLE handle)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(handle, &info);
    return {static_cast<short>(info.srWindow.Right - info.srWindow.Left + 1),
            static_cast<short>(info.srWindow.Bottom - info.srWindow.Top + 1)};
}

int GetConsoleDisplayWidth(const std::wstring& text)
{
    int width = 0;
    for (const wchar_t character : text)
    {
        const bool isWideCharacter =
            (character >= 0x1100 && character <= 0x115F) ||
            (character >= 0x2E80 && character <= 0xA4CF) ||
            (character >= 0xAC00 && character <= 0xD7A3) ||
            (character >= 0xF900 && character <= 0xFAFF) ||
            (character >= 0xFE10 && character <= 0xFE6F) ||
            (character >= 0xFF01 && character <= 0xFF60) ||
            (character >= 0xFFE0 && character <= 0xFFE6);

        width += isWideCharacter ? 2 : 1;
    }

    return width;
}

// 컬러 AA에는 화면에 보이지 않는 ANSI 색상 제어 문자가 섞여 있습니다.
// UI 위치 계산에는 실제 보이는 글자 폭만 사용해야 합니다.
int GetVisibleConsoleDisplayWidth(const std::wstring& text)
{
    int width = 0;
    for (size_t index = 0; index < text.size(); ++index)
    {
        if (text[index] == L'\x1b' && index + 1 < text.size() && text[index + 1] == L'[')
        {
            index += 2;
            while (index < text.size() && text[index] != L'm')
            {
                ++index;
            }
            continue;
        }

        const wchar_t character = text[index];
        const bool isWideCharacter =
            (character >= 0x1100 && character <= 0x115F) ||
            (character >= 0x2E80 && character <= 0xA4CF) ||
            (character >= 0xAC00 && character <= 0xD7A3) ||
            (character >= 0xF900 && character <= 0xFAFF) ||
            (character >= 0xFE10 && character <= 0xFE6F) ||
            (character >= 0xFF01 && character <= 0xFF60) ||
            (character >= 0xFFE0 && character <= 0xFFE6);

        width += isWideCharacter ? 2 : 1;
    }
    return width;
}

ControlPanelLayout CreateControlPanelLayout(HANDLE handle, bool showDeveloperPanel)
{
    const COORD size = GetVisibleConsoleSize(handle);
    ControlPanelLayout layout;
    if (!showDeveloperPanel)
    {
        // 평상시 전투에서는 장면을 화면 맨 위부터 넓게 사용합니다.
        layout.artStartY = 0;
        return layout;
    }

    const bool horizontal = size.X >= kControlWidth * 3 + 4;

    layout.sliders[0] = {ESlider::OUTPUT_WIDTH, 2, 2, static_cast<short>(2 + kSliderLabelWidth)};
    if (horizontal)
    {
        layout.sliders[1] = {ESlider::HEIGHT_SCALE, static_cast<short>(2 + kControlWidth), 2,
                             static_cast<short>(2 + kControlWidth + kSliderLabelWidth)};
        layout.sliders[2] = {ESlider::CONTRAST, static_cast<short>(2 + kControlWidth * 2), 2,
                             static_cast<short>(2 + kControlWidth * 2 + kSliderLabelWidth)};
        layout.artStartY = 5;
    }
    else
    {
        layout.sliders[1] = {ESlider::HEIGHT_SCALE, 2, 3, static_cast<short>(2 + kSliderLabelWidth)};
        layout.sliders[2] = {ESlider::CONTRAST, 2, 4, static_cast<short>(2 + kSliderLabelWidth)};
        layout.artStartY = 7;
    }
    return layout;
}

int CalculateMaximumOutputPixelWidth(
    HANDLE handle,
    const RenderSettings& settings,
    const ControlPanelLayout& layout,
    const SceneConfig& config)
{
    const COORD size = GetVisibleConsoleSize(handle);
    const int rows = std::max(1, size.Y - layout.artStartY - 1);
    const double scale = GetCharacterHeightScale(settings);
    const int byHeight = static_cast<int>(rows * 4.0 * config.sceneWidth / (config.sceneHeight * scale));
    const int byWidth = std::max(1, size.X - 1) * 2; // 마지막 열은 자동 줄바꿈 방지용 여백
    int maximum = std::max(16, std::min(byHeight, byWidth));
    return maximum - maximum % 2;
}

std::wstring CreateSliderTrack(int value)
{
    const int position = ClampSetting(value) * kSliderWidth / 1000;
    std::wstring result = L"[";
    for (int i = 0; i <= kSliderWidth; ++i)
    {
        result += i == position ? L'O' : L'-';
    }
    return result + L"] " + std::to_wstring(ClampSetting(value));
}

void DrawSlider(HANDLE handle, const SliderLayout& layout, const wchar_t* label, int value)
{
    WriteAt(handle, layout.x, layout.y, label);
    WriteAt(handle, layout.sliderStartX, layout.y, CreateSliderTrack(value));
}

void DrawImageWithWhiteKey(
    Gdiplus::Graphics& graphics,
    Gdiplus::Image& image,
    const Gdiplus::RectF& destination,
    const Gdiplus::Rect& source)
{
    Gdiplus::ImageAttributes attributes;
    attributes.SetColorKey(Gdiplus::Color(245, 245, 245), Gdiplus::Color(255, 255, 255), Gdiplus::ColorAdjustTypeBitmap);
    graphics.DrawImage(&image, destination, static_cast<Gdiplus::REAL>(source.X), static_cast<Gdiplus::REAL>(source.Y),
                       static_cast<Gdiplus::REAL>(source.Width), static_cast<Gdiplus::REAL>(source.Height),
                       Gdiplus::UnitPixel, &attributes);
}

Gdiplus::RectF FitImageAspectRatio(const Gdiplus::RectF& maximumArea, const Gdiplus::Rect& source)
{
    const float scaleX = maximumArea.Width / source.Width;
    const float scaleY = maximumArea.Height / source.Height;
    const float scale = std::min(scaleX, scaleY);
    const float width = source.Width * scale;
    const float height = source.Height * scale;

    return {
        maximumArea.X + (maximumArea.Width - width) * 0.5f,
        maximumArea.Y + (maximumArea.Height - height) * 0.5f,
        width,
        height,
    };
}

void DrawWeapon(Gdiplus::Graphics& graphics, float x, float y, float swing)
{
    const float angle = -0.65f + swing * 1.35f;
    const float length = 118.0f;
    const float endX = x + std::cos(angle) * length;
    const float endY = y + std::sin(angle) * length;
    // 완전한 흰색은 점 변환에서 사라질 수 있어, 밝은 은색으로 그립니다.
    Gdiplus::Pen blade(Gdiplus::Color(255, 185, 195, 205), 10.0f);
    Gdiplus::Pen handle(Gdiplus::Color(255, 105, 115, 125), 16.0f);
    graphics.DrawLine(&handle, x - 11, y + 11, x + 11, y - 11);
    graphics.DrawLine(&blade, x, y, endX, endY);
}

// 기본 게임 쪽의 Monster 객체를 AA 모듈이 직접 알 필요는 없습니다.
// BattleSceneState에 이미 전달된 표시 이름으로 선택한 스프라이트 경로만 고릅니다.
std::wstring GetMonsterSpritePath(const AsciiArt::ActorBattleStatus& status, const SceneConfig& config)
{
    const std::wstring name = Utf8ToWide(status.displayName);
    if (name == L"\uC2AC\uB77C\uC784") return L"Resources\\Images\\Monsters\\slime_diagonal_white_eyes_v2_transparent.png";
    if (name == L"\uACE0\uBE14\uB9B0") return L"Resources\\Images\\Monsters\\goblin_diagonal_v4_transparent.png";
    if (name == L"\uC2A4\uCF08\uB808\uD1A4") return L"Resources\\Images\\Monsters\\skeleton_diagonal_v2_transparent.png";
    if (name == L"\uC880\uBE44") return L"Resources\\Images\\Monsters\\zombie_diagonal_v4_transparent.png";
    if (name == L"\uCF54\uBCFC\uD2B8") return L"Resources\\Images\\Monsters\\kobold_diagonal_v5_transparent.png";
    if (name == L"\uACE8\uB818") return L"Resources\\Images\\Monsters\\golem_diagonal_v1_transparent.png";
    if (name == L"\uBC29\uD669\uD558\uB294 \uAC11\uC637") return L"Resources\\Images\\Monsters\\wandering_armor_transparent.png";
    if (name == L"\uB4DC\uB77C\uD058\uB77C") return L"Resources\\Images\\Monsters\\dracula_diagonal_v3_transparent.png";
    if (name == L"\uB808\uB4DC \uB4DC\uB798\uACE4") return L"Resources\\Images\\Monsters\\red_dragon_diagonal_v1_transparent.png";
    return config.monsterImagePath;
}

struct MonsterVisualArea
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

enum class EMonsterVisualProfileIndex : size_t
{
    SLIME, GOBLIN, SKELETON, ZOMBIE, KOBOLD, GOLEM, WANDERING_ARMOR, DRACULA, RED_DRAGON,
};

EMonsterVisualProfileIndex GetMonsterVisualProfileIndex(const AsciiArt::ActorBattleStatus& status)
{
    const std::wstring name = Utf8ToWide(status.displayName);
    if (name == L"고블린") return EMonsterVisualProfileIndex::GOBLIN;
    if (name == L"스켈레톤") return EMonsterVisualProfileIndex::SKELETON;
    if (name == L"좀비") return EMonsterVisualProfileIndex::ZOMBIE;
    if (name == L"코볼트") return EMonsterVisualProfileIndex::KOBOLD;
    if (name == L"골렘") return EMonsterVisualProfileIndex::GOLEM;
    if (name == L"방황하는 갑옷") return EMonsterVisualProfileIndex::WANDERING_ARMOR;
    if (name == L"드라큘라") return EMonsterVisualProfileIndex::DRACULA;
    if (name == L"레드 드래곤") return EMonsterVisualProfileIndex::RED_DRAGON;
    return EMonsterVisualProfileIndex::SLIME;
}

const MonsterVisualProfile& GetMonsterVisualProfile(const SceneConfig& config, const AsciiArt::ActorBattleStatus& status)
{
    return config.monsterProfiles[static_cast<size_t>(GetMonsterVisualProfileIndex(status))];
}

const std::array<std::string, 9>& GetMonsterDisplayNames()
{
    static const std::array<std::string, 9> names = {
        u8"슬라임", u8"고블린", u8"스켈레톤", u8"좀비", u8"코볼트",
        u8"골렘", u8"방황하는 갑옷", u8"드라큘라", u8"레드 드래곤"
    };
    return names;
}

int GetSameTypeMonsterOrder(const AsciiArt::BattleSceneState& battleState, int statusIndex)
{
    if (statusIndex <= 0 || statusIndex >= static_cast<int>(battleState.monsterStatuses.size())) return 0;
    const EMonsterVisualProfileIndex type = GetMonsterVisualProfileIndex(battleState.monsterStatuses[statusIndex]);
    int order = 0;
    for (int index = 0; index < statusIndex; ++index)
    {
        if (GetMonsterVisualProfileIndex(battleState.monsterStatuses[index]) == type) ++order;
    }
    return order;
}

// 몬스터 종류별 기본값을 사용합니다. 같은 종류가 동시에 여러 마리일 때만 겹침 방지 오프셋을 더합니다.
MonsterVisualArea GetMonsterVisualArea(const SceneConfig& config, const AsciiArt::ActorBattleStatus& status, int sameTypeOrder = 0)
{
    const auto instance = gBattleMonsterInstanceProfiles.find(status.id);
    if (instance != gBattleMonsterInstanceProfiles.end())
    {
        const MonsterVisualProfile& profile = instance->second;
        return { profile.x, profile.y, profile.width, profile.height };
    }
    const MonsterVisualProfile& profile = GetMonsterVisualProfile(config, status);
    return { profile.x + sameTypeOrder * 46.0f, profile.y + sameTypeOrder * 24.0f,
             profile.width, profile.height };
}

// 전투에 실제 등장한 순서대로 1~4번 고정 슬롯을 사용합니다.
// 이제 몬스터 크기를 통일했으므로 종류보다 화면 대형을 우선하는 편이 배치에 더 적합합니다.
MonsterVisualProfile GetMonsterSlotProfile(
    const SceneConfig& config,
    int slotIndex,
    const AsciiArt::ActorBattleStatus& status)
{
    // 슬롯은 자리만, 몬스터 종류 설정은 크기만 담당합니다.
    const MonsterVisualProfile& typeProfile = GetMonsterVisualProfile(config, status);
    switch (slotIndex)
    {
    case 0: return { config.monsterX, config.monsterY, typeProfile.width, typeProfile.height, config.monsterLayer };
    case 1: return { config.monster2X, config.monster2Y, typeProfile.width, typeProfile.height, config.monster2Layer };
    case 2: return { config.monster3X, config.monster3Y, typeProfile.width, typeProfile.height, config.monster3Layer };
    default: return { config.monster4X, config.monster4Y, typeProfile.width, typeProfile.height, config.monster4Layer };
    }
}

void SaveMonsterSlotProfile(SceneConfig& config, int slotIndex, const MonsterVisualProfile& profile)
{
    if (slotIndex == 0) { config.monsterX = profile.x; config.monsterY = profile.y; config.monsterLayer = profile.layer; }
    else if (slotIndex == 1) { config.monster2X = profile.x; config.monster2Y = profile.y; config.monster2Layer = profile.layer; }
    else if (slotIndex == 2) { config.monster3X = profile.x; config.monster3Y = profile.y; config.monster3Layer = profile.layer; }
    else { config.monster4X = profile.x; config.monster4Y = profile.y; config.monster4Layer = profile.layer; }
}

void CopyBattleMonsterInstancesToSlots(SceneConfig& config, const AsciiArt::BattleSceneState& battleState)
{
    const int count = std::min(4, static_cast<int>(battleState.monsterStatuses.size()));
    for (int index = 0; index < count; ++index)
    {
        const auto found = gBattleMonsterInstanceProfiles.find(battleState.monsterStatuses[index].id);
        if (found != gBattleMonsterInstanceProfiles.end()) SaveMonsterSlotProfile(config, index, found->second);
    }
}

// 화면 슬롯은 전투 시작 시의 몬스터 개체 순서를 끝까지 유지합니다.
// 따라서 죽은 슬롯을 건너뛰되, 살아 있는 몬스터의 인덱스는 앞으로 당기지 않습니다.
int FindNextLivingMonsterIndex(const AsciiArt::BattleSceneState& battleState, int startIndex)
{
    const int count = static_cast<int>(battleState.monsterStatuses.size());
    for (int index = std::max(0, startIndex); index < count; ++index)
    {
        if (!battleState.monsterStatuses[index].isDead)
        {
            return index;
        }
    }
    return -1;
}

// PNG의 투명/흰색 여백은 체력바 기준에서 제외합니다.
// 결과는 이미지 포인터별로 한 번만 계산하므로 프레임마다 전체 픽셀을 다시 읽지 않습니다.
float GetVisibleTopRatio(Gdiplus::Image& image)
{
    static std::map<Gdiplus::Image*, float> cachedTopRatios;
    const auto cached = cachedTopRatios.find(&image);
    if (cached != cachedTopRatios.end())
    {
        return cached->second;
    }

    const UINT width = image.GetWidth();
    const UINT height = image.GetHeight();
    if (width == 0 || height == 0)
    {
        return 0.0f;
    }

    Gdiplus::Bitmap raster(width, height, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&raster);
    graphics.DrawImage(&image, 0, 0, static_cast<INT>(width), static_cast<INT>(height));

    UINT topPixel = 0;
    bool found = false;
    for (UINT y = 0; y < height && !found; ++y)
    {
        for (UINT x = 0; x < width; x += 2)
        {
            Gdiplus::Color color{};
            if (raster.GetPixel(x, y, &color) != Gdiplus::Ok)
            {
                continue;
            }

            // DrawImageWithWhiteKey와 동일하게 거의 흰색은 빈 배경으로 봅니다.
            const bool isWhiteKey = color.GetRed() > 245 && color.GetGreen() > 245 && color.GetBlue() > 245;
            if (color.GetAlpha() > 16 && !isWhiteKey)
            {
                topPixel = y;
                found = true;
                break;
            }
        }
    }

    const float ratio = found ? static_cast<float>(topPixel) / static_cast<float>(height) : 0.0f;
    cachedTopRatios.emplace(&image, ratio);
    return ratio;
}

// 설정 파일에 무기 PNG가 있으면 기본 선 무기 대신 사용합니다.
// swing은 평소 0이고, 공격하는 짧은 시간에만 커져서 무기가 휘둘러집니다.
void DrawWeaponImage(Gdiplus::Graphics& graphics, Gdiplus::Image& image, float x, float y, float width, float height, float swing)
{
    const Gdiplus::Rect source(0, 0, static_cast<int>(image.GetWidth()), static_cast<int>(image.GetHeight()));
    if (source.Width <= 0 || source.Height <= 0)
    {
        return;
    }

    const Gdiplus::GraphicsState state = graphics.Save();
    const float angle = -22.0f + swing * 82.0f;
    graphics.TranslateTransform(x, y);
    graphics.RotateTransform(angle);
    const Gdiplus::RectF area(-width * 0.50f, -height * 0.50f, width, height);
    DrawImageWithWhiteKey(graphics, image, FitImageAspectRatio(area, source), source);
    graphics.Restore(state);
}

void DrawHitEffect(Gdiplus::Graphics& graphics, float x, float y, float progress)
{
    const float radius = 45.0f + progress * 95.0f;
    Gdiplus::Pen slash(Gdiplus::Color(255, 255, 255, 255), 9.0f);
    graphics.DrawLine(&slash, x - radius, y + radius * 0.55f, x + radius, y - radius * 0.55f);
    graphics.DrawLine(&slash, x - radius * 0.75f, y - radius * 0.20f, x + radius * 0.75f, y + radius * 0.20f);
}

// PNG 하나를 회전해 재사용하므로 30/45/55도 타격 이펙트를 따로 관리할 필요가 없습니다.
void DrawEffectImage(
    Gdiplus::Graphics& graphics,
    Gdiplus::Image& image,
    float centerX,
    float centerY,
    float width,
    float height,
    float angle)
{
    const Gdiplus::Rect source(0, 0, static_cast<int>(image.GetWidth()), static_cast<int>(image.GetHeight()));
    if (source.Width <= 0 || source.Height <= 0)
    {
        return;
    }

    const Gdiplus::GraphicsState state = graphics.Save();
    graphics.TranslateTransform(centerX, centerY);
    graphics.RotateTransform(angle);
    const Gdiplus::RectF area(-width * 0.5f, -height * 0.5f, width, height);
    DrawImageWithWhiteKey(graphics, image, FitImageAspectRatio(area, source), source);
    graphics.Restore(state);
}

void DrawHealthBar(
    Gdiplus::Graphics& graphics,
    const AsciiArt::ActorBattleStatus& status,
    float shownHp,
    float x,
    float y,
    float width)
{
    if (status.maximumHp <= 0) return;
    const float maxHp = static_cast<float>(status.maximumHp);
    const float currentRatio = std::clamp(static_cast<float>(status.currentHp) / maxHp, 0.0f, 1.0f);
    // shownHp는 이전 체력에서 실제 현재 체력으로 빠르게 따라오는 애니메이션 값입니다.
    // 따라서 초록 뒤에 남은 구간만 빨강으로 그리면, 빨강이 초록 끝까지 줄어드는 연출이 됩니다.
    const float delayedRatio = std::clamp(shownHp / maxHp, 0.0f, 1.0f);
    Gdiplus::SolidBrush back(Gdiplus::Color(245, 50, 53, 58));
    Gdiplus::SolidBrush current(Gdiplus::Color(255, 35, 235, 65));
    Gdiplus::SolidBrush damage(Gdiplus::Color(255, 238, 62, 65));
    Gdiplus::Pen border(Gdiplus::Color(255, 205, 215, 220), 2.0f);
    constexpr float kHeight = 14.0f;
    constexpr float kSlant = 9.0f;
    graphics.FillRectangle(&back, x, y, width, kHeight);

    const float greenWidth = width * currentRatio;
    if (greenWidth > 0.0f)
    {
        const Gdiplus::PointF points[] = {
            {x, y}, {x + greenWidth, y},
            {x + std::max(0.0f, greenWidth - kSlant), y + kHeight}, {x, y + kHeight},
        };
        graphics.FillPolygon(&current, points, 4);
    }

    if (delayedRatio > currentRatio)
    {
        const float redStart = x + greenWidth;
        const float redEnd = x + width * delayedRatio;
        const Gdiplus::PointF points[] = {
            {redStart, y}, {redEnd, y},
            {std::max(redStart, redEnd - kSlant), y + kHeight}, {std::max(redStart - kSlant, x), y + kHeight},
        };
        graphics.FillPolygon(&damage, points, 4);
    }
    graphics.DrawRectangle(&border, x, y, width, kHeight);
}

void DrawFloatingCombatText(
    Gdiplus::Graphics& graphics,
    const AsciiArt::BattleSceneState& battleState,
    const std::string& actorId,
    float x,
    float y)
{
    constexpr double kDuration = 0.72;
    if (battleState.floatingTextTargetId != actorId ||
        battleState.floatingTextValue == 0 ||
        battleState.floatingTextAgeSeconds >= kDuration)
    {
        return;
    }

    const float progress = static_cast<float>(battleState.floatingTextAgeSeconds / kDuration);
    const BYTE alpha = static_cast<BYTE>(std::clamp((1.0f - progress) * 255.0f, 0.0f, 255.0f));
    const bool isPositive = battleState.floatingTextIsHealing || battleState.floatingTextIsPowerBuff;
    const std::wstring text = std::wstring(isPositive ? L"+" : L"-") +
        std::to_wstring(battleState.floatingTextValue);
    Gdiplus::FontFamily family(L"Consolas");
    Gdiplus::Font font(&family, 28.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brush(battleState.floatingTextIsPowerBuff
        ? Gdiplus::Color(alpha, 255, 225, 45)
        : (battleState.floatingTextIsHealing
            ? Gdiplus::Color(alpha, 105, 255, 105)
            : Gdiplus::Color(alpha, 255, 60, 70)));
    graphics.DrawString(text.c_str(), -1, &font,
        Gdiplus::PointF(x, y - 45.0f - progress * 30.0f), &brush);
}

// 피해/회복 수치는 이미지 캔버스가 아니라 콘솔 위에 직접 출력합니다.
// 그래서 Braille 점으로 바뀌지 않고 일반 문자 색상과 상승 애니메이션을 유지할 수 있습니다.
void DrawFloatingCombatTextOverlay(
    HANDLE output,
    const AsciiArt::BattleSceneState& battleState,
    const SceneConfig& config,
    const ControlPanelLayout& layout,
    const ArtResolution& resolution)
{
    constexpr double kDuration = 0.72;
    if (battleState.floatingTextTargetId.empty() || battleState.floatingTextValue == 0 ||
        battleState.floatingTextAgeSeconds >= kDuration)
    {
        return;
    }

    float sceneX = 0.0f;
    float sceneY = 0.0f;
    float actorWidth = 0.0f;
    bool found = false;
    for (size_t index = 0; index < battleState.playerStatuses.size() && index < 3; ++index)
    {
        if (battleState.playerStatuses[index].id != battleState.floatingTextTargetId) continue;
        if (index == 0) { sceneX = config.heroX; sceneY = config.heroY; actorWidth = config.heroWidth; }
        else if (index == 1) { sceneX = config.hero2X; sceneY = config.hero2Y; actorWidth = config.hero2Width; }
        else { sceneX = config.tankX; sceneY = config.tankY; actorWidth = config.tankWidth; }
        found = true;
        break;
    }
    if (!found)
    {
        for (size_t index = 0; index < battleState.monsterStatuses.size(); ++index)
        {
            if (battleState.monsterStatuses[index].id != battleState.floatingTextTargetId) continue;
            const int sameTypeOrder = GetSameTypeMonsterOrder(battleState, static_cast<int>(index));
            const MonsterVisualArea area = GetMonsterVisualArea(config, battleState.monsterStatuses[index], sameTypeOrder);
            sceneX = area.x;
            sceneY = area.y;
            actorWidth = area.width;
            found = true;
            break;
        }
    }
    if (!found) return;

    const float progress = static_cast<float>(battleState.floatingTextAgeSeconds / kDuration);
    const bool isPositive = battleState.floatingTextIsHealing || battleState.floatingTextIsPowerBuff;
    const std::wstring text = battleState.floatingTextIsPowerBuff
        ? L"⚔ + " + std::to_wstring(battleState.floatingTextValue)
        : std::wstring(isPositive ? L"+ " : L"- ") + std::to_wstring(battleState.floatingTextValue);
    const float actorCenterSceneX = sceneX + actorWidth * 0.5f;
    const int actorCenterConsoleX = static_cast<int>(actorCenterSceneX * resolution.pixelWidth / (2.0f * config.sceneWidth));
    const float barSceneY = std::max(0.0f, sceneY - 17.0f);
    const short textX = static_cast<short>(std::clamp(actorCenterConsoleX - static_cast<int>(text.size()) / 2, 0, 32767));
    // 체력바 중앙에서 짧게 위로 올라가는 피해/회복 수치입니다.
    const short textY = static_cast<short>(std::clamp(
        layout.artStartY + static_cast<int>(barSceneY * resolution.pixelHeight / (4.0f * config.sceneHeight)) - 2 - static_cast<int>(progress * 2.0f),
        0, 32767));
    const bool bright = progress < 0.52f;
    const WORD color = battleState.floatingTextIsPowerBuff
        ? static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | (bright ? FOREGROUND_INTENSITY : 0))
        : (battleState.floatingTextIsHealing
            ? static_cast<WORD>(FOREGROUND_GREEN | (bright ? FOREGROUND_INTENSITY : 0))
            : static_cast<WORD>(FOREGROUND_RED | (bright ? FOREGROUND_INTENSITY : 0)));
    SetConsoleCursorPosition(output, {textX, textY});
    SetConsoleTextAttribute(output, color);
    DWORD written = 0;
    WriteConsoleW(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
    SetConsoleTextAttribute(output, kTextColor);
}

// 체력 바와 턴 표시는 이미지 캔버스에 그리지 않습니다.
// 콘솔 문자 한 줄을 직접 색칠해야 Braille 점 여러 줄로 분해되지 않습니다.
void DrawBattleHudOverlay(
    HANDLE output,
    const AsciiArt::BattleSceneState& battleState,
    const SceneConfig& config,
    const ControlPanelLayout& layout,
    const ArtResolution& resolution,
    const std::map<std::string, float>& displayedHp,
    const std::vector<Gdiplus::Image*>& monsterImages,
    bool showHealEffectPreview,
    bool showPowerBuffEffectPreview,
    bool autoBattleEnabled)
{
    constexpr int kBarLength = 24;
    // Braille 한 글자는 콘솔에서는 한 칸이므로, 점 느낌을 유지하면서도 체력바는 한 줄입니다.
    constexpr wchar_t kBarCharacter[] = L"⠿";

    const auto drawBar = [&](const AsciiArt::ActorBattleStatus& status, float sceneX, float sceneY, float actorWidth, float gapAboveActor)
    {
        if (status.maximumHp <= 0)
        {
            return;
        }

        const float maximumHp = static_cast<float>(status.maximumHp);
        const float currentRatio = std::clamp(static_cast<float>(status.currentHp) / maximumHp, 0.0f, 1.0f);
        const auto shown = displayedHp.find(status.id);
        const float shownHp = shown == displayedHp.end() ? static_cast<float>(status.currentHp) : shown->second;
        const float delayedRatio = std::clamp(shownHp / maximumHp, currentRatio, 1.0f);
        const int currentLength = static_cast<int>(std::round(currentRatio * kBarLength));
        const int delayedLength = static_cast<int>(std::round(delayedRatio * kBarLength));

        const float actorCenterSceneX = sceneX + actorWidth * 0.5f;
        const int actorCenterConsoleX = static_cast<int>(actorCenterSceneX * resolution.pixelWidth / (2.0f * config.sceneWidth));
        const int barX = std::max(0, actorCenterConsoleX - kBarLength / 2);
        const int barSceneY = std::max(0, static_cast<int>(sceneY - gapAboveActor));
        const short barY = static_cast<short>(std::clamp(
            layout.artStartY + static_cast<int>(barSceneY * resolution.pixelHeight / (4.0f * config.sceneHeight)), 0, 32767));

        for (int index = 0; index < kBarLength; ++index)
        {
            WORD color = static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            if (index < currentLength)
            {
                color = static_cast<WORD>(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }
            else if (index < delayedLength)
            {
                color = static_cast<WORD>(FOREGROUND_RED | FOREGROUND_INTENSITY);
            }

            SetConsoleCursorPosition(output, {static_cast<short>(barX + index), barY});
            SetConsoleTextAttribute(output, color);
            DWORD written = 0;
            WriteConsoleW(output, kBarCharacter, 1, &written, nullptr);
        }
        if (status.isPowerBuffed && barX >= 2)
        {
            SetConsoleCursorPosition(output, {static_cast<short>(barX - 2), barY});
            SetConsoleTextAttribute(output, static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            DWORD written = 0;
            WriteConsoleW(output, L"⚔", 1, &written, nullptr);
        }
    };

    const size_t playerCount = std::min<size_t>(battleState.playerStatuses.size(), 3);
    for (size_t index = 0; index < playerCount; ++index)
    {
        if (index == 0) drawBar(battleState.playerStatuses[index], config.heroX, config.heroY, config.heroWidth, 17.0f);
        else if (index == 1) drawBar(battleState.playerStatuses[index], config.hero2X, config.hero2Y, config.hero2Width, 17.0f);
        else drawBar(battleState.playerStatuses[index], config.tankX, config.tankY, config.tankWidth, 17.0f);
    }
    for (size_t index = 0; index < battleState.monsterStatuses.size(); ++index)
    {
        if (battleState.monsterStatuses[index].isDead) continue;
        const int sameTypeOrder = GetSameTypeMonsterOrder(battleState, static_cast<int>(index));
        const MonsterVisualArea area = GetMonsterVisualArea(config, battleState.monsterStatuses[index], sameTypeOrder);
        Gdiplus::Image* image = index < monsterImages.size() ? monsterImages[index] : nullptr;
        const float visibleTop = area.y + (image == nullptr ? 0.0f : area.height * GetVisibleTopRatio(*image));
        // 실제 보이는 머리 위에서 콘솔 한두 줄 정도만 띄웁니다.
        drawBar(battleState.monsterStatuses[index], area.x, visibleTop, area.width, 8.0f);
    }

    const std::wstring turnText = battleState.isMonsterTurn
        ? L"[ MONSTER TURN ]"
        : L"[ PLAYER TURN ]";
    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(output, &info);
    const short turnX = static_cast<short>(std::max(0, (info.dwSize.X - static_cast<short>(turnText.size())) / 2));
    SetConsoleCursorPosition(output, {turnX, static_cast<short>(layout.artStartY)});
    SetConsoleTextAttribute(output, battleState.isMonsterTurn
        ? static_cast<WORD>(FOREGROUND_RED | FOREGROUND_INTENSITY)
        : static_cast<WORD>(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
    DWORD written = 0;
    WriteConsoleW(output, turnText.c_str(), static_cast<DWORD>(turnText.size()), &written, nullptr);
    SetConsoleTextAttribute(output, kTextColor);

    // 실제 전투 코드가 마지막으로 전달한 행동 문구입니다.
    // 턴 문구와 섞이지 않도록 두 줄 아래에서 다음 행동 전까지 유지합니다.
    const std::wstring actionText = Utf8ToWide(battleState.turnActorName);
    if (!actionText.empty())
    {
        const short actionX = static_cast<short>(std::max(0, (info.dwSize.X - static_cast<short>(actionText.size())) / 2));
        SetConsoleCursorPosition(output, {actionX, static_cast<short>(layout.artStartY + 2)});
        SetConsoleTextAttribute(output, kTextColor);
        WriteConsoleW(output, actionText.c_str(), static_cast<DWORD>(actionText.size()), &written, nullptr);
    }

    // 행동 결과는 행동 문구와 한 줄 띄운 다음 줄에 남깁니다.
    // 기존 RpgLogger의 즉시 출력과 달리, 다음 행동이 오기 전까지 이 위치에서 유지됩니다.
    const std::wstring resultText = Utf8ToWide(battleState.actionResultText);
    if (!resultText.empty())
    {
        const short resultX = static_cast<short>(std::max(0, (info.dwSize.X - static_cast<short>(resultText.size())) / 2));
        SetConsoleCursorPosition(output, {resultX, static_cast<short>(layout.artStartY + 4)});
        const WORD resultColor = battleState.actionResultIsPowerBuff
            ? static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
            : (battleState.actionResultIsHealing
                ? static_cast<WORD>(FOREGROUND_GREEN | FOREGROUND_INTENSITY)
                : static_cast<WORD>(FOREGROUND_RED | FOREGROUND_INTENSITY));
        SetConsoleTextAttribute(output, resultColor);
        WriteConsoleW(output, resultText.c_str(), static_cast<DWORD>(resultText.size()), &written, nullptr);
        SetConsoleTextAttribute(output, kTextColor);
    }

    if (battleState.showTestControls)
    {
        // 테스트 중에도 사라지지 않는 숫자 조작표입니다.
        const std::array<std::wstring, 9> kTestControls = {
            L"[ 전투 테스트 조작 ]",
            L"1. HP 포션 사용",
            L"2. 공격력 버프 사용",
            L"3. 아군 전체 공격",
            L"4. 몬스터 전체 공격",
            std::wstring(L"5. 회복 이펙트: ") + (showHealEffectPreview ? L"표시" : L"숨김"),
            std::wstring(L"6. 버프 이펙트: ") + (showPowerBuffEffectPreview ? L"표시" : L"숨김"),
            std::wstring(L"7. 자동 전투: ") + (autoBattleEnabled ? L"켜짐" : L"꺼짐"),
            L"8. 같은 몬스터 4마리 배치 테스트 (Tab/방향키: 종류 변경, 8: 종료)",
        };
        for (size_t index = 0; index < kTestControls.size(); ++index)
        {
            const short controlY = static_cast<short>(layout.artStartY + static_cast<short>(index));
            SetConsoleCursorPosition(output, {2, controlY});
            SetConsoleTextAttribute(output, index == 0
                ? static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
                : kTextColor);
            const DWORD controlLength = static_cast<DWORD>(kTestControls[index].size());
            WriteConsoleW(output, kTestControls[index].c_str(), controlLength, &written, nullptr);
        }
        SetConsoleTextAttribute(output, kTextColor);
    }
}

void RenderScene(
    Gdiplus::Bitmap& canvas,
    Gdiplus::Image* backgroundImage,
    Gdiplus::Image& heroImage,
    Gdiplus::Image* hero2Image,
    Gdiplus::Image* tankImage,
    Gdiplus::Image& fallbackMonsterImage,
    const std::vector<Gdiplus::Image*>& monsterImages,
    Gdiplus::Image* warriorWeaponImage,
    Gdiplus::Image* mageWeaponImage,
    Gdiplus::Image* tankWeaponImage,
    const std::array<Gdiplus::Image*, 3>& monsterHitEffectImages,
    const std::array<Gdiplus::Image*, 3>& heroHitEffectImages,
    Gdiplus::Image* healEffectImage,
    Gdiplus::Image* powerBuffEffectImage,
    const SceneConfig& config,
    const PlacementMode& placement,
    const AttackAnimation& attack,
    const AsciiArt::BattleSceneState* battleState,
    const std::map<std::string, float>& displayedHp,
    double timeSeconds,
    bool showHealEffectPreview,
    bool showPowerBuffEffectPreview)
{
    Gdiplus::Graphics graphics(&canvas);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.Clear(Gdiplus::Color(255, 0, 0, 0));
    if (backgroundImage != nullptr)
    {
        graphics.DrawImage(backgroundImage, 0, 0, static_cast<INT>(canvas.GetWidth()), static_cast<INT>(canvas.GetHeight()));
    }

    double attackTime = attack.playing ? GetElapsedSeconds(attack.startedAt) : -1.0;
    const bool effectVisible = !attack.playerUsingPotion && attackTime >= 0.18 && attackTime < 0.44;
    const bool monsterHit = !attack.monsterAttacking && attackTime >= 0.22 && attackTime < 0.62;
    const bool heroHit = attack.monsterAttacking && attackTime >= 0.22 && attackTime < 0.62;
    if (attack.playing && attackTime >= 0.72)
    {
        attackTime = -1.0;
    }

    float heroAdvance = 0.0f;
    float weaponSwing = 0.0f;
    if (!attack.playerUsingPotion && attackTime >= 0.0 && attackTime < 0.18)
    {
        heroAdvance = SmoothStep(static_cast<float>(attackTime / 0.18)) * config.heroAttackAdvanceRange;
        weaponSwing = static_cast<float>(attackTime / 0.18) * 0.55f;
    }
    else if (attackTime >= 0.18 && attackTime < 0.38)
    {
        heroAdvance = config.heroAttackAdvanceRange;
        weaponSwing = 0.55f + SmoothStep(static_cast<float>((attackTime - 0.18) / 0.20)) * 0.65f;
    }
    else if (attackTime >= 0.38 && attackTime < 0.62)
    {
        heroAdvance = (1.0f - SmoothStep(static_cast<float>((attackTime - 0.38) / 0.24))) * config.heroAttackAdvanceRange;
        weaponSwing = 1.20f;
    }

    if (attack.playerUsingPotion)
    {
        heroAdvance = 0.0f;
        weaponSwing = 0.0f;
    }

    const float breatheTime = static_cast<float>(timeSeconds * config.breatheSpeedScale);
    const float breatheAmount = config.breatheMotionScale;
    const float heroBreathX = std::sin(breatheTime * 1.3f + 0.8f) * config.heroBreatheHorizontalRange * breatheAmount;
    const float heroBreathY = std::sin(breatheTime * 2.2f) * config.heroBreatheVerticalRange * breatheAmount;
    const float hero2BreathX = std::sin(breatheTime * 1.4f + 2.1f) * config.heroBreatheHorizontalRange * breatheAmount;
    const float hero2BreathY = std::sin(breatheTime * 2.0f + 1.4f) * config.heroBreatheVerticalRange * breatheAmount;
    const float hero1AttackAdvance = !attack.monsterAttacking && attack.attackerIndex == 0 ? heroAdvance : 0.0f;
    const float hero2AttackAdvance = !attack.monsterAttacking && attack.attackerIndex == 1 ? heroAdvance : 0.0f;
    const float tankAttackAdvance = !attack.monsterAttacking && attack.attackerIndex == 2 ? heroAdvance : 0.0f;
    const float monsterAttackAdvance = attack.monsterAttacking ? -heroAdvance : 0.0f;
    const float heroShake = heroHit ? std::sin(static_cast<float>((attackTime - 0.22) * 65.0)) * config.monsterHitShakeRange : 0.0f;
    const int monsterCount = std::max(1, battleState == nullptr ? 0 : static_cast<int>(battleState->monsterStatuses.size()));

    // 이펙트의 중심은 저장된 고정 좌표가 아니라 실제 맞거나 회복한 대상의 중심을 따릅니다.
    float hitEffectX = config.hitEffectX;
    float hitEffectY = config.hitEffectY;
    if (effectVisible)
    {
        if (attack.monsterAttacking)
        {
            const int target = std::clamp(attack.targetIndex, 0, 2);
            if (target == 0) { hitEffectX = config.heroX + config.heroWidth * 0.5f; hitEffectY = config.heroY + config.heroHeight * 0.55f; }
            else if (target == 1) { hitEffectX = config.hero2X + config.hero2Width * 0.5f; hitEffectY = config.hero2Y + config.hero2Height * 0.55f; }
            else { hitEffectX = config.tankX + config.tankWidth * 0.5f; hitEffectY = config.tankY + config.tankHeight * 0.55f; }
        }
        else
        {
            const int targetIndex = std::clamp(attack.targetIndex, 0, monsterCount - 1);
            const MonsterVisualArea target = GetMonsterVisualArea(
                config, battleState->monsterStatuses[targetIndex], GetSameTypeMonsterOrder(*battleState, targetIndex));
            hitEffectX = target.x + target.width * 0.5f;
            hitEffectY = target.y + target.height * 0.55f;
        }
    }

    std::vector<SceneObject> objects = {
        {EObjectType::HERO, config.heroX + hero1AttackAdvance + heroBreathX + heroShake, config.heroY + heroBreathY, config.heroWidth, config.heroHeight, config.heroLayer, 0, 0, 0},
        // 장비는 평소에도 보이며, 자기 차례의 공격 중에만 본체와 함께 전진/회전합니다.
        {EObjectType::WARRIOR_WEAPON, config.warriorWeaponX + hero1AttackAdvance, config.warriorWeaponY + heroBreathY,
         config.warriorWeaponWidth, config.warriorWeaponHeight, config.warriorWeaponLayer, 0, 0, 0},
        {EObjectType::MAGE_WEAPON, config.mageWeaponX + hero2AttackAdvance, config.mageWeaponY + hero2BreathY,
         config.mageWeaponWidth, config.mageWeaponHeight, config.mageWeaponLayer, 0, 0, 0},
        {EObjectType::HIT_EFFECT, hitEffectX, hitEffectY, config.hitEffectWidth, config.hitEffectHeight,
         config.hitEffectLayer, 0, 0, static_cast<float>(attack.hitEffectVariant), effectVisible},
    };
    for (int index = 0; index < monsterCount; ++index)
    {
        if (battleState != nullptr && battleState->monsterStatuses[index].isDead) continue;
        const MonsterVisualArea area = GetMonsterVisualArea(
            config, battleState->monsterStatuses[index], GetSameTypeMonsterOrder(*battleState, index));
        const float breathY = std::sin(breatheTime * 1.8f + 1.3f + index * 0.91f) * config.monsterBreatheVerticalRange * breatheAmount;
        const float breathX = std::sin(breatheTime * 1.2f + 0.4f + index * 1.17f) * config.monsterBreatheHorizontalRange * breatheAmount;
        const bool isHitTarget = monsterHit && attack.targetIndex == index;
        const bool isAttacker = attack.monsterAttacking && attack.attackerIndex == index;
        const float shake = isHitTarget ? std::sin(static_cast<float>((attackTime - 0.22) * 65.0)) * config.monsterHitShakeRange : 0.0f;
        const float advance = isAttacker ? -heroAdvance : 0.0f;
        objects.push_back({ EObjectType::MONSTER, area.x + breathX + shake + advance, area.y + breathY,
                            area.width, area.height, GetMonsterVisualProfile(config, battleState->monsterStatuses[index]).layer,
                            0, 0, static_cast<float>(index) });
    }
    if (hero2Image != nullptr)
    {
        objects.push_back({EObjectType::HERO2, config.hero2X + hero2AttackAdvance + hero2BreathX, config.hero2Y + hero2BreathY, config.hero2Width, config.hero2Height,
                           config.hero2Layer, 0, 0, 0});
    }
    if (tankImage != nullptr)
    {
        objects.push_back({EObjectType::TANK, config.tankX + tankAttackAdvance + heroBreathX, config.tankY + heroBreathY,
                           config.tankWidth, config.tankHeight, config.tankLayer, 0, 0, 0});
    }
    if (tankWeaponImage != nullptr)
    {
        // 방패는 탱커와 같은 숨쉬기/공격 이동을 하고, 본체보다 항상 뒤 레이어에 놓입니다.
        objects.push_back({EObjectType::TANK_SHIELD, config.tankShieldX + tankAttackAdvance + heroBreathX, config.tankShieldY + heroBreathY,
                           config.tankShieldWidth, config.tankShieldHeight, config.tankLayer - 1, 0, 0, 0});
    }
    // 회복/버프 효과는 실제 사용 중에는 대상에게, 5·6번 미리 보기 중에는 지정한 기준 영웅에게 보입니다.
    const bool actualHealEffect = battleState != nullptr && battleState->floatingTextIsHealing &&
        battleState->floatingTextAgeSeconds >= 0.0 && battleState->floatingTextAgeSeconds < 0.75;
    const bool actualPowerEffect = battleState != nullptr && battleState->floatingTextIsPowerBuff &&
        battleState->floatingTextAgeSeconds >= 0.0 && battleState->floatingTextAgeSeconds < 0.75;
    const auto addSupportEffect = [&](EObjectType effectType, bool visible, Gdiplus::Image* image, bool useMagePreview)
    {
        if (!visible || image == nullptr) return;
        const bool isPreview = !actualHealEffect && !actualPowerEffect;
        const bool previewOnMage = isPreview && useMagePreview && hero2Image != nullptr;
        float effectX = previewOnMage ? config.hero2X + config.hero2Width * 0.5f : config.heroX + config.heroWidth * 0.5f;
        float effectY = previewOnMage ? config.hero2Y + config.hero2Height * 0.44f : config.heroY + config.heroHeight * 0.44f;
        float targetHeight = previewOnMage ? config.hero2Height : config.heroHeight;
        const auto player = std::find_if(battleState->playerStatuses.begin(), battleState->playerStatuses.end(),
            [&](const AsciiArt::ActorBattleStatus& status) { return status.id == battleState->floatingTextTargetId; });
        if ((actualHealEffect || actualPowerEffect) && player != battleState->playerStatuses.end())
        {
            const int index = static_cast<int>(std::distance(battleState->playerStatuses.begin(), player));
            if (index == 1)
            {
                effectX = config.hero2X + config.hero2Width * 0.5f;
                effectY = config.hero2Y + config.hero2Height * 0.44f;
                targetHeight = config.hero2Height;
            }
            else if (index >= 2)
            {
                effectX = config.tankX + config.tankWidth * 0.5f;
                effectY = config.tankY + config.tankHeight * 0.44f;
                targetHeight = config.tankHeight;
            }
        }
        constexpr float kReferenceCharacterHeight = 273.0f;
        const float effectScale = std::max(0.1f, targetHeight / kReferenceCharacterHeight);
        const bool isPower = effectType == EObjectType::POWER_BUFF_EFFECT;
        const float offsetX = isPower ? config.powerBuffEffectOffsetX : config.healEffectOffsetX;
        const float offsetY = isPower ? config.powerBuffEffectOffsetY : config.healEffectOffsetY;
        const float width = isPower ? config.powerBuffEffectWidth : config.healEffectWidth;
        const float height = isPower ? config.powerBuffEffectHeight : config.healEffectHeight;
        objects.push_back({effectType, effectX + offsetX, effectY + offsetY, width * effectScale, height * effectScale,
                           config.hitEffectLayer + 1, 0, 0, 0, true});
    };
    if (battleState != nullptr)
    {
        addSupportEffect(EObjectType::HEAL_EFFECT, actualHealEffect || showHealEffectPreview, healEffectImage, false);
        addSupportEffect(EObjectType::POWER_BUFF_EFFECT, actualPowerEffect || showPowerBuffEffectPreview, powerBuffEffectImage, true);
    }
    std::sort(objects.begin(), objects.end(), [](const SceneObject& left, const SceneObject& right) { return left.layer < right.layer; });

    for (const SceneObject& object : objects)
    {
        if (!object.visible)
        {
            continue;
        }

        switch (object.type)
        {
        case EObjectType::HERO:
        {
            const Gdiplus::Rect source(0, 0, static_cast<int>(heroImage.GetWidth()), static_cast<int>(heroImage.GetHeight()));
            const Gdiplus::RectF area(object.x, object.y, object.width, object.height);
            DrawImageWithWhiteKey(graphics, heroImage, FitImageAspectRatio(area, source), source);
            break;
        }
        case EObjectType::HERO2:
        {
            const Gdiplus::Rect source(0, 0, static_cast<int>(hero2Image->GetWidth()), static_cast<int>(hero2Image->GetHeight()));
            const Gdiplus::RectF area(object.x, object.y, object.width, object.height);
            DrawImageWithWhiteKey(graphics, *hero2Image, FitImageAspectRatio(area, source), source);
            break;
        }
        case EObjectType::MONSTER:
        {
            const int monsterIndex = static_cast<int>(object.phase);
            Gdiplus::Image* image = monsterIndex >= 0 && monsterIndex < static_cast<int>(monsterImages.size()) &&
                monsterImages[monsterIndex] != nullptr ? monsterImages[monsterIndex] : &fallbackMonsterImage;
            const Gdiplus::Rect source(0, 0, static_cast<int>(image->GetWidth()), static_cast<int>(image->GetHeight()));
            const Gdiplus::RectF area(object.x, object.y, object.width, object.height);
            DrawImageWithWhiteKey(graphics, *image, FitImageAspectRatio(area, source), source);
            break;
        }
        case EObjectType::WARRIOR_WEAPON:
        {
            const float swing = !attack.monsterAttacking && attack.attackerIndex == 0 ? weaponSwing : 0.0f;
            if (warriorWeaponImage != nullptr)
                DrawWeaponImage(graphics, *warriorWeaponImage, object.x, object.y, object.width, object.height, swing);
            else
                DrawWeapon(graphics, object.x, object.y, swing);
            break;
        }
        case EObjectType::MAGE_WEAPON:
        {
            const float swing = !attack.monsterAttacking && attack.attackerIndex == 1 ? weaponSwing : 0.0f;
            if (mageWeaponImage != nullptr)
                DrawWeaponImage(graphics, *mageWeaponImage, object.x, object.y, object.width, object.height, swing);
            else
                DrawWeapon(graphics, object.x, object.y, swing);
            break;
        }
        case EObjectType::TANK:
        {
            const Gdiplus::Rect source(0, 0, static_cast<int>(tankImage->GetWidth()), static_cast<int>(tankImage->GetHeight()));
            const Gdiplus::RectF area(object.x, object.y, object.width, object.height);
            DrawImageWithWhiteKey(graphics, *tankImage, FitImageAspectRatio(area, source), source);
            break;
        }
        case EObjectType::TANK_SHIELD:
        {
            const Gdiplus::Rect source(0, 0, static_cast<int>(tankWeaponImage->GetWidth()), static_cast<int>(tankWeaponImage->GetHeight()));
            const Gdiplus::RectF area(object.x, object.y, object.width, object.height);
            DrawImageWithWhiteKey(graphics, *tankWeaponImage, FitImageAspectRatio(area, source), source);
            break;
        }
        case EObjectType::HIT_EFFECT:
        {
            const int imageIndex = std::clamp(static_cast<int>(object.phase), 0, 2);
            // 몬스터의 공격은 세 갈퀴, 영웅의 공격은 한 줄 베기를 사용합니다.
            const std::array<Gdiplus::Image*, 3>& images = attack.monsterAttacking ? monsterHitEffectImages : heroHitEffectImages;
            if (images[imageIndex] != nullptr)
                DrawEffectImage(graphics, *images[imageIndex], object.x, object.y, object.width, object.height, 0.0f);
            else
                DrawHitEffect(graphics, object.x, object.y, static_cast<float>((attackTime - 0.18) / 0.26));
            break;
        }
        case EObjectType::HEAL_EFFECT:
            DrawEffectImage(graphics, *healEffectImage, object.x, object.y, object.width, object.height, 0.0f);
            break;
        case EObjectType::POWER_BUFF_EFFECT:
            DrawEffectImage(graphics, *powerBuffEffectImage, object.x, object.y, object.width, object.height, 0.0f);
            break;
        }
    }

    if (placement.active && placement.selected != EPlacementTarget::NONE)
    {
        Gdiplus::RectF selectedArea;
        if (placement.selected == EPlacementTarget::HERO)
            selectedArea = {config.heroX, config.heroY, config.heroWidth, config.heroHeight};
        else if (placement.selected == EPlacementTarget::HERO2)
            selectedArea = {config.hero2X, config.hero2Y, config.hero2Width, config.hero2Height};
        else if (placement.selected == EPlacementTarget::TANK)
            selectedArea = {config.tankX, config.tankY, config.tankWidth, config.tankHeight};
        else if (placement.selected == EPlacementTarget::WARRIOR_WEAPON)
            selectedArea = {config.warriorWeaponX - config.warriorWeaponWidth * 0.5f, config.warriorWeaponY - config.warriorWeaponHeight * 0.5f, config.warriorWeaponWidth, config.warriorWeaponHeight};
        else if (placement.selected == EPlacementTarget::TANK_SHIELD)
            selectedArea = {config.tankShieldX, config.tankShieldY, config.tankShieldWidth, config.tankShieldHeight};
        else if (placement.selected == EPlacementTarget::MAGE_WEAPON)
            selectedArea = {config.mageWeaponX - config.mageWeaponWidth * 0.5f, config.mageWeaponY - config.mageWeaponHeight * 0.5f, config.mageWeaponWidth, config.mageWeaponHeight};
        else if (placement.selected == EPlacementTarget::MONSTER)
        {
            const MonsterVisualProfile& profile = gBattleMonsterInstanceProfiles[placement.monsterInstanceId];
            selectedArea = {profile.x, profile.y, profile.width, profile.height};
        }
        else if (placement.selected == EPlacementTarget::MONSTER2)
            selectedArea = {config.monster2X, config.monster2Y, config.monster2Width, config.monster2Height};
        else if (placement.selected == EPlacementTarget::MONSTER3)
            selectedArea = {config.monster3X, config.monster3Y, config.monster3Width, config.monster3Height};
        else if (placement.selected == EPlacementTarget::MONSTER4)
            selectedArea = {config.monster4X, config.monster4Y, config.monster4Width, config.monster4Height};
        else if (placement.selected == EPlacementTarget::HEAL_EFFECT)
            selectedArea = {config.heroX + config.heroWidth * 0.5f + config.healEffectOffsetX - config.healEffectWidth * 0.5f,
                            config.heroY + config.heroHeight * 0.44f + config.healEffectOffsetY - config.healEffectHeight * 0.5f,
                            config.healEffectWidth, config.healEffectHeight};
        else if (placement.selected == EPlacementTarget::POWER_BUFF_EFFECT)
            selectedArea = {config.hero2X + config.hero2Width * 0.5f + config.powerBuffEffectOffsetX - config.powerBuffEffectWidth * 0.5f,
                            config.hero2Y + config.hero2Height * 0.44f + config.powerBuffEffectOffsetY - config.powerBuffEffectHeight * 0.5f,
                            config.powerBuffEffectWidth, config.powerBuffEffectHeight};

        const float pulse = (std::sin(static_cast<float>(timeSeconds * 5.0)) + 1.0f) * 0.5f;
        const BYTE brightness = static_cast<BYTE>(130 + pulse * 80);
        Gdiplus::Pen selectionPen(Gdiplus::Color(255, brightness, brightness, brightness), 4.0f);
        graphics.DrawRectangle(&selectionPen, selectedArea);
    }

}

std::wstring DescribePlacementTarget(const PlacementMode& placement)
{
    switch (placement.selected)
    {
    case EPlacementTarget::HERO: return L"전사";
    case EPlacementTarget::HERO2: return L"마법사";
    case EPlacementTarget::TANK: return L"탱커";
    case EPlacementTarget::WARRIOR_WEAPON: return L"전사 무기";
    case EPlacementTarget::MAGE_WEAPON: return L"마법사 무기";
    case EPlacementTarget::TANK_SHIELD: return L"탱커 방패";
    case EPlacementTarget::HEAL_EFFECT: return L"회복 이펙트";
    case EPlacementTarget::POWER_BUFF_EFFECT: return L"공격력 버프 이펙트";
    case EPlacementTarget::MONSTER: return L"몬스터: " + placement.monsterDisplayName;
    default: return L"없음";
    }
}

void DrawPlacementStatus(HANDLE output, const ControlPanelLayout& layout, const ArtResolution& resolution, const PlacementMode& placement)
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info)) return;
    const short statusY = static_cast<short>(std::clamp(
        static_cast<int>(layout.artStartY + resolution.pixelHeight / 4 + 1), 0, static_cast<int>(info.dwSize.Y) - 1));
    const std::wstring blank(static_cast<size_t>(std::max<short>(0, info.dwSize.X - 1)), L' ');
    WriteAt(output, 0, statusY, blank);
    if (placement.active)
    {
        SetConsoleTextAttribute(output, static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
        WriteAt(output, 0, statusY, L"[ 배치 선택 ] " + DescribePlacementTarget(placement) + L"  |  드래그: 이동  +/-: 크기  Tab/→: 다음  ←: 이전");
        SetConsoleTextAttribute(output, kTextColor);
    }
}

// 처음에는 완전히 검은 화면이고, 가운데 한 줄부터 위·아래로 실제 전투 아트를 펼쳐 보입니다.
// 나중에는 마을 복귀·보스 등장에도 같은 함수를 호출해서 재사용할 수 있습니다.
void PlayVerticalReveal(HANDLE output, const std::vector<std::wstring>& artLines, short top, const std::wstring& title)
{
    if (artLines.empty()) return;
    ClearConsole(output);
    const int height = static_cast<int>(artLines.size());
    const int width = GetVisibleConsoleDisplayWidth(artLines.front());
    const int middle = height / 2;
    constexpr int kSteps = 9;
    // 시작 전 검은 화면을 잠깐 유지해 "전투로 전환된다"는 준비감을 줍니다.
    std::this_thread::sleep_for(std::chrono::milliseconds(650));
    for (int step = 1; step <= kSteps; ++step)
    {
        const int halfHeight = std::max(1, height * step / (kSteps * 2));
        const int firstRow = std::max(0, middle - halfHeight);
        const int lastRow = std::min(static_cast<int>(height) - 1, middle + halfHeight);
        for (int row = firstRow; row <= lastRow; ++row)
        {
            WriteAt(output, 0, static_cast<short>(top + row), artLines[row]);
        }
        // 첫 색상 한 줄은 비교적 길게 보여주고, 화면 끝으로 갈수록 빠르게 펼칩니다.
        const int delayMilliseconds = (step == 1) ? 170 : std::max(8, 55 - step * 6);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMilliseconds));
    }

    const short titleY = static_cast<short>(top + middle);
    const short titleX = static_cast<short>(std::max(0, (width - GetConsoleDisplayWidth(title)) / 2));
    SetConsoleTextAttribute(output, static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
    WriteAt(output, titleX, titleY, title);
    SetConsoleTextAttribute(output, kTextColor);
    std::this_thread::sleep_for(std::chrono::milliseconds(320));
}

ControlPanelLayout DrawFrame(
    HANDLE handle,
    const std::vector<std::wstring>& artLines,
    const RenderSettings& settings,
    const ControlPanelLayout& layout,
    bool attackPlaying,
    bool placementActive,
    bool showDeveloperPanel,
    bool refreshFloatingTextRows)
{
    // 전투 중에는 배경 대부분이 변하지 않습니다. 매 프레임 화면 전체를 지우면
    // 콘솔 출력 자체가 병목이 되므로, 달라진 Braille 행만 다시 씁니다.
    // 창 크기·P 개발 패널 전환처럼 좌표가 달라질 때는 WriteLayeredFrame이 한 번 전체를 정리합니다.
    const short artWidth = artLines.empty()
        ? 0
        : static_cast<short>(GetVisibleConsoleDisplayWidth(artLines.front()));
    // 턴·행동 문구는 그림 위에 직접 겹쳐 그립니다. 해당 다섯 줄은 매 프레임 원래 Braille 배경으로
    // 되돌린 뒤 HUD를 덮어써서, 문구가 짧아지거나 사라져도 잔상이 남지 않게 합니다.
    // 피해/회복 수치가 위로 움직이는 0.72초 동안에는 그 수치가 지나간 줄도 복원해야 하므로
    // 그 짧은 시간에만 그림 전체 행을 다시 출력합니다.
    const short forcedLastLine = refreshFloatingTextRows
        ? static_cast<short>(artLines.empty() ? -1 : artLines.size() - 1)
        : 4;
    WriteLayeredFrame(handle, artLines, 0, layout.artStartY, artWidth, 0, forcedLastLine);

    if (showDeveloperPanel)
    {
        WriteAt(handle, 2, 0, placementActive
            ? L"배치 모드: 이미지 드래그 / Enter: 완료 저장 / X: 취소 원복"
            : L"마우스: 슬라이더 / P: 배치 / A: 공격 / C: 컬러 / S: 저장 / ESC: 종료");
        DrawSlider(handle, layout.sliders[0], L"해상도", settings.outputPixelWidth);
        DrawSlider(handle, layout.sliders[1], L"세로비율", settings.characterHeightScaleValue);
        DrawSlider(handle, layout.sliders[2], L"대비", settings.contrastValue);
        WriteAt(handle, 2, static_cast<short>(layout.artStartY - 1),
                attackPlaying ? L"공격 연출 재생 중" : L"대기: A 키를 눌러 공격 테스트");
    }

    return layout;
}

ESlider FindSliderAt(short x, short y, const ControlPanelLayout& layout)
{
    for (const SliderLayout& slider : layout.sliders)
    {
        const short endX = static_cast<short>(slider.sliderStartX + kSliderWidth + 1);
        if (y == slider.y && x >= slider.sliderStartX && x <= endX)
        {
            return slider.slider;
        }
    }
    return ESlider::NONE;
}

void UpdateSliderValue(ESlider slider, short mouseX, const ControlPanelLayout& layout, RenderSettings& settings)
{
    const SliderLayout* target = nullptr;
    for (const SliderLayout& item : layout.sliders)
    {
        if (item.slider == slider)
        {
            target = &item;
            break;
        }
    }
    if (target == nullptr)
    {
        return;
    }

    const int position = std::clamp(static_cast<int>(mouseX - target->sliderStartX - 1), 0, kSliderWidth);
    const int value = position * 1000 / kSliderWidth;
    if (slider == ESlider::OUTPUT_WIDTH)
    {
        settings.outputPixelWidth = value;
    }
    else if (slider == ESlider::HEIGHT_SCALE)
    {
        settings.characterHeightScaleValue = value;
    }
    else if (slider == ESlider::CONTRAST)
    {
        settings.contrastValue = value;
    }
}

void CopyRenderSettingsToConfig(const RenderSettings& settings, SceneConfig& config)
{
    config.outputPixelWidth = settings.outputPixelWidth;
    config.characterHeightScaleValue = settings.characterHeightScaleValue;
    config.contrastValue = settings.contrastValue;
    config.useOrderedDithering = settings.useOrderedDithering;
    config.useAnsiColor = settings.useAnsiColor;
    config.colorMode = settings.colorMode;
}

EPlacementTarget FindPlacementTarget(float sceneX, float sceneY, const SceneConfig& config)
{
    if (sceneX >= config.tankShieldX && sceneX <= config.tankShieldX + config.tankShieldWidth &&
        sceneY >= config.tankShieldY && sceneY <= config.tankShieldY + config.tankShieldHeight)
        return EPlacementTarget::TANK_SHIELD;
    if (std::abs(sceneX - config.warriorWeaponX) <= config.warriorWeaponWidth * 0.5f &&
        std::abs(sceneY - config.warriorWeaponY) <= config.warriorWeaponHeight * 0.5f)
        return EPlacementTarget::WARRIOR_WEAPON;
    if (std::abs(sceneX - config.mageWeaponX) <= config.mageWeaponWidth * 0.5f &&
        std::abs(sceneY - config.mageWeaponY) <= config.mageWeaponHeight * 0.5f)
        return EPlacementTarget::MAGE_WEAPON;
    if (sceneX >= config.heroX && sceneX <= config.heroX + config.heroWidth &&
        sceneY >= config.heroY && sceneY <= config.heroY + config.heroHeight)
        return EPlacementTarget::HERO;
    if (sceneX >= config.hero2X && sceneX <= config.hero2X + config.hero2Width &&
        sceneY >= config.hero2Y && sceneY <= config.hero2Y + config.hero2Height)
        return EPlacementTarget::HERO2;
    if (sceneX >= config.tankX && sceneX <= config.tankX + config.tankWidth &&
        sceneY >= config.tankY && sceneY <= config.tankY + config.tankHeight)
        return EPlacementTarget::TANK;
    if (sceneX >= config.monsterX && sceneX <= config.monsterX + config.monsterWidth &&
        sceneY >= config.monsterY && sceneY <= config.monsterY + config.monsterHeight)
        return EPlacementTarget::MONSTER;
    return EPlacementTarget::NONE;
}

void MovePlacementTarget(PlacementMode& placement, SceneConfig& config, float sceneX, float sceneY)
{
    if (placement.selected == EPlacementTarget::HERO)
    {
        config.heroX = sceneX - placement.grabOffsetX;
        config.heroY = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::MONSTER)
    {
        MonsterVisualProfile& profile = gBattleMonsterInstanceProfiles[placement.monsterInstanceId];
        profile.x = sceneX - placement.grabOffsetX;
        profile.y = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::MONSTER2)
    {
        config.monster2X = sceneX - placement.grabOffsetX;
        config.monster2Y = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::MONSTER3)
    {
        config.monster3X = sceneX - placement.grabOffsetX;
        config.monster3Y = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::MONSTER4)
    {
        config.monster4X = sceneX - placement.grabOffsetX;
        config.monster4Y = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::HERO2)
    {
        config.hero2X = sceneX - placement.grabOffsetX;
        config.hero2Y = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::TANK)
    {
        const float newX = sceneX - placement.grabOffsetX;
        const float newY = sceneY - placement.grabOffsetY;
        // 탱커를 움직일 때 방패도 같은 거리만 움직여 대형을 유지합니다.
        config.tankShieldX += newX - config.tankX;
        config.tankShieldY += newY - config.tankY;
        config.tankX = newX;
        config.tankY = newY;
    }
    else if (placement.selected == EPlacementTarget::WARRIOR_WEAPON)
    {
        config.warriorWeaponX = sceneX - placement.grabOffsetX;
        config.warriorWeaponY = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::MAGE_WEAPON)
    {
        config.mageWeaponX = sceneX - placement.grabOffsetX;
        config.mageWeaponY = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::TANK_SHIELD)
    {
        config.tankShieldX = sceneX - placement.grabOffsetX;
        config.tankShieldY = sceneY - placement.grabOffsetY;
    }
    else if (placement.selected == EPlacementTarget::HEAL_EFFECT)
    {
        config.healEffectOffsetX = sceneX - (config.heroX + config.heroWidth * 0.5f);
        config.healEffectOffsetY = sceneY - (config.heroY + config.heroHeight * 0.44f);
    }
    else if (placement.selected == EPlacementTarget::POWER_BUFF_EFFECT)
    {
        config.powerBuffEffectOffsetX = sceneX - (config.hero2X + config.hero2Width * 0.5f);
        config.powerBuffEffectOffsetY = sceneY - (config.hero2Y + config.hero2Height * 0.44f);
    }
}

void ScalePlacementTarget(PlacementMode& placement, SceneConfig& config, float scale)
{
    if (placement.selected == EPlacementTarget::HERO)
    {
        config.heroWidth = std::clamp(config.heroWidth * scale, 20.0f, 2000.0f);
        config.heroHeight = std::clamp(config.heroHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::MONSTER)
    {
        const auto typeFound = gBattleMonsterInstanceTypes.find(placement.monsterInstanceId);
        if (typeFound == gBattleMonsterInstanceTypes.end()) return;

        // 위치는 슬롯별로 유지하고, 크기는 몬스터 종류별 설정에 저장합니다.
        MonsterVisualProfile& typeProfile = config.monsterProfiles[static_cast<size_t>(typeFound->second)];
        typeProfile.width = std::clamp(typeProfile.width * scale, 20.0f, 2000.0f);
        typeProfile.height = std::clamp(typeProfile.height * scale, 20.0f, 2000.0f);
        for (const auto& [instanceId, typeIndex] : gBattleMonsterInstanceTypes)
        {
            if (typeIndex != typeFound->second) continue;
            MonsterVisualProfile& instance = gBattleMonsterInstanceProfiles[instanceId];
            instance.width = typeProfile.width;
            instance.height = typeProfile.height;
        }
    }
    else if (placement.selected == EPlacementTarget::MONSTER2)
    {
        config.monster2Width = std::clamp(config.monster2Width * scale, 20.0f, 2000.0f);
        config.monster2Height = std::clamp(config.monster2Height * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::MONSTER3)
    {
        config.monster3Width = std::clamp(config.monster3Width * scale, 20.0f, 2000.0f);
        config.monster3Height = std::clamp(config.monster3Height * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::MONSTER4)
    {
        config.monster4Width = std::clamp(config.monster4Width * scale, 20.0f, 2000.0f);
        config.monster4Height = std::clamp(config.monster4Height * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::HERO2)
    {
        config.hero2Width = std::clamp(config.hero2Width * scale, 20.0f, 2000.0f);
        config.hero2Height = std::clamp(config.hero2Height * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::TANK)
    {
        config.tankWidth = std::clamp(config.tankWidth * scale, 20.0f, 2000.0f);
        config.tankHeight = std::clamp(config.tankHeight * scale, 20.0f, 2000.0f);
        config.tankShieldWidth = std::clamp(config.tankShieldWidth * scale, 20.0f, 2000.0f);
        config.tankShieldHeight = std::clamp(config.tankShieldHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::WARRIOR_WEAPON)
    {
        config.warriorWeaponWidth = std::clamp(config.warriorWeaponWidth * scale, 20.0f, 2000.0f);
        config.warriorWeaponHeight = std::clamp(config.warriorWeaponHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::MAGE_WEAPON)
    {
        config.mageWeaponWidth = std::clamp(config.mageWeaponWidth * scale, 20.0f, 2000.0f);
        config.mageWeaponHeight = std::clamp(config.mageWeaponHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::TANK_SHIELD)
    {
        config.tankShieldWidth = std::clamp(config.tankShieldWidth * scale, 20.0f, 2000.0f);
        config.tankShieldHeight = std::clamp(config.tankShieldHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::HEAL_EFFECT)
    {
        config.healEffectWidth = std::clamp(config.healEffectWidth * scale, 20.0f, 2000.0f);
        config.healEffectHeight = std::clamp(config.healEffectHeight * scale, 20.0f, 2000.0f);
    }
    else if (placement.selected == EPlacementTarget::POWER_BUFF_EFFECT)
    {
        config.powerBuffEffectWidth = std::clamp(config.powerBuffEffectWidth * scale, 20.0f, 2000.0f);
        config.powerBuffEffectHeight = std::clamp(config.powerBuffEffectHeight * scale, 20.0f, 2000.0f);
    }
}

void SelectNextPlacementTarget(
    PlacementMode& placement,
    const AsciiArt::BattleSceneState& battleState,
    bool showHealEffect,
    bool showPowerBuffEffect)
{
    // Tab은 설정 파일의 종류 목록이 아니라 이번 전투 화면에 실제로 있는 각 개체를 순회합니다.
    std::vector<const AsciiArt::ActorBattleStatus*> activeMonsters;
    for (const AsciiArt::ActorBattleStatus& status : battleState.monsterStatuses)
    {
        if (!status.isDead) activeMonsters.push_back(&status);
    }
    const auto selectAfterMonsters = [&]
    {
        placement.selected = showHealEffect ? EPlacementTarget::HEAL_EFFECT :
            (showPowerBuffEffect ? EPlacementTarget::POWER_BUFF_EFFECT : EPlacementTarget::HERO);
    };

    switch (placement.selected)
    {
    case EPlacementTarget::NONE: placement.selected = EPlacementTarget::HERO; break;
    case EPlacementTarget::HERO: placement.selected = EPlacementTarget::HERO2; break;
    case EPlacementTarget::HERO2: placement.selected = EPlacementTarget::TANK; break;
    case EPlacementTarget::TANK: placement.selected = EPlacementTarget::WARRIOR_WEAPON; break;
    case EPlacementTarget::WARRIOR_WEAPON: placement.selected = EPlacementTarget::TANK_SHIELD; break;
    case EPlacementTarget::TANK_SHIELD: placement.selected = EPlacementTarget::MAGE_WEAPON; break;
    case EPlacementTarget::MAGE_WEAPON:
        if (activeMonsters.empty()) selectAfterMonsters();
        else
        {
            placement.selected = EPlacementTarget::MONSTER;
            placement.monsterInstanceId = activeMonsters.front()->id;
            placement.monsterDisplayName = Utf8ToWide(activeMonsters.front()->displayName);
        }
        break;
    case EPlacementTarget::MONSTER:
    {
        const auto current = std::find_if(activeMonsters.begin(), activeMonsters.end(), [&](const AsciiArt::ActorBattleStatus* status)
        {
            return status->id == placement.monsterInstanceId;
        });
        if (current != activeMonsters.end() && std::next(current) != activeMonsters.end())
        {
            const AsciiArt::ActorBattleStatus* next = *std::next(current);
            placement.monsterInstanceId = next->id;
            placement.monsterDisplayName = Utf8ToWide(next->displayName);
        }
        else selectAfterMonsters();
        break;
    }
    case EPlacementTarget::MONSTER2:
    case EPlacementTarget::MONSTER3:
    case EPlacementTarget::MONSTER4:
        selectAfterMonsters();
        break;
    case EPlacementTarget::HEAL_EFFECT: placement.selected = showPowerBuffEffect ? EPlacementTarget::POWER_BUFF_EFFECT : EPlacementTarget::HERO; break;
    case EPlacementTarget::POWER_BUFF_EFFECT: placement.selected = EPlacementTarget::HERO; break;
    }
}

bool GetSceneMousePosition(
    short mouseX, short mouseY, const ControlPanelLayout& layout, const ArtResolution& resolution,
    const SceneConfig& config, float& sceneX, float& sceneY)
{
    if (mouseX < 0 || mouseY < layout.artStartY || mouseX >= resolution.pixelWidth / 2 ||
        mouseY >= layout.artStartY + resolution.pixelHeight / 4)
        return false;
    sceneX = mouseX * 2.0f * config.sceneWidth / resolution.pixelWidth;
    sceneY = (mouseY - layout.artStartY) * 4.0f * config.sceneHeight / resolution.pixelHeight;
    return true;
}

// ProcessInput에서 왼쪽 화살표를 처리할 때 사용합니다.
void SelectPreviousPlacementTarget(
    PlacementMode& placement,
    const AsciiArt::BattleSceneState& battleState,
    bool showHealEffect,
    bool showPowerBuffEffect);

void ProcessInput(
    HANDLE inputHandle,
    const ControlPanelLayout& layout,
    RenderSettings& settings,
    SceneConfig& config,
    int& currentTurn,
    PlacementMode& placement,
    const ArtResolution& resolution,
    ESlider& activeSlider,
    AttackAnimation& attack,
    bool& isRunning,
    bool& showDeveloperPanel,
    bool& manualAttackMode,
    bool testInputMode,
    bool sameMonsterPlacementTestMode,
    bool showHealEffect,
    bool showPowerBuffEffect,
    const AsciiArt::BattleSceneState& battleState,
    int& requestedMonsterIndex,
    int& requestedTestCommand)
{
    const int monsterCount = static_cast<int>(battleState.monsterStatuses.size());
    DWORD count = 0;
    GetNumberOfConsoleInputEvents(inputHandle, &count);
    while (count-- > 0)
    {
        INPUT_RECORD record;
        DWORD read = 0;
        ReadConsoleInputW(inputHandle, &record, 1, &read);

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
        {
            const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
            if (key == VK_ESCAPE)
            {
                isRunning = false;
            }
            else if (testInputMode && !attack.playing && (key == '8' || key == VK_NUMPAD8))
            {
                // 8번은 배치모드 중에도 동일 몬스터 4마리 테스트를 바로 종료할 수 있습니다.
                requestedTestCommand = 8;
            }
            else if (testInputMode && sameMonsterPlacementTestMode && !placement.active &&
                     (key == VK_TAB || key == VK_RIGHT || key == VK_LEFT))
            {
                // P를 누르기 전에는 같은 몬스터 4마리의 "종류"를 빠르게 바꿔 볼 수 있습니다.
                requestedTestCommand = key == VK_LEFT ? 10 : 9;
            }
            else if (testInputMode && !placement.active && !attack.playing &&
                     (key == '1' || key == VK_NUMPAD1 || key == '2' || key == VK_NUMPAD2 ||
                      key == '3' || key == VK_NUMPAD3 || key == '4' || key == VK_NUMPAD4 ||
                      key == '5' || key == VK_NUMPAD5 || key == '6' || key == VK_NUMPAD6 ||
                      key == '7' || key == VK_NUMPAD7))
            {
                if (key == '1' || key == VK_NUMPAD1) requestedTestCommand = 1;
                else if (key == '2' || key == VK_NUMPAD2) requestedTestCommand = 2;
                else if (key == '3' || key == VK_NUMPAD3) requestedTestCommand = 3;
                else if (key == '4' || key == VK_NUMPAD4) requestedTestCommand = 4;
                else if (key == '5' || key == VK_NUMPAD5) requestedTestCommand = 5;
                else if (key == '6' || key == VK_NUMPAD6) requestedTestCommand = 6;
                else requestedTestCommand = 7;
            }
            else if (key == 'P' && !placement.active)
            {
                // 평소에는 편집 UI를 숨겨 두고 P를 눌렀을 때만 표시합니다.
                showDeveloperPanel = true;
                placement.active = true;
                placement.beforeEditing = config;
                placement.selected = EPlacementTarget::HERO;
            }
            else if ((key == VK_TAB || key == VK_RIGHT) && placement.active)
            {
                SelectNextPlacementTarget(placement, battleState, showHealEffect, showPowerBuffEffect);
            }
            else if (key == VK_LEFT && placement.active)
            {
                SelectPreviousPlacementTarget(placement, battleState, showHealEffect, showPowerBuffEffect);
            }
            else if (key == 'X' && placement.active)
            {
                config = placement.beforeEditing;
                // 저장하지 않고 취소한 경우, 다음 프레임에 취소 전 슬롯값으로 다시 생성합니다.
                gBattleMonsterInstanceProfiles.clear();
                gBattleMonsterInstanceTypes.clear();
                placement = PlacementMode{};
                showDeveloperPanel = false;
            }
            else if (key == VK_RETURN && placement.active)
            {
                // 현재 전투에서 실제로 등장한 몬스터 순서대로 슬롯 1~4에 저장합니다.
                CopyBattleMonsterInstancesToSlots(config, battleState);
                CopyRenderSettingsToConfig(settings, config);
                SaveSceneConfig(config);
                placement = PlacementMode{};
            }
            else if (placement.active && placement.selected != EPlacementTarget::NONE &&
                     (key == VK_OEM_PLUS || key == VK_ADD))
            {
                ScalePlacementTarget(placement, config, 1.10f);
            }
            else if (placement.active && placement.selected != EPlacementTarget::NONE &&
                     (key == VK_OEM_MINUS || key == VK_SUBTRACT))
            {
                ScalePlacementTarget(placement, config, 0.90f);
            }
            else if (key == 'D')
            {
                settings.useOrderedDithering = !settings.useOrderedDithering;
            }
            else if (key == 'C')
            {
                settings.useAnsiColor = !settings.useAnsiColor;
            }
            else if (key == 'S')
            {
                CopyBattleMonsterInstancesToSlots(config, battleState);
                CopyRenderSettingsToConfig(settings, config);
                SaveSceneConfig(config);
            }
            else if (key == 'A' && !attack.playing)
            {
                manualAttackMode = !manualAttackMode;
            }
            continue;
        }

        if (record.EventType != MOUSE_EVENT)
        {
            continue;
        }

        const MOUSE_EVENT_RECORD& mouse = record.Event.MouseEvent;
        const bool pressed = (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;

        // P로 배치 모드에 들어가도 위쪽 슬라이더는 그대로 조절할 수 있어야 합니다.
        // 이전에는 placement.active가 먼저 처리되어 슬라이더 드래그가 막혔습니다.
        const ESlider sliderUnderMouse = showDeveloperPanel
            ? FindSliderAt(mouse.dwMousePosition.X, mouse.dwMousePosition.Y, layout)
            : ESlider::NONE;
        if (sliderUnderMouse != ESlider::NONE || activeSlider != ESlider::NONE)
        {
            if (pressed && mouse.dwEventFlags == 0)
            {
                activeSlider = sliderUnderMouse;
                UpdateSliderValue(activeSlider, mouse.dwMousePosition.X, layout, settings);
            }
            else if (pressed && mouse.dwEventFlags == MOUSE_MOVED && activeSlider != ESlider::NONE)
            {
                UpdateSliderValue(activeSlider, mouse.dwMousePosition.X, layout, settings);
            }
            else if (!pressed)
            {
                activeSlider = ESlider::NONE;
            }
            continue;
        }

        if (placement.active)
        {
            float sceneX = 0.0f;
            float sceneY = 0.0f;
            const bool isOnArt = GetSceneMousePosition(
                mouse.dwMousePosition.X, mouse.dwMousePosition.Y, layout, resolution, config, sceneX, sceneY);

            if (pressed && mouse.dwEventFlags == 0 && isOnArt &&
                placement.selected != EPlacementTarget::NONE)
            {
                // 배치 모드에서는 Tab으로 고른 대상을 유지합니다. 큰 이미지가
                // 겹쳐 있어도 빈 곳에서 드래그해 선택 대상을 쉽게 옮길 수 있습니다.
                placement.dragging = true;
                if (placement.selected == EPlacementTarget::HERO)
                {
                    placement.grabOffsetX = config.heroWidth * 0.5f;
                    placement.grabOffsetY = config.heroHeight * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::MONSTER)
                {
                    const MonsterVisualProfile& profile = gBattleMonsterInstanceProfiles[placement.monsterInstanceId];
                    placement.grabOffsetX = profile.width * 0.5f;
                    placement.grabOffsetY = profile.height * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::MONSTER2)
                {
                    placement.grabOffsetX = config.monster2Width * 0.5f;
                    placement.grabOffsetY = config.monster2Height * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::MONSTER3)
                {
                    placement.grabOffsetX = config.monster3Width * 0.5f;
                    placement.grabOffsetY = config.monster3Height * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::MONSTER4)
                {
                    placement.grabOffsetX = config.monster4Width * 0.5f;
                    placement.grabOffsetY = config.monster4Height * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::HERO2)
                {
                    placement.grabOffsetX = config.hero2Width * 0.5f;
                    placement.grabOffsetY = config.hero2Height * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::TANK)
                {
                    placement.grabOffsetX = config.tankWidth * 0.5f;
                    placement.grabOffsetY = config.tankHeight * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::WARRIOR_WEAPON)
                {
                    placement.grabOffsetX = 0.0f;
                    placement.grabOffsetY = 0.0f;
                }
                else if (placement.selected == EPlacementTarget::MAGE_WEAPON)
                {
                    placement.grabOffsetX = 0.0f;
                    placement.grabOffsetY = 0.0f;
                }
                else if (placement.selected == EPlacementTarget::TANK_SHIELD)
                {
                    placement.grabOffsetX = config.tankShieldWidth * 0.5f;
                    placement.grabOffsetY = config.tankShieldHeight * 0.5f;
                }
                else if (placement.selected == EPlacementTarget::HEAL_EFFECT ||
                         placement.selected == EPlacementTarget::POWER_BUFF_EFFECT)
                {
                    placement.grabOffsetX = 0.0f;
                    placement.grabOffsetY = 0.0f;
                }
            }
            else if (pressed && mouse.dwEventFlags == MOUSE_MOVED && placement.dragging && isOnArt)
            {
                MovePlacementTarget(placement, config, sceneX, sceneY);
            }
            else if (!pressed)
            {
                placement.dragging = false;
            }
            continue;
        }

        if (manualAttackMode && pressed && mouse.dwEventFlags == 0 && !attack.playing)
        {
            float sceneX = 0.0f;
            float sceneY = 0.0f;
            if (GetSceneMousePosition(mouse.dwMousePosition.X, mouse.dwMousePosition.Y, layout, resolution, config, sceneX, sceneY))
            {
                for (int index = 0; index < monsterCount; ++index)
                {
                    if (battleState.monsterStatuses[index].isDead) continue;
                    const MonsterVisualArea target = GetMonsterVisualArea(
                        config, battleState.monsterStatuses[index], GetSameTypeMonsterOrder(battleState, index));
                    if (sceneX >= target.x && sceneX <= target.x + target.width &&
                        sceneY >= target.y && sceneY <= target.y + target.height)
                    {
                        requestedMonsterIndex = index;
                        attack.playing = true;
                        attack.hitEffectVariant = PickRandomHitEffectVariant();
                        attack.monsterAttacking = false;
                        attack.attackerIndex = currentTurn;
                        attack.targetIndex = index;
                        attack.startedAt = std::chrono::steady_clock::now();
                        break;
                    }
                }
                continue;
            }
        }

        if (pressed && mouse.dwEventFlags == 0)
        {
            activeSlider = FindSliderAt(mouse.dwMousePosition.X, mouse.dwMousePosition.Y, layout);
            UpdateSliderValue(activeSlider, mouse.dwMousePosition.X, layout, settings);
        }
        else if (pressed && mouse.dwEventFlags == MOUSE_MOVED)
        {
            UpdateSliderValue(activeSlider, mouse.dwMousePosition.X, layout, settings);
        }
        else if (!pressed)
        {
            activeSlider = ESlider::NONE;
        }
    }
}
} // namespace

bool AsciiArt::RenderStaticImage(
    const std::wstring& imagePath,
    bool useColor,
    int monochromeInkDensity,
    EStaticArtStyle style,
    int monochromeContrast,
    int outputPixelWidth,
    int characterHeightScale)
{
    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;

    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok)
    {
        return false;
    }

    auto image = std::make_unique<Gdiplus::Bitmap>(imagePath.c_str());

    if (image->GetLastStatus() != Gdiplus::Ok)
    {
        std::wcerr << L"시작 화면 이미지를 열지 못했습니다: "
            << imagePath << L'\n';

        image.reset();
        Gdiplus::GdiplusShutdown(token);
        return false;
    }

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD originalOutputMode = 0;
    const bool canReadOutputMode =
        GetConsoleMode(output, &originalOutputMode) != FALSE;

    const bool previousAnsiColorSupported = gAnsiColorSupported;
    gAnsiColorSupported =
        canReadOutputMode &&
        SetConsoleMode(
            output,
            originalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
        ) != FALSE;

    SceneConfig config = LoadSceneConfig();

    RenderSettings settings;
    // 정지 배경은 슬라이더의 현재 해상도보다 콘솔 전체 폭 활용을 우선합니다.
    settings.outputPixelWidth = ClampSetting(outputPixelWidth);
    // 흑백과 컬러 전환에서 점의 배치와 출력 폭은 그대로 유지하고 색만 바꿉니다.
    settings.characterHeightScaleValue = ClampSetting(characterHeightScale);
    settings.contrastValue = ClampSetting(monochromeContrast);
    settings.darknessBoostValue = ClampSetting(monochromeInkDensity) * 128 / 1000;
    settings.useOrderedDithering = config.useOrderedDithering;
    settings.useAnsiColor = useColor && config.useAnsiColor && gAnsiColorSupported;
    settings.colorMode = config.colorMode;

    const COORD consoleSize = GetVisibleConsoleSize(output);

    // 안내 문구가 표시될 아래쪽 공간 4줄을 남깁니다.
    const int availableRows = std::max(4, static_cast<int>(consoleSize.Y) - 4);

    const double heightScale = GetCharacterHeightScale(settings);

    const int maximumWidthByHeight = static_cast<int>(
        availableRows * 4.0 *
        (static_cast<double>(image->GetWidth()) / image->GetHeight()) /
        heightScale
        );

    const int maximumOutputPixelWidth = std::max(
        16,
        std::min(
            static_cast<int>(consoleSize.X) * 2,
            maximumWidthByHeight
        )
    );

    const std::vector<std::wstring> artLines =
        style == EStaticArtStyle::LandscapeAscii
        ? CreateLandscapeAsciiLines(
            *image,
            settings,
            static_cast<int>(consoleSize.X),
            availableRows)
        : CreateBrailleLines(*image, settings, maximumOutputPixelWidth);

    ClearConsole(output);

    CONSOLE_SCREEN_BUFFER_INFO outputInfo;
    GetConsoleScreenBufferInfo(output, &outputInfo);
    const int artWidth = artLines.empty() ? 0 : GetVisibleConsoleDisplayWidth(artLines.front());
    // 시작 배경은 콘솔의 왼쪽부터 출력합니다.
    // 메뉴 문구는 아래에 저장하는 이미지 범위를 기준으로 중앙에 맞춥니다.
    const short artX = outputInfo.srWindow.Left;
    const short artY = outputInfo.srWindow.Top;
    gStaticImageLeft = artX;
    gStaticImageWidth = static_cast<short>(artWidth);
    gStaticImageTop = artY;
    gStaticImageHeight = static_cast<short>(artLines.size());
    gHasStaticImageBounds = artWidth > 0;

    for (short lineIndex = 0;
        lineIndex < static_cast<short>(artLines.size());
        ++lineIndex)
    {
        WriteAt(output, artX, static_cast<short>(artY + lineIndex), artLines[lineIndex]);
    }

    SetConsoleCursorPosition(
        output,
        { artX, static_cast<short>(artY + artLines.size()) }
    );

    if (canReadOutputMode)
    {
        SetConsoleMode(output, originalOutputMode);
    }

    gAnsiColorSupported = previousAnsiColorSupported;
    image.reset();
    Gdiplus::GdiplusShutdown(token);

    return true;
}

bool AsciiArt::RenderLayeredStaticImage(
    const std::wstring& backgroundImagePath,
    const std::wstring& characterImagePath,
    const std::wstring& foregroundImagePath,
    float characterX,
    float characterY,
    float characterWidth,
    float characterHeight,
    int outputPixelWidth,
    int characterHeightScale,
    int contrast)
{
    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;

    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok)
    {
        return false;
    }

    bool didRenderScene = false;

    // GDI+ 객체는 모두 이 중괄호 안에서 먼저 파괴됩니다.
    // 그 다음에만 GdiplusShutdown을 호출해야 접근 위반이 나지 않습니다.
    {
        Gdiplus::Bitmap background(backgroundImagePath.c_str());
        Gdiplus::Bitmap character(characterImagePath.c_str());
        const bool hasForegroundImage = !foregroundImagePath.empty();
        std::unique_ptr<Gdiplus::Bitmap> foreground;
        if (hasForegroundImage)
        {
            foreground = std::make_unique<Gdiplus::Bitmap>(foregroundImagePath.c_str());
        }

        if (background.GetLastStatus() == Gdiplus::Ok &&
            character.GetLastStatus() == Gdiplus::Ok &&
            (!hasForegroundImage || foreground->GetLastStatus() == Gdiplus::Ok))
        {
            const UINT sceneWidth = background.GetWidth();
            const UINT sceneHeight = background.GetHeight();
            Gdiplus::Bitmap scene(sceneWidth, sceneHeight, PixelFormat32bppARGB);
            Gdiplus::Graphics graphics(&scene);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(&background, 0, 0, static_cast<INT>(sceneWidth), static_cast<INT>(sceneHeight));

            // 흰 배경은 투명하게 처리해 상점 주인을 배경 위에 배치합니다.
            Gdiplus::ImageAttributes transparentWhite;
            transparentWhite.SetColorKey(
                Gdiplus::Color(245, 245, 245),
                Gdiplus::Color(255, 255, 255),
                Gdiplus::ColorAdjustTypeBitmap
            );

            const Gdiplus::Rect characterRect(
                static_cast<INT>(characterX),
                static_cast<INT>(characterY),
                static_cast<INT>(characterWidth),
                static_cast<INT>(characterHeight)
            );
            graphics.DrawImage(&character, characterRect, 0, 0,
                static_cast<INT>(character.GetWidth()), static_cast<INT>(character.GetHeight()),
                Gdiplus::UnitPixel, &transparentWhite);

            // 별도 전경이 없으면 배경의 원래 테이블 부분을 다시 덮습니다.
            // 따라서 상점 주인은 배경 속 테이블 뒤에 서 있는 것처럼 보입니다.
            const Gdiplus::Rect foregroundRect(
                0,
                static_cast<INT>(sceneHeight * 0.70),
                static_cast<INT>(sceneWidth),
                static_cast<INT>(sceneHeight * 0.38)
            );
            if (hasForegroundImage)
            {
                graphics.DrawImage(foreground.get(), foregroundRect, 0, 0,
                    static_cast<INT>(foreground->GetWidth()), static_cast<INT>(foreground->GetHeight()),
                    Gdiplus::UnitPixel, &transparentWhite);
            }
            else
            {
                graphics.DrawImage(&background, foregroundRect,
                    0, static_cast<INT>(sceneHeight * 0.70),
                    static_cast<INT>(sceneWidth), static_cast<INT>(sceneHeight * 0.30),
                    Gdiplus::UnitPixel);
            }

            // 임시 PNG 저장/재로드 없이 합성된 메모리 장면을 바로 Braille로 변환합니다.
            const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD originalOutputMode = 0;
            const bool canReadOutputMode = GetConsoleMode(output, &originalOutputMode) != FALSE;
            const bool previousAnsiColorSupported = gAnsiColorSupported;
            gAnsiColorSupported = canReadOutputMode &&
                SetConsoleMode(output, originalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != FALSE;

            const SceneConfig config = LoadSceneConfig();
            RenderSettings settings;
            settings.outputPixelWidth = ClampSetting(outputPixelWidth);
            settings.characterHeightScaleValue = ClampSetting(characterHeightScale);
            settings.contrastValue = ClampSetting(contrast);
            settings.useOrderedDithering = config.useOrderedDithering;
            settings.useAnsiColor = config.useAnsiColor && gAnsiColorSupported;
            settings.colorMode = config.colorMode;

            const COORD consoleSize = GetVisibleConsoleSize(output);
            const int availableRows = std::max(4, static_cast<int>(consoleSize.Y) - 4);
            const int maximumWidthByHeight = static_cast<int>(availableRows * 4.0 *
                (static_cast<double>(scene.GetWidth()) / scene.GetHeight()) / GetCharacterHeightScale(settings));
            const int maximumOutputPixelWidth = std::max(16, std::min(static_cast<int>(consoleSize.X) * 2, maximumWidthByHeight));
            const std::vector<std::wstring> artLines = CreateBrailleLines(scene, settings, maximumOutputPixelWidth);

            CONSOLE_SCREEN_BUFFER_INFO outputInfo;
            GetConsoleScreenBufferInfo(output, &outputInfo);
            const int artWidth = artLines.empty() ? 0 : GetVisibleConsoleDisplayWidth(artLines.front());
            gStaticImageLeft = outputInfo.srWindow.Left;
            gStaticImageWidth = static_cast<short>(artWidth);
            gStaticImageTop = outputInfo.srWindow.Top;
            gStaticImageHeight = static_cast<short>(artLines.size());
            gHasStaticImageBounds = artWidth > 0;

            // 매 프레임 화면 전체를 지우지 않습니다. 바뀐 Braille 행만 다시 출력합니다.
            WriteLayeredFrame(output, artLines, gStaticImageLeft, gStaticImageTop, gStaticImageWidth);
            SetConsoleCursorPosition(output, {gStaticImageLeft, static_cast<short>(gStaticImageTop + artLines.size())});
            if (canReadOutputMode) SetConsoleMode(output, originalOutputMode);
            gAnsiColorSupported = previousAnsiColorSupported;
            didRenderScene = true;
        }
    }

    Gdiplus::GdiplusShutdown(token);

    return didRenderScene;
}

bool AsciiArt::RenderSavedStartScreenImage(const std::wstring& imagePath, bool useColor)
{
    const SceneConfig config = LoadSceneConfig();
    return RenderStaticImage(
        imagePath,
        useColor,
        config.startScreenInkDensity,
        EStaticArtStyle::Braille,
        config.startScreenContrast);
}

bool AsciiArt::RenderSavedMainMenuImage(const std::wstring& imagePath, bool useColor)
{
    const SceneConfig config = LoadSceneConfig();
    return RenderStaticImage(
        imagePath,
        useColor,
        0,
        EStaticArtStyle::Braille,
        config.mainMenuContrast,
        config.mainMenuOutputPixelWidth,
        config.mainMenuCharacterHeightScale);
}

void AsciiArt::RenderBattleEntryTransition()
{
    const SceneConfig config = LoadSceneConfig();
    if (RenderStaticImage(config.battleTransitionImagePath, true, 0, EStaticArtStyle::Braille,
                          config.contrastValue, config.outputPixelWidth, config.characterHeightScaleValue))
    {
        DrawCenteredTextOnClearPanel(L"[ 전투를 향해 ]", 0.50f);
        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, config.battleTransitionMilliseconds)));
    }
}

namespace
{
void SelectPreviousPlacementTarget(
    PlacementMode& placement,
    const AsciiArt::BattleSceneState& battleState,
    bool showHealEffect,
    bool showPowerBuffEffect)
{
    // Tab/오른쪽 화살표의 정확한 역순으로 실제 전투에 등장한 몬스터만 순회합니다.
    std::vector<const AsciiArt::ActorBattleStatus*> activeMonsters;
    for (const AsciiArt::ActorBattleStatus& status : battleState.monsterStatuses)
    {
        if (!status.isDead) activeMonsters.push_back(&status);
    }
    const auto selectLastMonsterOrWeapon = [&]
    {
        if (activeMonsters.empty())
        {
            placement.selected = EPlacementTarget::MAGE_WEAPON;
            return;
        }
        const AsciiArt::ActorBattleStatus* last = activeMonsters.back();
        placement.selected = EPlacementTarget::MONSTER;
        placement.monsterInstanceId = last->id;
        placement.monsterDisplayName = Utf8ToWide(last->displayName);
    };

    switch (placement.selected)
    {
    case EPlacementTarget::NONE: placement.selected = EPlacementTarget::HERO; break;
    case EPlacementTarget::HERO:
        if (showPowerBuffEffect) placement.selected = EPlacementTarget::POWER_BUFF_EFFECT;
        else if (showHealEffect) placement.selected = EPlacementTarget::HEAL_EFFECT;
        else selectLastMonsterOrWeapon();
        break;
    case EPlacementTarget::HERO2: placement.selected = EPlacementTarget::HERO; break;
    case EPlacementTarget::TANK: placement.selected = EPlacementTarget::HERO2; break;
    case EPlacementTarget::WARRIOR_WEAPON: placement.selected = EPlacementTarget::TANK; break;
    case EPlacementTarget::TANK_SHIELD: placement.selected = EPlacementTarget::WARRIOR_WEAPON; break;
    case EPlacementTarget::MAGE_WEAPON: placement.selected = EPlacementTarget::TANK_SHIELD; break;
    case EPlacementTarget::MONSTER:
    {
        const auto current = std::find_if(activeMonsters.begin(), activeMonsters.end(), [&](const AsciiArt::ActorBattleStatus* status)
        {
            return status->id == placement.monsterInstanceId;
        });
        if (current != activeMonsters.end() && current != activeMonsters.begin())
        {
            const AsciiArt::ActorBattleStatus* previous = *std::prev(current);
            placement.monsterInstanceId = previous->id;
            placement.monsterDisplayName = Utf8ToWide(previous->displayName);
        }
        else placement.selected = EPlacementTarget::MAGE_WEAPON;
        break;
    }
    case EPlacementTarget::HEAL_EFFECT: selectLastMonsterOrWeapon(); break;
    case EPlacementTarget::POWER_BUFF_EFFECT: placement.selected = showHealEffect ? EPlacementTarget::HEAL_EFFECT : EPlacementTarget::HERO; break;
    default: placement.selected = EPlacementTarget::HERO; break;
    }
}
}

void AsciiArt::RenderNextBattleTransition()
{
    const SceneConfig config = LoadSceneConfig();
    if (RenderStaticImage(config.nextBattleTransitionImagePath, true, 0, EStaticArtStyle::Braille,
                          config.contrastValue, config.outputPixelWidth, config.characterHeightScaleValue))
    {
        DrawCenteredTextOnClearPanel(L"다음 전투가 시작됩니다", 0.50f);
        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, config.nextBattleTransitionMilliseconds)));
    }
}

void AsciiArt::RenderBattleReturnTransition()
{
    const SceneConfig config = LoadSceneConfig();
    if (RenderStaticImage(config.battleReturnTransitionImagePath, true, 0, EStaticArtStyle::Braille,
                          config.contrastValue, config.outputPixelWidth, config.characterHeightScaleValue))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(0, config.battleTransitionMilliseconds)));
    }
}

bool AsciiArt::RenderPulsingMainMenuImage(const std::wstring& imagePath, double elapsedSeconds)
{
    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok)
    {
        return false;
    }

    bool didRenderScene = false;
    {
        Gdiplus::Bitmap image(imagePath.c_str());
        if (image.GetLastStatus() == Gdiplus::Ok)
        {
            const UINT sceneWidth = image.GetWidth();
            const UINT sceneHeight = image.GetHeight();
            Gdiplus::Bitmap scene(sceneWidth, sceneHeight, PixelFormat32bppARGB);
            Gdiplus::Graphics graphics(&scene);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(&image, 0, 0, static_cast<INT>(sceneWidth), static_cast<INT>(sceneHeight));

            // 광원은 원본 PNG를 건드리지 않고, 렌더링 직전에만 덧그립니다.
            // 낮에는 은은하게, 밤에는 랜턴/횃불이 더 분명하게 숨쉬도록 만듭니다.
            const bool isNight = imagePath.find(L"night") != std::wstring::npos;
            // 각 광원은 서로 다른 속도와 시작 위상을 사용합니다.
            // 따라서 모든 조명이 동시에 밝아졌다 어두워지는 느낌을 피합니다.
            // 원본 PNG에는 저장되지 않고, 매 프레임의 임시 장면에만 적용됩니다.
            const auto drawGlow = [&](float centerX, float centerY, float radius, double phase)
            {
                // 광원 하나가 한 번 밝아졌다 돌아오는 데 약 7~13초가 걸립니다.
                // 각 광원은 속도가 달라 동시에 박자 맞춰 움직이지 않습니다.
                const double speed = 0.50 + phase * 0.08;
                const double localWave = (std::sin(elapsedSeconds * speed + phase) + 1.0) * 0.5;
                // 이전보다 변화 폭은 줄이되, AA로도 밝기 차이가 남도록 조절합니다.
                const float strength = isNight
                    ? static_cast<float>(0.58 + localWave * 0.58)
                    : static_cast<float>(0.30 + localWave * 0.48);
                // 불빛 가장자리의 도트가 가만히 멈춰 보이지 않도록 1~2픽셀 범위만 천천히 흔듭니다.
                const float offsetX = static_cast<float>(std::sin(elapsedSeconds * 1.20 + phase) * sceneWidth * 0.0012);
                const float offsetY = static_cast<float>(std::cos(elapsedSeconds * 1.65 + phase) * sceneHeight * 0.0018);
                centerX += offsetX;
                centerY += offsetY;

                for (int ring = 5; ring >= 1; --ring)
                {
                    const float ringRatio = static_cast<float>(ring) / 5.0f;
                    const float radiusPulse = 0.92f + static_cast<float>(localWave) * 0.14f;
                    const float currentRadius = radius * ringRatio * radiusPulse;
                    const float opacity = strength * (1.0f - ringRatio + 0.26f) * 205.0f;
                    Gdiplus::SolidBrush glow(Gdiplus::Color(
                        static_cast<BYTE>(std::clamp(opacity, 0.0f, 245.0f)),
                        255, 208, 104));
                    graphics.FillEllipse(&glow,
                        centerX - currentRadius,
                        centerY - currentRadius,
                        currentRadius * 2.0f,
                        currentRadius * 2.0f);
                }

                // 중심은 더 밝게, 주변의 작은 불티는 매우 느리게 이동합니다.
                const float coreRadius = radius * (0.14f + static_cast<float>(localWave) * 0.055f);
                Gdiplus::SolidBrush core(Gdiplus::Color(230, 255, 244, 184));
                graphics.FillEllipse(&core,
                    centerX - coreRadius,
                    centerY - coreRadius,
                    coreRadius * 2.0f,
                    coreRadius * 2.0f);

                for (int particle = 0; particle < 3; ++particle)
                {
                    const double particlePhase = phase + particle * 2.13;
                    const float particleX = centerX + static_cast<float>(std::sin(elapsedSeconds * 0.85 + particlePhase) * radius * 0.55f);
                    const float particleY = centerY - radius * (0.18f + particle * 0.12f) +
                        static_cast<float>(std::cos(elapsedSeconds * 1.05 + particlePhase) * radius * 0.18f);
                    const float particleRadius = std::max(1.5f, radius * 0.045f);
                    Gdiplus::SolidBrush ember(Gdiplus::Color(
                        static_cast<BYTE>(145 + localWave * 70.0), 255, 222, 128));
                    graphics.FillEllipse(&ember,
                        particleX - particleRadius,
                        particleY - particleRadius,
                        particleRadius * 2.0f,
                        particleRadius * 2.0f);
                }
            };

            // 상점 창/랜턴, 길드 랜턴, 전투 길 횃불의 상대 위치입니다.
            drawGlow(sceneWidth * 0.17f, sceneHeight * 0.53f, sceneWidth * 0.050f, 0.0);
            drawGlow(sceneWidth * 0.25f, sceneHeight * 0.55f, sceneWidth * 0.045f, 1.4);
            drawGlow(sceneWidth * 0.85f, sceneHeight * 0.55f, sceneWidth * 0.060f, 2.8);
            drawGlow(sceneWidth * 0.38f, sceneHeight * 0.59f, sceneWidth * 0.045f, 4.2);

            const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD originalOutputMode = 0;
            const bool canReadOutputMode = GetConsoleMode(output, &originalOutputMode) != FALSE;
            const bool previousAnsiColorSupported = gAnsiColorSupported;
            gAnsiColorSupported = canReadOutputMode &&
                SetConsoleMode(output, originalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != FALSE;

            const SceneConfig config = LoadSceneConfig();
            RenderSettings settings;
            settings.outputPixelWidth = ClampSetting(config.mainMenuOutputPixelWidth);
            settings.characterHeightScaleValue = ClampSetting(config.mainMenuCharacterHeightScale);
            settings.contrastValue = ClampSetting(config.mainMenuContrast);
            settings.useOrderedDithering = config.useOrderedDithering;
            settings.useAnsiColor = config.useAnsiColor && gAnsiColorSupported;
            settings.colorMode = config.colorMode;

            const COORD consoleSize = GetVisibleConsoleSize(output);
            // 메뉴 한 줄과 입력 한 줄을 이미지 아래에 남깁니다.
            const int availableRows = std::max(4, static_cast<int>(consoleSize.Y) - 3);
            const int maximumWidthByHeight = static_cast<int>(availableRows * 4.0 *
                (static_cast<double>(scene.GetWidth()) / scene.GetHeight()) / GetCharacterHeightScale(settings));
            const int maximumOutputPixelWidth = std::max(16, std::min(
                static_cast<int>(consoleSize.X) * 2, maximumWidthByHeight));
            const std::vector<std::wstring> artLines =
                CreateBrailleLines(scene, settings, maximumOutputPixelWidth);

            CONSOLE_SCREEN_BUFFER_INFO outputInfo;
            GetConsoleScreenBufferInfo(output, &outputInfo);
            const int artWidth = artLines.empty() ? 0 : GetVisibleConsoleDisplayWidth(artLines.front());
            gStaticImageLeft = outputInfo.srWindow.Left;
            gStaticImageWidth = static_cast<short>(artWidth);
            gStaticImageTop = outputInfo.srWindow.Top;
            gStaticImageHeight = static_cast<short>(artLines.size());
            gHasStaticImageBounds = artWidth > 0;

            // 이전 프레임과 달라진 행만 출력하므로, 광원만 움직일 때 전체 화면을 지우지 않습니다.
            WriteLayeredFrame(output, artLines, gStaticImageLeft, gStaticImageTop, gStaticImageWidth);
            SetConsoleCursorPosition(output, {
                gStaticImageLeft,
                static_cast<short>(gStaticImageTop + gStaticImageHeight)
            });

            if (canReadOutputMode) SetConsoleMode(output, originalOutputMode);
            gAnsiColorSupported = previousAnsiColorSupported;
            didRenderScene = true;
        }
    }

    Gdiplus::GdiplusShutdown(token);
    return didRenderScene;
}

bool AsciiArt::RunStaticImageTuner(const std::wstring& imagePath, bool useColor)
{
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD originalInputMode = 0;
    const bool canReadInputMode = GetConsoleMode(input, &originalInputMode) != FALSE;
    if (canReadInputMode)
    {
        SetConsoleMode(
            input,
            (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
    }

    SceneConfig config = LoadSceneConfig();
    int inkDensity = ClampSetting(config.startScreenInkDensity);
    int contrast = ClampSetting(config.startScreenContrast);
    ESlider activeSlider = ESlider::NONE;

    auto drawControls = [&]()
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
        {
            return std::array<SliderLayout, 2>{};
        }

        const short visibleWidth = static_cast<short>(consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1);
        const short densityY = static_cast<short>(std::max<short>(consoleInfo.srWindow.Top, consoleInfo.srWindow.Bottom - 2));
        const std::wstring densityLine = L"점 농도  " + CreateSliderTrack(inkDensity) + L" " + std::to_wstring(inkDensity);
        const std::wstring contrastLine = L"대비      " + CreateSliderTrack(contrast) + L" " + std::to_wstring(contrast);
        const int panelWidth = std::max(GetConsoleDisplayWidth(densityLine), GetConsoleDisplayWidth(contrastLine));
        const short panelX = static_cast<short>(consoleInfo.srWindow.Left + std::max(0, (visibleWidth - panelWidth) / 2));

        WriteAt(output, panelX, densityY, densityLine + std::wstring(std::max(0, panelWidth - GetConsoleDisplayWidth(densityLine)), L' '));
        WriteAt(output, panelX, static_cast<short>(densityY + 1), contrastLine + std::wstring(std::max(0, panelWidth - GetConsoleDisplayWidth(contrastLine)), L' '));
        WriteAt(output, panelX, consoleInfo.srWindow.Bottom, L"마우스 조절 / S: 저장 / ENTER 또는 SPACE: 계속");

        return std::array<SliderLayout, 2>{
            SliderLayout{ESlider::OUTPUT_WIDTH, panelX, densityY, static_cast<short>(panelX + 9)},
            SliderLayout{ESlider::CONTRAST, panelX, static_cast<short>(densityY + 1), static_cast<short>(panelX + 9)},
        };
    };

    while (true)
    {
        if (!RenderStaticImage(imagePath, useColor, inkDensity, EStaticArtStyle::Braille, contrast))
        {
            if (canReadInputMode) SetConsoleMode(input, originalInputMode);
            return false;
        }

        const std::array<SliderLayout, 2> sliders = drawControls();
        INPUT_RECORD record{};
        DWORD read = 0;
        ReadConsoleInputW(input, &record, 1, &read);

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
        {
            const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
            if (key == VK_RETURN || key == VK_SPACE)
            {
                if (canReadInputMode) SetConsoleMode(input, originalInputMode);
                return true;
            }
            if (key == 'S')
            {
                config.startScreenInkDensity = inkDensity;
                config.startScreenContrast = contrast;
                SaveSceneConfig(config);
            }
            continue;
        }

        if (record.EventType != MOUSE_EVENT)
        {
            continue;
        }

        const MOUSE_EVENT_RECORD& mouse = record.Event.MouseEvent;
        const bool pressed = (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
        if (pressed && mouse.dwEventFlags == 0)
        {
            activeSlider = ESlider::NONE;
            for (const SliderLayout& slider : sliders)
            {
                const short endX = static_cast<short>(slider.sliderStartX + kSliderWidth + 1);
                if (mouse.dwMousePosition.Y == slider.y && mouse.dwMousePosition.X >= slider.sliderStartX && mouse.dwMousePosition.X <= endX)
                {
                    activeSlider = slider.slider;
                    break;
                }
            }
        }

        if (pressed && activeSlider != ESlider::NONE)
        {
            const SliderLayout& slider = activeSlider == ESlider::OUTPUT_WIDTH ? sliders[0] : sliders[1];
            const int position = std::clamp(static_cast<int>(mouse.dwMousePosition.X - slider.sliderStartX - 1), 0, kSliderWidth);
            const int value = position * 1000 / kSliderWidth;
            if (activeSlider == ESlider::OUTPUT_WIDTH)
            {
                inkDensity = value;
            }
            else
            {
                contrast = value;
            }
        }
        else if (!pressed)
        {
            activeSlider = ESlider::NONE;
        }
    }
}

bool AsciiArt::RunMainMenuImageTuner(const std::wstring& imagePath)
{
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD originalInputMode = 0;
    const bool canReadInputMode = GetConsoleMode(input, &originalInputMode) != FALSE;
    if (canReadInputMode)
    {
        SetConsoleMode(input, (originalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
    }

    SceneConfig config = LoadSceneConfig();
    int outputPixelWidth = ClampSetting(config.mainMenuOutputPixelWidth);
    int characterHeightScale = ClampSetting(config.mainMenuCharacterHeightScale);
    int contrast = ClampSetting(config.mainMenuContrast);
    ESlider activeSlider = ESlider::NONE;

    auto drawControls = [&]()
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
        {
            return std::array<SliderLayout, 3>{};
        }

        constexpr short kLabelWidth = 10;
        const short visibleWidth = static_cast<short>(consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1);
        const short firstRow = static_cast<short>(std::max<short>(consoleInfo.srWindow.Top, consoleInfo.srWindow.Bottom - 3));
        const std::array<std::wstring, 3> lines{
            L"해상도    " + CreateSliderTrack(outputPixelWidth) + L" " + std::to_wstring(outputPixelWidth),
            L"세로 비율 " + CreateSliderTrack(characterHeightScale) + L" " + std::to_wstring(characterHeightScale),
            L"대비      " + CreateSliderTrack(contrast) + L" " + std::to_wstring(contrast),
        };
        const int panelWidth = std::max({ GetConsoleDisplayWidth(lines[0]), GetConsoleDisplayWidth(lines[1]), GetConsoleDisplayWidth(lines[2]) });
        const short panelX = static_cast<short>(consoleInfo.srWindow.Left + std::max(0, (visibleWidth - panelWidth) / 2));

        for (short index = 0; index < 3; ++index)
        {
            WriteAt(output, panelX, static_cast<short>(firstRow + index),
                lines[static_cast<size_t>(index)] + std::wstring(std::max(0, panelWidth - GetConsoleDisplayWidth(lines[static_cast<size_t>(index)])), L' '));
        }
        WriteAt(output, panelX, consoleInfo.srWindow.Bottom, L"[메인 메뉴 조절] 마우스 / S: 저장 / ENTER 또는 SPACE: 돌아가기");

        return std::array<SliderLayout, 3>{
            SliderLayout{ESlider::OUTPUT_WIDTH, panelX, firstRow, static_cast<short>(panelX + kLabelWidth)},
            SliderLayout{ESlider::HEIGHT_SCALE, panelX, static_cast<short>(firstRow + 1), static_cast<short>(panelX + kLabelWidth)},
            SliderLayout{ESlider::CONTRAST, panelX, static_cast<short>(firstRow + 2), static_cast<short>(panelX + kLabelWidth)},
        };
    };

    while (true)
    {
        if (!RenderStaticImage(imagePath, true, 0, EStaticArtStyle::Braille, contrast, outputPixelWidth, characterHeightScale))
        {
            if (canReadInputMode) SetConsoleMode(input, originalInputMode);
            return false;
        }

        const std::array<SliderLayout, 3> sliders = drawControls();
        INPUT_RECORD record{};
        DWORD read = 0;
        ReadConsoleInputW(input, &record, 1, &read);

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
        {
            const WORD key = record.Event.KeyEvent.wVirtualKeyCode;
            if (key == VK_RETURN || key == VK_SPACE)
            {
                if (canReadInputMode) SetConsoleMode(input, originalInputMode);
                return true;
            }
            if (key == 'S')
            {
                config.mainMenuOutputPixelWidth = outputPixelWidth;
                config.mainMenuCharacterHeightScale = characterHeightScale;
                config.mainMenuContrast = contrast;
                SaveSceneConfig(config);
            }
            continue;
        }

        if (record.EventType != MOUSE_EVENT)
        {
            continue;
        }

        const MOUSE_EVENT_RECORD& mouse = record.Event.MouseEvent;
        const bool pressed = (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
        if (pressed && mouse.dwEventFlags == 0)
        {
            activeSlider = ESlider::NONE;
            for (const SliderLayout& slider : sliders)
            {
                const short endX = static_cast<short>(slider.sliderStartX + kSliderWidth + 1);
                if (mouse.dwMousePosition.Y == slider.y && mouse.dwMousePosition.X >= slider.sliderStartX && mouse.dwMousePosition.X <= endX)
                {
                    activeSlider = slider.slider;
                    break;
                }
            }
        }

        if (pressed && activeSlider != ESlider::NONE)
        {
            const SliderLayout* activeLayout = nullptr;
            for (const SliderLayout& slider : sliders)
            {
                if (slider.slider == activeSlider)
                {
                    activeLayout = &slider;
                    break;
                }
            }
            if (activeLayout != nullptr)
            {
                const int position = std::clamp(static_cast<int>(mouse.dwMousePosition.X - activeLayout->sliderStartX - 1), 0, kSliderWidth);
                const int value = position * 1000 / kSliderWidth;
                if (activeSlider == ESlider::OUTPUT_WIDTH) outputPixelWidth = value;
                if (activeSlider == ESlider::HEIGHT_SCALE) characterHeightScale = value;
                if (activeSlider == ESlider::CONTRAST) contrast = value;
            }
        }
        else if (!pressed)
        {
            activeSlider = ESlider::NONE;
        }
    }
}

void AsciiArt::ClearScreen()
{
    ClearConsole(GetStdHandle(STD_OUTPUT_HANDLE));
    gHasStaticImageBounds = false;
}

void AsciiArt::ClearBottomRows(int rowCount)
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    const short visibleWidth =
        static_cast<short>(consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1);
    const short firstRow = static_cast<short>(std::max(
        static_cast<int>(consoleInfo.srWindow.Top),
        static_cast<int>(consoleInfo.srWindow.Bottom) - std::max(0, rowCount) + 1));

    for (short y = firstRow; y <= consoleInfo.srWindow.Bottom; ++y)
    {
        WriteAt(output, consoleInfo.srWindow.Left, y, std::wstring(visibleWidth, L' '));
    }
}

void AsciiArt::ScrollScreenOneLine()
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    SetConsoleCursorPosition(output, {consoleInfo.srWindow.Left, consoleInfo.srWindow.Bottom});
    DWORD written = 0;
    WriteConsoleW(output, L"\n", 1, &written, nullptr);
}

void AsciiArt::MoveCursorBelowStaticImage(int blankRowCount)
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    if (!gHasStaticImageBounds)
    {
        SetConsoleCursorPosition(output, {consoleInfo.srWindow.Left, consoleInfo.srWindow.Top});
        return;
    }

    const short y = static_cast<short>(std::min(
        static_cast<int>(consoleInfo.srWindow.Bottom),
        static_cast<int>(gStaticImageTop) + gStaticImageHeight + std::max(0, blankRowCount)));
    SetConsoleCursorPosition(output, {gStaticImageLeft, y});
}

void AsciiArt::DrawCenteredText(const std::wstring& text, float verticalRatio)
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    const short visibleWidth =
        static_cast<short>(consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1);
    const short visibleHeight =
        static_cast<short>(consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1);
    const short alignmentLeft = gHasStaticImageBounds ? gStaticImageLeft : consoleInfo.srWindow.Left;
    const short alignmentWidth = gHasStaticImageBounds ? gStaticImageWidth : visibleWidth;

    const int textWidth = GetConsoleDisplayWidth(text);
    const short x = static_cast<short>(alignmentLeft + std::max(0, (alignmentWidth - textWidth) / 2));
    const short y = static_cast<short>(consoleInfo.srWindow.Top + std::clamp(
        static_cast<int>((visibleHeight - 1) * verticalRatio),
        0,
        std::max(0, static_cast<int>(visibleHeight) - 1)
    ));

    WriteAt(output, x, y, text);
}

void AsciiArt::DrawCenteredTextOnClearPanel(const std::wstring& text, float verticalRatio)
{
    // 콘솔은 글자마다 배경색을 따로 칠하기보다 공백을 덮어쓰는 편이 안정적입니다.
    // 좌우 여백까지 공백으로 덮어 ASCII 배경과 메뉴 글자가 겹치지 않게 합니다.
    constexpr int kHorizontalPadding = 2;

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    const short visibleWidth =
        static_cast<short>(consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1);
    const short visibleHeight =
        static_cast<short>(consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1);
    const short alignmentLeft = gHasStaticImageBounds ? gStaticImageLeft : consoleInfo.srWindow.Left;
    const short alignmentWidth = gHasStaticImageBounds ? gStaticImageWidth : visibleWidth;

    const std::wstring panel =
        std::wstring(kHorizontalPadding, L' ') + text +
        std::wstring(kHorizontalPadding, L' ');
    const int panelWidth = GetConsoleDisplayWidth(panel);
    const short x = static_cast<short>(alignmentLeft + std::max(0, (alignmentWidth - panelWidth) / 2));
    const short y = static_cast<short>(consoleInfo.srWindow.Top + std::clamp(
        static_cast<int>((visibleHeight - 1) * verticalRatio),
        0,
        std::max(0, static_cast<int>(visibleHeight) - 1)
    ));

    WriteAt(output, x, y, panel);
}

void AsciiArt::DrawStaticImageText(
    const std::wstring& text,
    float horizontalRatio,
    float verticalRatio,
    bool useGoldColor,
    int rowOffset)
{
    if (!gHasStaticImageBounds)
    {
        return;
    }

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(output, &consoleInfo) == FALSE)
    {
        return;
    }

    const int textWidth = GetConsoleDisplayWidth(text);
    const int imageRight = gStaticImageLeft + gStaticImageWidth;
    const int targetCenter = gStaticImageLeft + static_cast<int>(
        static_cast<float>(gStaticImageWidth) * std::clamp(horizontalRatio, 0.0f, 1.0f)
    );
    const short x = static_cast<short>(std::clamp(
        targetCenter - textWidth / 2,
        static_cast<int>(gStaticImageLeft),
        std::max(static_cast<int>(gStaticImageLeft), imageRight - textWidth)
    ));
    const short y = static_cast<short>(std::clamp(
        static_cast<int>(gStaticImageTop) + static_cast<int>(
            static_cast<float>(std::max<short>(1, gStaticImageHeight - 1)) *
            std::clamp(verticalRatio, 0.0f, 1.0f)
        ) + rowOffset,
        static_cast<int>(gStaticImageTop),
        static_cast<int>(consoleInfo.srWindow.Bottom)
    ));

    const std::wstring panel(static_cast<size_t>(textWidth), L' ');
    const WORD originalAttributes = consoleInfo.wAttributes;
    SetConsoleTextAttribute(output, 0);
    SetConsoleCursorPosition(output, {x, y});
    DWORD written = 0;
    WriteConsoleW(output, panel.c_str(), static_cast<DWORD>(panel.size()), &written, nullptr);

    const WORD textColor = useGoldColor
        ? static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
        : kTextColor;
    SetConsoleTextAttribute(output, textColor);
    SetConsoleCursorPosition(output, {x, y});
    WriteConsoleW(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
    SetConsoleTextAttribute(output, originalAttributes);
}

bool AsciiArt::GetStaticImageBounds(short& left, short& top, short& width, short& height)
{
    if (!gHasStaticImageBounds) return false;
    left = gStaticImageLeft;
    top = gStaticImageTop;
    width = gStaticImageWidth;
    height = gStaticImageHeight;
    return true;
}

namespace
{
using AsciiArt::ActorBattleStatus;
using AsciiArt::BattleSceneState;
std::wstring ResolveDemoImagePath(const std::wstring& configuredPath)
{
    // 이전 테스트 설정에는 파일명만 저장되어 있을 수 있습니다.
    // 프로젝트의 실제 리소스 위치를 한 번 더 확인해 전투 화면이 바로 열리게 합니다.
    if (GetFileAttributesW(configuredPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        return configuredPath;
    }

    if (configuredPath.find(L'\\') == std::wstring::npos &&
        configuredPath.find(L'/') == std::wstring::npos)
    {
        const std::wstring resourcePath = L"Resources\\Images\\" + configuredPath;
        if (GetFileAttributesW(resourcePath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            return resourcePath;
        }
    }

    return configuredPath;
}
}

int AsciiArt::RunStandaloneDemo(
    int heroTurnCount,
    const BattleActionCallback& onBattleAction,
    const BattleStateProvider& getBattleState,
    bool potionOnlyTestMode)
{
    Gdiplus::GdiplusStartupInput startupInput;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok)
    {
        return 1;
    }
    // 이 객체를 이미지보다 먼저 만들면 함수가 끝날 때 이미지는 먼저 파괴되고,
    // 그 다음에만 GDI+가 종료됩니다. ESC 종료 시 접근 위반을 막는 수명 순서입니다.
    struct FGdiPlusLifetime
    {
        ULONG_PTR token = 0;
        ~FGdiPlusLifetime() { Gdiplus::GdiplusShutdown(token); }
    } gdiPlusLifetime{token};

    SceneConfig config = LoadSceneConfig();
    // 이전 테스트 이미지 설정을 쓰고 있었다면, 새 파티 스프라이트 대열의 기본값으로 한 번 전환합니다.
    // 이후에는 SceneConfig.jsonc의 같은 키를 사용자가 직접 수정하고 저장하면 그 값을 계속 사용합니다.
    if (config.heroImagePath == L"3561912736885df2.png")
    {
        config.heroImagePath = L"Resources\\Images\\Characters\\warrior_back_v2.png";
        config.hero2ImagePath = L"Resources\\Images\\Characters\\mage_back_v2.png";
        config.tankImagePath = L"Resources\\Images\\Characters\\tank_back_guard_selected.png";
        config.warriorWeaponImagePath = L"Resources\\Images\\Weapons\\warrior_greatsword.png";
        config.mageWeaponImagePath = L"Resources\\Images\\Weapons\\mage_staff.png";
        config.tankWeaponImagePath = L"Resources\\Images\\Weapons\\tank_monarch_shield_back_upright.png";
        config.heroX = 65.0f;  config.heroY = 305.0f; config.heroWidth = 205.0f; config.heroHeight = 205.0f;
        config.hero2X = 475.0f; config.hero2Y = 300.0f; config.hero2Width = 195.0f; config.hero2Height = 205.0f;
        config.tankX = 255.0f; config.tankY = 205.0f; config.tankWidth = 285.0f; config.tankHeight = 300.0f;
        config.tankShieldX = 420.0f; config.tankShieldY = 225.0f; config.tankShieldWidth = 148.0f; config.tankShieldHeight = 250.0f;
        // 기존 테스트 설정을 처음 한 번만 직업별 파일명 설정으로 마이그레이션합니다.
        // 이후에는 SceneConfig.jsonc의 warrior/tank/mage 항목만 수정하면 됩니다.
        SaveSceneConfig(config);
    }
    const std::wstring heroImagePath = ResolveDemoImagePath(config.heroImagePath);
    const std::wstring hero2ImagePath = ResolveDemoImagePath(config.hero2ImagePath);
    const std::wstring tankImagePath = ResolveDemoImagePath(config.tankImagePath);
    const std::wstring monsterImagePath = ResolveDemoImagePath(config.monsterImagePath);
    Gdiplus::Image hero(heroImagePath.c_str());
    Gdiplus::Image hero2(hero2ImagePath.c_str());
    Gdiplus::Image tank(tankImagePath.c_str());
    Gdiplus::Image monster(monsterImagePath.c_str());
    if (hero.GetLastStatus() != Gdiplus::Ok || monster.GetLastStatus() != Gdiplus::Ok)
    {
        std::wcerr << L"이미지 파일을 열지 못했습니다. 파일명과 실행 작업 폴더를 확인해 주세요.\n";
        return 1;
    }
    Gdiplus::Image* hero2ToRender = hero2.GetLastStatus() == Gdiplus::Ok ? &hero2 : nullptr;
    Gdiplus::Image* tankToRender = tank.GetLastStatus() == Gdiplus::Ok ? &tank : nullptr;

    // 무기는 선택 사항입니다. 아직 파일을 넣지 않은 직업은 기존 은색 기본 검으로 보입니다.
    const auto loadOptionalWeapon = [](const std::wstring& configuredPath) -> std::unique_ptr<Gdiplus::Image>
    {
        if (configuredPath.empty())
        {
            return nullptr;
        }

        auto image = std::make_unique<Gdiplus::Image>(ResolveDemoImagePath(configuredPath).c_str());
        return image->GetLastStatus() == Gdiplus::Ok ? std::move(image) : nullptr;
    };
    const std::unique_ptr<Gdiplus::Image> warriorWeapon = loadOptionalWeapon(config.warriorWeaponImagePath);
    const std::unique_ptr<Gdiplus::Image> mageWeapon = loadOptionalWeapon(config.mageWeaponImagePath);
    const std::unique_ptr<Gdiplus::Image> tankWeapon = loadOptionalWeapon(config.tankWeaponImagePath);
    const std::array<std::unique_ptr<Gdiplus::Image>, 3> hitEffects = {
        loadOptionalWeapon(config.hitEffect30ImagePath),
        loadOptionalWeapon(config.hitEffect45ImagePath),
        loadOptionalWeapon(config.hitEffect55ImagePath),
    };
    const std::array<Gdiplus::Image*, 3> hitEffectImages = {
        hitEffects[0].get(), hitEffects[1].get(), hitEffects[2].get(),
    };
    const std::array<std::unique_ptr<Gdiplus::Image>, 3> heroSlashEffects = {
        loadOptionalWeapon(config.heroSlash30ImagePath),
        loadOptionalWeapon(config.heroSlash45ImagePath),
        loadOptionalWeapon(config.heroSlash55ImagePath),
    };
    const std::array<Gdiplus::Image*, 3> heroSlashEffectImages = {
        heroSlashEffects[0].get(), heroSlashEffects[1].get(), heroSlashEffects[2].get(),
    };
    const std::unique_ptr<Gdiplus::Image> healEffect = loadOptionalWeapon(config.healEffectImagePath);
    const std::unique_ptr<Gdiplus::Image> powerBuffEffect = loadOptionalWeapon(config.powerBuffEffectImagePath);
    const std::unique_ptr<Gdiplus::Image> battleBackground = loadOptionalWeapon(config.battleBackgroundImagePath);

    // 전투 중 프레임마다 파일을 다시 열지 않도록, 실제로 등장한 몬스터 스프라이트만 캐시합니다.
    // 경로를 찾지 못한 경우에는 설정 파일의 기존 monster_image를 안전한 대체 이미지로 씁니다.
    std::map<std::wstring, std::unique_ptr<Gdiplus::Image>> monsterImageCache;
    const auto loadMonsterVisual = [&](const ActorBattleStatus& status) -> Gdiplus::Image*
    {
        const std::wstring configuredPath = GetMonsterSpritePath(status, config);
        const std::wstring resolvedPath = ResolveDemoImagePath(configuredPath);
        const auto existing = monsterImageCache.find(resolvedPath);
        if (existing != monsterImageCache.end())
        {
            return existing->second.get();
        }

        auto image = std::make_unique<Gdiplus::Image>(resolvedPath.c_str());
        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            return &monster;
        }
        Gdiplus::Image* result = image.get();
        monsterImageCache.emplace(resolvedPath, std::move(image));
        return result;
    };

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo;
    GetConsoleScreenBufferInfo(output, &originalConsoleInfo);

    DWORD originalOutputMode = 0;
    GetConsoleMode(output, &originalOutputMode);
    const bool ansiColorSupported =
        SetConsoleMode(output, originalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != FALSE;
    gAnsiColorSupported = ansiColorSupported;

    DWORD originalInputMode = 0;
    GetConsoleMode(input, &originalInputMode);
    const DWORD inputMode =
        (originalInputMode | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT) & ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(input, inputMode);
    FlushConsoleInputBuffer(input);

    CONSOLE_CURSOR_INFO originalCursor;
    GetConsoleCursorInfo(output, &originalCursor);
    CONSOLE_CURSOR_INFO hiddenCursor = originalCursor;
    hiddenCursor.bVisible = FALSE;
    SetConsoleCursorInfo(output, &hiddenCursor);

    RenderSettings settings;
    settings.outputPixelWidth = config.outputPixelWidth;
    settings.characterHeightScaleValue = config.characterHeightScaleValue;
    settings.contrastValue = config.contrastValue;
    settings.useOrderedDithering = config.useOrderedDithering;
    settings.useAnsiColor = config.useAnsiColor;
    settings.colorMode = config.colorMode;
    if (!ansiColorSupported)
    {
        settings.useAnsiColor = false;
    }
    AttackAnimation attack;
    int currentTurn = 0;
    PlacementMode placement;
    bool showDeveloperPanel = false;
    ESlider activeSlider = ESlider::NONE;
    ControlPanelLayout layout;
    bool running = true;
    bool manualAttackMode = false;
    const int safeHeroTurnCount = std::max(1, heroTurnCount);
    bool pendingMonsterPhase = false;
    // 실제 전투 콜백이 종료를 알려도, 마지막 HP 잔상이 정리될 때까지 화면을 유지합니다.
    bool pendingBattleExit = false;
    // 기본 전투는 사망한 적을 즉시 목록에서 제거합니다.
    // AA 화면은 마지막 HP 0 프레임을 남겨야 하므로, 종료 직전 상태를 잠깐 보관합니다.
    BattleSceneState finalBattleState;
    bool hasFinalBattleState = false;
    int currentMonsterTurn = 0;
    int requestedMonsterIndex = 0;
    int requestedTestCommand = 0;
    bool showHealEffectPreview = false;
    bool showPowerBuffEffectPreview = false;
    bool autoBattleEnabled = false;
    bool sameMonsterPlacementTestMode = false;
    int sameMonsterTestTypeIndex = 0;
    bool autoBattleMonsterPhase = false;
    bool battleRevealPlayed = false;
    std::deque<BattleAction> testActionQueue;
    std::map<std::string, float> displayedHp;
    gBattleMonsterInstanceProfiles.clear();
    gBattleMonsterInstanceTypes.clear();
    auto nextAutomaticAttackAt = std::chrono::steady_clock::now();
    const auto startedAt = std::chrono::steady_clock::now();
    Gdiplus::Bitmap scene(config.sceneWidth, config.sceneHeight, PixelFormat32bppARGB);
    ArtResolution resolution;

    while (running)
    {
        BattleSceneState battleState = getBattleState ? getBattleState() : BattleSceneState{};
        if (pendingBattleExit && hasFinalBattleState)
        {
            battleState = finalBattleState;
        }
        if (sameMonsterPlacementTestMode)
        {
            // 게임 전투 데이터는 건드리지 않고, 배치 확인용 표시 상태만 4마리로 바꿉니다.
            ActorBattleStatus prototype;
            if (!battleState.monsterStatuses.empty()) prototype = battleState.monsterStatuses.front();
            prototype.currentHp = std::max(1, prototype.currentHp);
            prototype.maximumHp = std::max(prototype.currentHp, prototype.maximumHp);
            prototype.previousHp = prototype.currentHp;
            prototype.isDead = false;
            prototype.displayName = GetMonsterDisplayNames()[static_cast<size_t>(sameMonsterTestTypeIndex)];

            battleState.monsterStatuses.clear();
            for (int slot = 0; slot < 4; ++slot)
            {
                ActorBattleStatus preview = prototype;
                preview.id = "debug_monster_slot_" + std::to_string(slot + 1);
                battleState.monsterStatuses.push_back(std::move(preview));
            }
        }
        const int playerCount = battleState.playerStatuses.empty() ? safeHeroTurnCount : static_cast<int>(battleState.playerStatuses.size());
        const int monsterCount = static_cast<int>(battleState.monsterStatuses.size());
        // 실제 등장 순서대로 슬롯 1~4의 위치·크기를 한 번씩 복사합니다.
        // 같은 종류가 여럿이어도 서로 다른 슬롯을 쓰며, 죽어도 남은 몬스터가 이동하지 않습니다.
        for (int index = 0; index < monsterCount; ++index)
        {
            const ActorBattleStatus& status = battleState.monsterStatuses[index];
            if (gBattleMonsterInstanceProfiles.find(status.id) == gBattleMonsterInstanceProfiles.end())
            {
                gBattleMonsterInstanceProfiles.emplace(status.id, GetMonsterSlotProfile(config, index, status));
                gBattleMonsterInstanceTypes.emplace(
                    status.id,
                    static_cast<int>(GetMonsterVisualProfileIndex(status)));
            }
        }
        // 전투 종료 직후에는 마지막 HP 변화가 느리게 남지 않도록 조금 더 빠르게 정리합니다.
        const float hpAnimationSpeed = pendingBattleExit ? 0.38f : 0.18f;
        for (const ActorBattleStatus& status : battleState.playerStatuses)
        {
            float& shown = displayedHp[status.id];
            if (shown <= 0.0f) shown = static_cast<float>(status.currentHp);
            shown += (static_cast<float>(status.currentHp) - shown) * hpAnimationSpeed;
        }
        for (const ActorBattleStatus& status : battleState.monsterStatuses)
        {
            float& shown = displayedHp[status.id];
            if (shown <= 0.0f) shown = static_cast<float>(status.currentHp);
            shown += (static_cast<float>(status.currentHp) - shown) * hpAnimationSpeed;
        }
        if (pendingBattleExit)
        {
            const auto hpAnimationFinished = [&](const std::vector<ActorBattleStatus>& statuses)
            {
                constexpr int kBarLength = 24;
                for (const ActorBattleStatus& status : statuses)
                {
                    if (status.maximumHp <= 0) continue;
                    const auto shown = displayedHp.find(status.id);
                    const float shownHp = shown == displayedHp.end() ? static_cast<float>(status.currentHp) : shown->second;
                    const int currentLength = static_cast<int>(std::round(
                        std::clamp(static_cast<float>(status.currentHp) / status.maximumHp, 0.0f, 1.0f) * kBarLength));
                    const int shownLength = static_cast<int>(std::round(
                        std::clamp(shownHp / status.maximumHp, 0.0f, 1.0f) * kBarLength));
                    if (shownLength != currentLength) return false;
                }
                return true;
            };
            if (hpAnimationFinished(battleState.playerStatuses) && hpAnimationFinished(battleState.monsterStatuses))
            {
                running = false;
            }
        }

        if (!pendingBattleExit)
        {
            ProcessInput(input, layout, settings, config, currentTurn, placement, resolution, activeSlider, attack, running, showDeveloperPanel, manualAttackMode,
                         potionOnlyTestMode, sameMonsterPlacementTestMode, showHealEffectPreview, showPowerBuffEffectPreview,
                         battleState, requestedMonsterIndex, requestedTestCommand);
        }
        // 임시 테스트 모드: 숫자 입력이 들어올 때만 행동 큐를 만들고, 큐가 끝나면 다시 입력을 기다립니다.
        if (!pendingBattleExit && potionOnlyTestMode && requestedTestCommand != 0 && !attack.playing && testActionQueue.empty() &&
            (!placement.active || requestedTestCommand == 8))
        {
            const int playerCount = static_cast<int>(battleState.playerStatuses.size());
            const int livingMonsterIndex = FindNextLivingMonsterIndex(battleState, requestedMonsterIndex);
            if (requestedTestCommand == 8)
            {
                sameMonsterPlacementTestMode = !sameMonsterPlacementTestMode;
                sameMonsterTestTypeIndex = 0;
                placement = PlacementMode{};
                showDeveloperPanel = false;
                gBattleMonsterInstanceProfiles.clear();
                gBattleMonsterInstanceTypes.clear();
            }
            else if ((requestedTestCommand == 9 || requestedTestCommand == 10) && sameMonsterPlacementTestMode)
            {
                constexpr int kMonsterTypeCount = 9;
                const int direction = requestedTestCommand == 9 ? 1 : -1;
                sameMonsterTestTypeIndex = (sameMonsterTestTypeIndex + direction + kMonsterTypeCount) % kMonsterTypeCount;
                gBattleMonsterInstanceProfiles.clear();
                gBattleMonsterInstanceTypes.clear();
            }
            else if (requestedTestCommand == 5)
            {
                showHealEffectPreview = !showHealEffectPreview;
            }
            else if (requestedTestCommand == 6)
            {
                showPowerBuffEffectPreview = !showPowerBuffEffectPreview;
            }
            else if (requestedTestCommand == 7)
            {
                autoBattleEnabled = !autoBattleEnabled;
                autoBattleMonsterPhase = false;
            }
            else if (requestedTestCommand == 1 || requestedTestCommand == 2)
            {
                if (playerCount > 0)
                {
                    testActionQueue.push_back({ requestedTestCommand == 1 ? EBattleActionType::PlayerUsePotion : EBattleActionType::PlayerUsePowerPotion,
                                                currentTurn % playerCount, 0 });
                    currentTurn = (currentTurn + 1) % playerCount;
                }
            }
            else if (requestedTestCommand == 3 && playerCount > 0 && livingMonsterIndex >= 0)
            {
                for (int index = 0; index < playerCount; ++index)
                {
                    testActionQueue.push_back({ EBattleActionType::PlayerAttack, index, livingMonsterIndex });
                }
            }
            else if (requestedTestCommand == 4 && playerCount > 0)
            {
                for (size_t index = 0; index < battleState.monsterStatuses.size(); ++index)
                {
                    if (!battleState.monsterStatuses[index].isDead)
                    {
                        testActionQueue.push_back({ EBattleActionType::MonsterAttack, static_cast<int>(index), static_cast<int>(index) % playerCount });
                    }
                }
            }
            requestedTestCommand = 0;
        }
        // 테스트 중에도 7번을 켜면 기존 자동 전투처럼 아군 전체 → 몬스터 전체 순서로 반복합니다.
        if (!pendingBattleExit && potionOnlyTestMode && autoBattleEnabled && !placement.active && !attack.playing && testActionQueue.empty())
        {
            const int livingMonsterIndex = FindNextLivingMonsterIndex(battleState, requestedMonsterIndex);
            if (livingMonsterIndex < 0)
            {
                autoBattleEnabled = false;
            }
            else if (!autoBattleMonsterPhase)
            {
                for (int index = 0; index < playerCount; ++index)
                    testActionQueue.push_back({ EBattleActionType::PlayerAttack, index, livingMonsterIndex });
                autoBattleMonsterPhase = true;
            }
            else
            {
                for (size_t index = 0; index < battleState.monsterStatuses.size(); ++index)
                    if (!battleState.monsterStatuses[index].isDead)
                        testActionQueue.push_back({ EBattleActionType::MonsterAttack, static_cast<int>(index), static_cast<int>(index) % std::max(1, playerCount) });
                autoBattleMonsterPhase = false;
            }
        }
        if (!pendingBattleExit && potionOnlyTestMode && !placement.active && !attack.playing && !testActionQueue.empty())
        {
            const BattleAction nextAction = testActionQueue.front();
            testActionQueue.pop_front();
            attack.playing = true;
            attack.hitEffectVariant = PickRandomHitEffectVariant();
            attack.monsterAttacking = nextAction.type == EBattleActionType::MonsterAttack;
            attack.playerUsingPotion = nextAction.type == EBattleActionType::PlayerUsePotion || nextAction.type == EBattleActionType::PlayerUsePowerPotion;
            attack.playerUsingPowerPotion = nextAction.type == EBattleActionType::PlayerUsePowerPotion;
            attack.attackerIndex = nextAction.attackerIndex;
            attack.targetIndex = nextAction.targetIndex;
            attack.startedAt = std::chrono::steady_clock::now();
        }
        // 자동 모드는 영웅이 순서대로 행동합니다. 수동 모드에서는 클릭한 몬스터만 공격하며,
        // 영웅 라운드가 끝나면 몬스터 턴은 두 모드 모두 자동으로 이어집니다.
        if (!potionOnlyTestMode && !pendingBattleExit && pendingMonsterPhase && !placement.active && !attack.playing &&
            std::chrono::steady_clock::now() >= nextAutomaticAttackAt)
        {
            currentMonsterTurn = FindNextLivingMonsterIndex(battleState, currentMonsterTurn);
            if (currentMonsterTurn < 0)
            {
                pendingMonsterPhase = false;
                currentTurn = 0;
            }
            else
            {
                attack.playing = true;
                attack.hitEffectVariant = PickRandomHitEffectVariant();
                attack.monsterAttacking = true;
                attack.attackerIndex = currentMonsterTurn;
                attack.targetIndex = currentMonsterTurn % std::max(1, playerCount);
                attack.startedAt = std::chrono::steady_clock::now();
            }
        }
        else if (!potionOnlyTestMode && !pendingBattleExit && !pendingMonsterPhase && !manualAttackMode && !placement.active && !attack.playing &&
            std::chrono::steady_clock::now() >= nextAutomaticAttackAt)
        {
            const int livingTargetIndex = FindNextLivingMonsterIndex(battleState, requestedMonsterIndex);
            if (livingTargetIndex < 0)
            {
                pendingBattleExit = true;
                finalBattleState = battleState;
                hasFinalBattleState = true;
                continue;
            }
            attack.playing = true;
            attack.playerUsingPotion = potionOnlyTestMode;
            attack.hitEffectVariant = PickRandomHitEffectVariant();
            attack.monsterAttacking = false;
            attack.attackerIndex = currentTurn;
            attack.targetIndex = livingTargetIndex;
            attack.startedAt = std::chrono::steady_clock::now();
        }
        if (attack.playing && GetElapsedSeconds(attack.startedAt) >= 0.72)
        {
            const BattleAction action{
                attack.monsterAttacking ? EBattleActionType::MonsterAttack :
                (attack.playerUsingPowerPotion ? EBattleActionType::PlayerUsePowerPotion :
                    (attack.playerUsingPotion ? EBattleActionType::PlayerUsePotion : EBattleActionType::PlayerAttack)),
                attack.attackerIndex,
                attack.targetIndex};
            attack.playing = false;
            attack.playerUsingPotion = false;
            attack.playerUsingPowerPotion = false;
            // 기본 코드의 즉시 로그가 턴 줄에 이어 붙지 않도록 행동 문구 영역으로 옮깁니다.
            SetConsoleCursorPosition(output, {0, static_cast<short>(layout.artStartY + 2)});
            if (onBattleAction && !onBattleAction(action))
            {
                // 승리/패배 같은 실제 전투 종료는 이미 결정됐지만, HP 바의 빨간 잔상은 끝까지 보여 줍니다.
                pendingBattleExit = true;
                finalBattleState = battleState;
                std::vector<ActorBattleStatus>& defeatedStatuses = action.type == EBattleActionType::PlayerAttack
                    ? finalBattleState.monsterStatuses
                    : finalBattleState.playerStatuses;
                if (!defeatedStatuses.empty())
                {
                    const int defeatedIndex = std::clamp(action.targetIndex, 0, static_cast<int>(defeatedStatuses.size()) - 1);
                    defeatedStatuses[defeatedIndex].previousHp = defeatedStatuses[defeatedIndex].currentHp;
                    defeatedStatuses[defeatedIndex].currentHp = 0;
                    defeatedStatuses[defeatedIndex].isDead = true;
                }
                hasFinalBattleState = true;
            }
            else if (!potionOnlyTestMode && action.type == EBattleActionType::MonsterAttack)
            {
                ++currentMonsterTurn;
                nextAutomaticAttackAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(360);
            }
            else if (!potionOnlyTestMode && currentTurn + 1 >= playerCount)
            {
                pendingMonsterPhase = true;
                currentMonsterTurn = 0;
                nextAutomaticAttackAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(420);
            }
            else if (!potionOnlyTestMode)
            {
                ++currentTurn;
                nextAutomaticAttackAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(420);
            }
        }

        layout = CreateControlPanelLayout(output, showDeveloperPanel);
        std::vector<Gdiplus::Image*> monsterImages;
        monsterImages.reserve(battleState.monsterStatuses.size());
        for (const ActorBattleStatus& status : battleState.monsterStatuses)
        {
            monsterImages.push_back(loadMonsterVisual(status));
        }

        RenderScene(scene, battleBackground.get(), hero, hero2ToRender, tankToRender, monster, monsterImages,
                    warriorWeapon.get(), mageWeapon.get(), tankWeapon.get(), hitEffectImages, heroSlashEffectImages, healEffect.get(), powerBuffEffect.get(),
                    config, placement, attack, &battleState, displayedHp, GetElapsedSeconds(startedAt), showHealEffectPreview, showPowerBuffEffectPreview);
        const int maximumWidth = CalculateMaximumOutputPixelWidth(output, settings, layout, config);
        resolution = CalculateArtResolution(scene, settings, maximumWidth);
        const std::vector<std::wstring> art = CreateBrailleLines(scene, settings, maximumWidth);
        const bool floatingTextVisible = !battleState.floatingTextTargetId.empty() &&
            battleState.floatingTextValue != 0 && battleState.floatingTextAgeSeconds < 0.72;
        if (!battleRevealPlayed)
        {
            PlayVerticalReveal(output, art, layout.artStartY, L"[ 전투 개시 ]");
            battleRevealPlayed = true;
        }
        DrawFrame(output, art, settings, layout, attack.playing, placement.active, showDeveloperPanel, floatingTextVisible);
        DrawBattleHudOverlay(output, battleState, config, layout, resolution, displayedHp, monsterImages,
                              showHealEffectPreview, showPowerBuffEffectPreview, autoBattleEnabled);
        DrawFloatingCombatTextOverlay(output, battleState, config, layout, resolution);
        DrawPlacementStatus(output, layout, resolution, placement);
        const int frameDelayMilliseconds = 1000 / std::max(1, config.framesPerSecond);
        std::this_thread::sleep_for(std::chrono::milliseconds(frameDelayMilliseconds));
    }

    // AA 루프가 끝난 뒤 기본 전투 코드가 출력하는 골드/결과 문구는
    // 상단 턴 표시가 아닌 그림 아래의 왼쪽부터 이어서 출력합니다.
    CONSOLE_SCREEN_BUFFER_INFO finalInfo{};
    if (GetConsoleScreenBufferInfo(output, &finalInfo))
    {
        const short resultY = static_cast<short>(std::clamp(
            layout.artStartY + resolution.pixelHeight / 4 + 1,
            0,
            static_cast<int>(finalInfo.dwSize.Y) - 1));
        SetConsoleCursorPosition(output, {0, resultY});
    }

    SetConsoleMode(input, originalInputMode);
    SetConsoleMode(output, originalOutputMode);
    SetConsoleCursorInfo(output, &originalCursor);
    SetConsoleTextAttribute(output, originalConsoleInfo.wAttributes);
    return 0;
}
