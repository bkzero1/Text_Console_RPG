#pragma once

#include <algorithm>
#include <array>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <windows.h>

// 몬스터 "종류"가 갖는 기본 배치값입니다.
// 전투 슬롯 번호가 아니라 슬라임/고블린 같은 종류별 설정으로 사용합니다.
struct MonsterVisualProfile
{
    float x = 620.0f;
    float y = 190.0f;
    float width = 255.0f;
    float height = 255.0f;
    int layer = 20;
};

struct SceneConfig
{
    int outputPixelWidth = 360;
    int characterHeightScaleValue = 500;
    int contrastValue = 630;
    int startScreenInkDensity = 0;
    int startScreenContrast = 850;
    int mainMenuOutputPixelWidth = 1000;
    int mainMenuCharacterHeightScale = 500;
    int mainMenuContrast = 350;
    bool useOrderedDithering = true;
    bool useAnsiColor = true;
    int colorMode = 0;
    int framesPerSecond = 30;
    int sceneWidth = 960;
    int sceneHeight = 540;
    int battleTransitionMilliseconds = 3000;
    int nextBattleTransitionMilliseconds = 3000;
    int bossDoorStageHoldMilliseconds = 420;
    int bossDoorSweepMilliseconds = 520;
    int bossBlackoutMilliseconds = 520;
    int bossDragonRevealMilliseconds = 920;

    std::wstring heroImagePath = L"Resources\\Images\\Characters\\warrior_back_v2.png";
    std::wstring hero2ImagePath = L"Resources\\Images\\Characters\\mage_back_v2.png";
    std::wstring tankImagePath = L"Resources\\Images\\Characters\\tank_back_guard_selected.png";
    std::wstring monsterImagePath = L"3561912736885df2.png";
    std::wstring warriorWeaponImagePath = L"Resources\\Images\\Weapons\\warrior_greatsword.png";
    std::wstring mageWeaponImagePath = L"Resources\\Images\\Weapons\\mage_staff.png";
    std::wstring tankWeaponImagePath = L"Resources\\Images\\Weapons\\tank_monarch_shield_back_upright.png";
    // 공격과 회복 연출은 모든 아군/적이 공용으로 사용합니다.
    std::wstring hitEffect30ImagePath = L"Resources\\Images\\Effects\\hit_slash_30.png";
    std::wstring hitEffect45ImagePath = L"Resources\\Images\\Effects\\hit_slash_45.png";
    std::wstring hitEffect55ImagePath = L"Resources\\Images\\Effects\\hit_slash_55.png";
    std::wstring heroSlash30ImagePath = L"Resources\\Images\\Effects\\hero_slash_30.png";
    std::wstring heroSlash45ImagePath = L"Resources\\Images\\Effects\\hero_slash_45.png";
    std::wstring heroSlash55ImagePath = L"Resources\\Images\\Effects\\hero_slash_55.png";
    std::wstring healEffectImagePath = L"Resources\\Images\\Effects\\heal_vertical_green.png";
    std::wstring powerBuffEffectImagePath = L"Resources\\Images\\Effects\\power_buff_golden.png";
    std::wstring battleTransitionImagePath = L"Resources\\Images\\Battle\\battle_transition_forest_path.png";
    std::wstring battleReturnTransitionImagePath = L"Resources\\Images\\Battle\\battle_transition_forest_path_return.png";
    std::wstring nextBattleTransitionImagePath = L"Resources\\Images\\Battle\\battle_next_encounter_moonlit_path.png";
    std::wstring battleBackgroundImagePath = L"Resources\\Images\\Battle\\battle_background_moonlit_clearing.png";
    std::wstring bossDoorStage1ImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_01_closed.png";
    std::wstring bossDoorStage2ImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_02_crack.png";
    std::wstring bossDoorStage3ImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_03_glow.png";
    std::wstring bossDoorStage4ImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_04_opening.png";
    std::wstring bossDoorStage5ImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_05_blinding.png";
    std::wstring bossDragonRevealImagePath = L"Resources\\Images\\Battle\\Boss\\boss_door_stage_06_dragon_reveal.png";
    float heroX = 100.0f, heroY = 250.0f, heroWidth = 310.0f, heroHeight = 215.0f;
    int heroLayer = 20;
    float hero2X = 420.0f, hero2Y = 260.0f, hero2Width = 190.0f, hero2Height = 210.0f;
    int hero2Layer = 20;
    float tankX = 270.0f, tankY = 200.0f, tankWidth = 280.0f, tankHeight = 300.0f;
    int tankLayer = 25;
    float tankShieldX = 435.0f, tankShieldY = 220.0f, tankShieldWidth = 150.0f, tankShieldHeight = 255.0f;
    int tankShieldLayer = 35;
    // 전사와 마법사 무기도 본체와 독립적으로 배치합니다. 공격할 때만 이 위치에서 움직입니다.
    float warriorWeaponX = 225.0f, warriorWeaponY = 310.0f, warriorWeaponWidth = 160.0f, warriorWeaponHeight = 160.0f;
    int warriorWeaponLayer = 30;
    float mageWeaponX = 610.0f, mageWeaponY = 305.0f, mageWeaponWidth = 135.0f, mageWeaponHeight = 175.0f;
    int mageWeaponLayer = 30;
    float monsterX = 620.0f, monsterY = 190.0f, monsterWidth = 255.0f, monsterHeight = 255.0f;
    int monsterLayer = 20;
    // Fixed battle slots 1-4, assigned in actual monster spawn order.
    float monster2X = 790.0f, monster2Y = 100.0f, monster2Width = 150.0f, monster2Height = 185.0f;
    int monster2Layer = 20;
    float monster3X = 620.0f, monster3Y = 285.0f, monster3Width = 150.0f, monster3Height = 185.0f;
    int monster3Layer = 20;
    float monster4X = 790.0f, monster4Y = 285.0f, monster4Width = 150.0f, monster4Height = 185.0f;
    int monster4Layer = 20;
    // 순서: 슬라임, 고블린, 스켈레톤, 좀비, 코볼트, 골렘, 방황하는 갑옷, 드라큘라, 레드 드래곤
    std::array<MonsterVisualProfile, 9> monsterProfiles{};
    float weaponX = 370.0f, weaponY = 365.0f;
    int weaponLayer = 30;
    float hitEffectX = 735.0f, hitEffectY = 305.0f;
    int hitEffectLayer = 40;
    float hitEffectWidth = 210.0f, hitEffectHeight = 210.0f;
    float healEffectWidth = 155.0f, healEffectHeight = 205.0f;
    // 회복/공격력 버프 이펙트는 캐릭터와 별도로 배치 모드에서 위치와 크기를 조절합니다.
    float healEffectOffsetX = 0.0f, healEffectOffsetY = 0.0f;
    float powerBuffEffectWidth = 155.0f, powerBuffEffectHeight = 205.0f;
    float powerBuffEffectOffsetX = 0.0f, powerBuffEffectOffsetY = 0.0f;
    float heroBreatheHorizontalRange = 0.0f, heroBreatheVerticalRange = 7.0f;
    float monsterBreatheHorizontalRange = 3.0f, monsterBreatheVerticalRange = 6.0f;
    // 전체 숨쉬기 연출의 공통 강도와 속도입니다. 낮을수록 덜 움직이고 더 느립니다.
    float breatheMotionScale = 0.35f;
    float breatheSpeedScale = 0.45f;
    float heroAttackAdvanceRange = 85.0f, monsterHitShakeRange = 22.0f;
};

inline std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) return L"";
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

inline std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty()) return "";
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
    return result;
}

inline std::wstring TrimConfigText(std::wstring text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](wchar_t value) { return std::iswspace(value) != 0; });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](wchar_t value) { return std::iswspace(value) != 0; }).base();
    return first >= last ? L"" : std::wstring(first, last);
}

inline bool ReadConfigBool(const std::wstring& value)
{
    return value == L"1" || value == L"true" || value == L"TRUE" || value == L"on";
}

// Markdown 안의 jsonc 코드 블록 한 줄, 예: "hero_x": 100,
inline bool TryParseJsoncConfigLine(const std::wstring& line, std::wstring& key, std::wstring& value)
{
    if (line.size() < 4 || line.front() != L'"') return false;

    const size_t closingQuote = line.find(L'"', 1);
    if (closingQuote == std::wstring::npos) return false;
    const size_t colon = line.find(L':', closingQuote + 1);
    if (colon == std::wstring::npos) return false;

    key = line.substr(1, closingQuote - 1);
    value = TrimConfigText(line.substr(colon + 1));
    // "값, // 설명" 형태의 JSONC 한 줄 주석은 값에서 제외합니다.
    const size_t lineComment = value.find(L"//");
    if (lineComment != std::wstring::npos)
        value = TrimConfigText(value.substr(0, lineComment));
    if (!value.empty() && value.back() == L',') value.pop_back();
    value = TrimConfigText(value);
    if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
        value = value.substr(1, value.size() - 2);
    return !key.empty() && !value.empty();
}

inline const wchar_t* DescribeSceneConfigKey(const wchar_t* key)
{
    const std::wstring name(key);
    if (name == L"output_pixel_width") return L"아스키 출력 가로 해상도 (0~1000, 높을수록 선명하지만 느려질 수 있음)";
    if (name == L"character_height_scale") return L"글자 세로 비율 (500은 기본 비율)";
    if (name == L"contrast") return L"명암 대비 (500은 원본 대비, 높을수록 차이가 강해짐)";
    if (name == L"start_screen_ink_density") return L"시작 화면 점 농도 (0~1000)";
    if (name == L"start_screen_contrast") return L"시작 화면 대비 (0~1000)";
    if (name == L"main_menu_output_pixel_width") return L"메인 메뉴 아스키 출력 가로 해상도 (0~1000)";
    if (name == L"main_menu_character_height_scale") return L"메인 메뉴 글자 세로 비율 (500은 기본 비율)";
    if (name == L"main_menu_contrast") return L"메인 메뉴 대비 (0~1000)";
    if (name == L"dithering") return L"중간 명암을 점 무늬로 표현할지 여부";
    if (name == L"ansi_color") return L"true면 컬러, false면 흑백으로 출력";
    if (name == L"color_mode") return L"색상 모드 예약값 (현재는 기본값 0 사용)";
    if (name == L"frames_per_second") return L"초당 화면 갱신 횟수";
    if (name == L"scene_width") return L"장면 가상 캔버스 가로 크기";
    if (name == L"scene_height") return L"장면 가상 캔버스 세로 크기";
    if (name == L"hero_image") return L"첫 번째 영웅 이미지 파일명";
    if (name == L"hero2_image") return L"두 번째 영웅 이미지 파일명 (비우면 표시하지 않음)";
    if (name == L"monster_image") return L"몬스터 이미지 파일명";
    if (name == L"hero_x" || name == L"hero2_x" || name == L"monster_x") return L"왼쪽 기준 가로 위치";
    if (name == L"hero_y" || name == L"hero2_y" || name == L"monster_y") return L"위쪽 기준 세로 위치";
    if (name == L"hero_width" || name == L"hero2_width" || name == L"monster_width") return L"이미지 배치 가로 크기";
    if (name == L"hero_height" || name == L"hero2_height" || name == L"monster_height") return L"이미지 배치 세로 크기";
    if (name == L"hero_layer" || name == L"hero2_layer" || name == L"monster_layer") return L"겹칠 때 앞에 보일 순서 (값이 클수록 앞)";
    if (name == L"weapon_x" || name == L"weapon_y") return L"무기 기본 위치";
    if (name == L"weapon_layer") return L"무기가 보일 순서";
    if (name == L"hit_effect_x" || name == L"hit_effect_y") return L"피격 이펙트 기본 위치";
    if (name == L"hit_effect_layer") return L"피격 이펙트가 보일 순서";
    if (name == L"hero_breathe_horizontal_range") return L"영웅 숨쉬기 가로 이동 범위";
    if (name == L"hero_breathe_vertical_range") return L"영웅 숨쉬기 세로 이동 범위";
    if (name == L"monster_breathe_horizontal_range") return L"몬스터 숨쉬기 가로 이동 범위";
    if (name == L"monster_breathe_vertical_range") return L"몬스터 숨쉬기 세로 이동 범위";
    if (name == L"hero_attack_advance_range") return L"공격 시 영웅이 앞으로 이동하는 거리";
    if (name == L"monster_hit_shake_range") return L"피격 시 몬스터가 좌우로 흔들리는 거리";
    return L"설정값";
}

inline void ApplySceneConfigValue(SceneConfig& c, const std::wstring& key, const std::wstring& value)
{
    try
    {
        // monster_<종류>_* 값은 슬롯 번호와 무관한 몬스터 종류별 배치값입니다.
        static constexpr std::array<std::wstring_view, 9> kMonsterProfileKeys = {
            L"slime", L"goblin", L"skeleton", L"zombie", L"kobold",
            L"golem", L"wandering_armor", L"dracula", L"red_dragon" };
        for (size_t index = 0; index < kMonsterProfileKeys.size(); ++index)
        {
            const std::wstring prefix = L"monster_" + std::wstring(kMonsterProfileKeys[index]) + L"_";
            MonsterVisualProfile& profile = c.monsterProfiles[index];
            if (key == prefix + L"x") { profile.x = std::stof(value); return; }
            if (key == prefix + L"y") { profile.y = std::stof(value); return; }
            if (key == prefix + L"width") { profile.width = std::stof(value); return; }
            if (key == prefix + L"height") { profile.height = std::stof(value); return; }
            if (key == prefix + L"layer") { profile.layer = std::stoi(value); return; }
        }
        if (key == L"output_pixel_width") c.outputPixelWidth = std::stoi(value);
        else if (key == L"character_height_scale") c.characterHeightScaleValue = std::stoi(value);
        else if (key == L"contrast") c.contrastValue = std::stoi(value);
        else if (key == L"start_screen_ink_density") c.startScreenInkDensity = std::stoi(value);
        else if (key == L"start_screen_contrast") c.startScreenContrast = std::stoi(value);
        else if (key == L"main_menu_output_pixel_width") c.mainMenuOutputPixelWidth = std::stoi(value);
        else if (key == L"main_menu_character_height_scale") c.mainMenuCharacterHeightScale = std::stoi(value);
        else if (key == L"main_menu_contrast") c.mainMenuContrast = std::stoi(value);
        else if (key == L"dithering") c.useOrderedDithering = ReadConfigBool(value);
        else if (key == L"ansi_color") c.useAnsiColor = ReadConfigBool(value);
        else if (key == L"color_mode") c.colorMode = std::stoi(value);
        else if (key == L"frames_per_second") c.framesPerSecond = std::stoi(value);
        else if (key == L"scene_width") c.sceneWidth = std::stoi(value);
        else if (key == L"scene_height") c.sceneHeight = std::stoi(value);
        else if (key == L"battle_transition_milliseconds") c.battleTransitionMilliseconds = std::stoi(value);
        else if (key == L"next_battle_transition_milliseconds") c.nextBattleTransitionMilliseconds = std::stoi(value);
        else if (key == L"boss_door_stage_hold_milliseconds") c.bossDoorStageHoldMilliseconds = std::stoi(value);
        else if (key == L"boss_door_sweep_milliseconds") c.bossDoorSweepMilliseconds = std::stoi(value);
        else if (key == L"boss_blackout_milliseconds") c.bossBlackoutMilliseconds = std::stoi(value);
        else if (key == L"boss_dragon_reveal_milliseconds") c.bossDragonRevealMilliseconds = std::stoi(value);
        // 이전 hero/hero2 이름도 읽되, 설정 파일에는 직업 이름으로 표시합니다.
        else if (key == L"warrior_image" || key == L"hero_image") c.heroImagePath = value;
        else if (key == L"mage_image" || key == L"hero2_image") c.hero2ImagePath = value;
        else if (key == L"tank_image") c.tankImagePath = value;
        else if (key == L"monster_image") c.monsterImagePath = value;
        else if (key == L"warrior_weapon_image") c.warriorWeaponImagePath = value;
        else if (key == L"mage_weapon_image") c.mageWeaponImagePath = value;
        else if (key == L"tank_weapon_image") c.tankWeaponImagePath = value;
        else if (key == L"hit_effect_30_image") c.hitEffect30ImagePath = value;
        else if (key == L"hit_effect_45_image") c.hitEffect45ImagePath = value;
        else if (key == L"hit_effect_55_image") c.hitEffect55ImagePath = value;
        else if (key == L"hero_slash_30_image") c.heroSlash30ImagePath = value;
        else if (key == L"hero_slash_45_image") c.heroSlash45ImagePath = value;
        else if (key == L"hero_slash_55_image") c.heroSlash55ImagePath = value;
        else if (key == L"heal_effect_image") c.healEffectImagePath = value;
        else if (key == L"power_buff_effect_image") c.powerBuffEffectImagePath = value;
        else if (key == L"battle_transition_image") c.battleTransitionImagePath = value;
        else if (key == L"battle_return_transition_image") c.battleReturnTransitionImagePath = value;
        else if (key == L"next_battle_transition_image") c.nextBattleTransitionImagePath = value;
        else if (key == L"battle_background_image") c.battleBackgroundImagePath = value;
        else if (key == L"boss_door_stage_1_image") c.bossDoorStage1ImagePath = value;
        else if (key == L"boss_door_stage_2_image") c.bossDoorStage2ImagePath = value;
        else if (key == L"boss_door_stage_3_image") c.bossDoorStage3ImagePath = value;
        else if (key == L"boss_door_stage_4_image") c.bossDoorStage4ImagePath = value;
        else if (key == L"boss_door_stage_5_image") c.bossDoorStage5ImagePath = value;
        else if (key == L"boss_dragon_reveal_image") c.bossDragonRevealImagePath = value;
        else if (key == L"hero_x") c.heroX = std::stof(value); else if (key == L"hero_y") c.heroY = std::stof(value);
        else if (key == L"hero_width") c.heroWidth = std::stof(value); else if (key == L"hero_height") c.heroHeight = std::stof(value);
        else if (key == L"hero_layer") c.heroLayer = std::stoi(value);
        else if (key == L"hero2_x") c.hero2X = std::stof(value); else if (key == L"hero2_y") c.hero2Y = std::stof(value);
        else if (key == L"hero2_width") c.hero2Width = std::stof(value); else if (key == L"hero2_height") c.hero2Height = std::stof(value);
        else if (key == L"hero2_layer") c.hero2Layer = std::stoi(value);
        else if (key == L"tank_x") c.tankX = std::stof(value); else if (key == L"tank_y") c.tankY = std::stof(value);
        else if (key == L"tank_width") c.tankWidth = std::stof(value); else if (key == L"tank_height") c.tankHeight = std::stof(value);
        else if (key == L"tank_layer") c.tankLayer = std::stoi(value);
        else if (key == L"tank_shield_x") c.tankShieldX = std::stof(value); else if (key == L"tank_shield_y") c.tankShieldY = std::stof(value);
        else if (key == L"tank_shield_width") c.tankShieldWidth = std::stof(value); else if (key == L"tank_shield_height") c.tankShieldHeight = std::stof(value);
        else if (key == L"tank_shield_layer") c.tankShieldLayer = std::stoi(value);
        else if (key == L"warrior_weapon_x") c.warriorWeaponX = std::stof(value); else if (key == L"warrior_weapon_y") c.warriorWeaponY = std::stof(value);
        else if (key == L"warrior_weapon_width") c.warriorWeaponWidth = std::stof(value); else if (key == L"warrior_weapon_height") c.warriorWeaponHeight = std::stof(value);
        else if (key == L"warrior_weapon_layer") c.warriorWeaponLayer = std::stoi(value);
        else if (key == L"mage_weapon_x") c.mageWeaponX = std::stof(value); else if (key == L"mage_weapon_y") c.mageWeaponY = std::stof(value);
        else if (key == L"mage_weapon_width") c.mageWeaponWidth = std::stof(value); else if (key == L"mage_weapon_height") c.mageWeaponHeight = std::stof(value);
        else if (key == L"mage_weapon_layer") c.mageWeaponLayer = std::stoi(value);
        else if (key == L"monster_x") c.monsterX = std::stof(value); else if (key == L"monster_y") c.monsterY = std::stof(value);
        else if (key == L"monster_width") c.monsterWidth = std::stof(value); else if (key == L"monster_height") c.monsterHeight = std::stof(value);
        else if (key == L"monster_layer") c.monsterLayer = std::stoi(value);
        else if (key == L"monster2_x") c.monster2X = std::stof(value); else if (key == L"monster2_y") c.monster2Y = std::stof(value);
        else if (key == L"monster2_width") c.monster2Width = std::stof(value); else if (key == L"monster2_height") c.monster2Height = std::stof(value);
        else if (key == L"monster2_layer") c.monster2Layer = std::stoi(value);
        else if (key == L"monster3_x") c.monster3X = std::stof(value); else if (key == L"monster3_y") c.monster3Y = std::stof(value);
        else if (key == L"monster3_width") c.monster3Width = std::stof(value); else if (key == L"monster3_height") c.monster3Height = std::stof(value);
        else if (key == L"monster3_layer") c.monster3Layer = std::stoi(value);
        else if (key == L"monster4_x") c.monster4X = std::stof(value); else if (key == L"monster4_y") c.monster4Y = std::stof(value);
        else if (key == L"monster4_width") c.monster4Width = std::stof(value); else if (key == L"monster4_height") c.monster4Height = std::stof(value);
        else if (key == L"monster4_layer") c.monster4Layer = std::stoi(value);
        else if (key == L"weapon_x") c.weaponX = std::stof(value); else if (key == L"weapon_y") c.weaponY = std::stof(value);
        else if (key == L"weapon_layer") c.weaponLayer = std::stoi(value);
        else if (key == L"hit_effect_x") c.hitEffectX = std::stof(value); else if (key == L"hit_effect_y") c.hitEffectY = std::stof(value);
        else if (key == L"hit_effect_layer") c.hitEffectLayer = std::stoi(value);
        else if (key == L"hit_effect_width") c.hitEffectWidth = std::stof(value); else if (key == L"hit_effect_height") c.hitEffectHeight = std::stof(value);
        else if (key == L"heal_effect_width") c.healEffectWidth = std::stof(value); else if (key == L"heal_effect_height") c.healEffectHeight = std::stof(value);
        else if (key == L"heal_effect_offset_x") c.healEffectOffsetX = std::stof(value); else if (key == L"heal_effect_offset_y") c.healEffectOffsetY = std::stof(value);
        else if (key == L"power_buff_effect_width") c.powerBuffEffectWidth = std::stof(value); else if (key == L"power_buff_effect_height") c.powerBuffEffectHeight = std::stof(value);
        else if (key == L"power_buff_effect_offset_x") c.powerBuffEffectOffsetX = std::stof(value); else if (key == L"power_buff_effect_offset_y") c.powerBuffEffectOffsetY = std::stof(value);
        else if (key == L"hero_breathe_horizontal_range") c.heroBreatheHorizontalRange = std::stof(value);
        else if (key == L"hero_breathe_vertical_range") c.heroBreatheVerticalRange = std::stof(value);
        else if (key == L"monster_breathe_horizontal_range") c.monsterBreatheHorizontalRange = std::stof(value);
        else if (key == L"monster_breathe_vertical_range") c.monsterBreatheVerticalRange = std::stof(value);
        else if (key == L"breathe_motion_scale") c.breatheMotionScale = std::stof(value);
        else if (key == L"breathe_speed_scale") c.breatheSpeedScale = std::stof(value);
        else if (key == L"hero_attack_advance_range") c.heroAttackAdvanceRange = std::stof(value);
        else if (key == L"monster_hit_shake_range") c.monsterHitShakeRange = std::stof(value);
    }
    catch (const std::exception&) {}
}

// JSONC의 주석과 중괄호는 설정값으로 처리하지 않습니다.
inline SceneConfig LoadSceneConfig(const std::wstring& fileName = L"SceneConfig.jsonc")
{
    SceneConfig config;
    std::ifstream file(WideToUtf8(fileName), std::ios::binary);
    std::string raw;
    bool hasMonsterTypeProfiles = false;
    while (std::getline(file, raw))
    {
        const std::wstring line = TrimConfigText(Utf8ToWide(raw));
        if (line.find(L"monster_slime_") != std::wstring::npos)
            hasMonsterTypeProfiles = true;
        if (line.empty() || line[0] == L'#') continue;
        const size_t equals = line.find(L'=');
        if (equals != std::wstring::npos)
        {
            // 이전 txt/ini 형식도 계속 읽을 수 있게 남겨 둡니다.
            ApplySceneConfigValue(config, TrimConfigText(line.substr(0, equals)), TrimConfigText(line.substr(equals + 1)));
            continue;
        }

        std::wstring key;
        std::wstring value;
        if (TryParseJsoncConfigLine(line, key, value))
            ApplySceneConfigValue(config, key, value);
    }
    // 기존 슬롯 형식 파일을 처음 열었을 때 현재 1번 몬스터의 모습을 유지하도록 마이그레이션합니다.
    if (!hasMonsterTypeProfiles)
    {
        const MonsterVisualProfile legacyProfile{ config.monsterX, config.monsterY, config.monsterWidth, config.monsterHeight, config.monsterLayer };
        config.monsterProfiles.fill(legacyProfile);
    }
    return config;
}

inline void SaveSceneConfig(const SceneConfig& c, const std::wstring& fileName = L"SceneConfig.jsonc")
{
    std::wostringstream text;
    const auto section = [&text](const wchar_t* title) { text << L"\n  // ===== " << title << L" =====\n"; };
    const auto number = [&text](const wchar_t* key, const auto& value)
    {
        // 설정값 설명의 인코딩이 깨져 여러 키가 한 줄로 합쳐지는 일을 막기 위해,
        // 저장 파일은 한 키당 한 줄의 순수 JSONC 형태로 기록합니다.
        text << L"  \"" << key << L"\": " << value << L",\n";
    };
    const auto string = [&text](const wchar_t* key, const std::wstring& value)
    {
        text << L"  \"" << key << L"\": \"" << value << L"\",\n";
    };

    text << L"// ASCII 전투 장면 설정 파일\n"
         << L"// 프로그램을 종료한 뒤 값을 수정해 주세요.\n"
         << L"// 실행 중 슬라이더를 조절한 뒤 S를 누르거나 배치 모드에서 Enter를 누르면 저장됩니다.\n"
         << L"// 큰따옴표 안의 이름은 바꾸지 말고, 오른쪽 값 또는 파일명만 수정해 주세요.\n{\n";

    section(L"출력 설정");
    number(L"output_pixel_width", c.outputPixelWidth); number(L"character_height_scale", c.characterHeightScaleValue);
    number(L"contrast", c.contrastValue); number(L"dithering", c.useOrderedDithering ? L"true" : L"false");
    number(L"start_screen_ink_density", c.startScreenInkDensity); number(L"start_screen_contrast", c.startScreenContrast);
    number(L"main_menu_output_pixel_width", c.mainMenuOutputPixelWidth); number(L"main_menu_character_height_scale", c.mainMenuCharacterHeightScale); number(L"main_menu_contrast", c.mainMenuContrast);
    number(L"ansi_color", c.useAnsiColor ? L"true" : L"false"); number(L"color_mode", c.colorMode);
    number(L"frames_per_second", c.framesPerSecond); number(L"scene_width", c.sceneWidth); number(L"scene_height", c.sceneHeight);
    number(L"battle_transition_milliseconds", c.battleTransitionMilliseconds); number(L"next_battle_transition_milliseconds", c.nextBattleTransitionMilliseconds);
    number(L"boss_door_stage_hold_milliseconds", c.bossDoorStageHoldMilliseconds); number(L"boss_door_sweep_milliseconds", c.bossDoorSweepMilliseconds);
    number(L"boss_blackout_milliseconds", c.bossBlackoutMilliseconds); number(L"boss_dragon_reveal_milliseconds", c.bossDragonRevealMilliseconds);

    section(L"이미지 파일");
    string(L"warrior_image", c.heroImagePath); string(L"warrior_weapon_image", c.warriorWeaponImagePath);
    string(L"tank_image", c.tankImagePath); string(L"tank_weapon_image", c.tankWeaponImagePath);
    string(L"mage_image", c.hero2ImagePath); string(L"mage_weapon_image", c.mageWeaponImagePath);
    string(L"monster_image", c.monsterImagePath);
    string(L"hit_effect_30_image", c.hitEffect30ImagePath); string(L"hit_effect_45_image", c.hitEffect45ImagePath); string(L"hit_effect_55_image", c.hitEffect55ImagePath);
    string(L"hero_slash_30_image", c.heroSlash30ImagePath); string(L"hero_slash_45_image", c.heroSlash45ImagePath); string(L"hero_slash_55_image", c.heroSlash55ImagePath); string(L"heal_effect_image", c.healEffectImagePath); string(L"power_buff_effect_image", c.powerBuffEffectImagePath);
    string(L"battle_transition_image", c.battleTransitionImagePath); string(L"battle_return_transition_image", c.battleReturnTransitionImagePath);
    string(L"next_battle_transition_image", c.nextBattleTransitionImagePath); string(L"battle_background_image", c.battleBackgroundImagePath);
    section(L"보스 문 개방 연출");
    string(L"boss_door_stage_1_image", c.bossDoorStage1ImagePath); string(L"boss_door_stage_2_image", c.bossDoorStage2ImagePath);
    string(L"boss_door_stage_3_image", c.bossDoorStage3ImagePath); string(L"boss_door_stage_4_image", c.bossDoorStage4ImagePath);
    string(L"boss_door_stage_5_image", c.bossDoorStage5ImagePath); string(L"boss_dragon_reveal_image", c.bossDragonRevealImagePath);

    section(L"영웅 배치");
    number(L"hero_x", c.heroX); number(L"hero_y", c.heroY); number(L"hero_width", c.heroWidth); number(L"hero_height", c.heroHeight); number(L"hero_layer", c.heroLayer);
    number(L"hero2_x", c.hero2X); number(L"hero2_y", c.hero2Y); number(L"hero2_width", c.hero2Width); number(L"hero2_height", c.hero2Height); number(L"hero2_layer", c.hero2Layer);
    number(L"tank_x", c.tankX); number(L"tank_y", c.tankY); number(L"tank_width", c.tankWidth); number(L"tank_height", c.tankHeight); number(L"tank_layer", c.tankLayer);
    number(L"tank_shield_x", c.tankShieldX); number(L"tank_shield_y", c.tankShieldY); number(L"tank_shield_width", c.tankShieldWidth); number(L"tank_shield_height", c.tankShieldHeight); number(L"tank_shield_layer", c.tankShieldLayer);
    section(L"직업별 무기 배치");
    number(L"warrior_weapon_x", c.warriorWeaponX); number(L"warrior_weapon_y", c.warriorWeaponY); number(L"warrior_weapon_width", c.warriorWeaponWidth); number(L"warrior_weapon_height", c.warriorWeaponHeight); number(L"warrior_weapon_layer", c.warriorWeaponLayer);
    number(L"mage_weapon_x", c.mageWeaponX); number(L"mage_weapon_y", c.mageWeaponY); number(L"mage_weapon_width", c.mageWeaponWidth); number(L"mage_weapon_height", c.mageWeaponHeight); number(L"mage_weapon_layer", c.mageWeaponLayer);

    section(L"몬스터 종류별 배치");
    // 여기의 값은 1~4번 자리값이 아니라, 몬스터 종류 자체의 기본 위치/크기입니다.
    // 같은 종류가 동시에 여러 마리면, 첫 번째 개체를 기준으로 작은 간격만 자동 적용됩니다.
    static constexpr std::array<std::wstring_view, 9> kMonsterProfileKeys = {
        L"slime", L"goblin", L"skeleton", L"zombie", L"kobold",
        L"golem", L"wandering_armor", L"dracula", L"red_dragon" };
    for (size_t index = 0; index < kMonsterProfileKeys.size(); ++index)
    {
        const std::wstring prefix = L"monster_" + std::wstring(kMonsterProfileKeys[index]) + L"_";
        const MonsterVisualProfile& profile = c.monsterProfiles[index];
        number((prefix + L"x").c_str(), profile.x); number((prefix + L"y").c_str(), profile.y);
        number((prefix + L"width").c_str(), profile.width); number((prefix + L"height").c_str(), profile.height);
        number((prefix + L"layer").c_str(), profile.layer);
    }

    section(L"이전 슬롯 배치값 (호환용 - 새 전투 화면에서는 사용하지 않음)");
    number(L"monster_x", c.monsterX); number(L"monster_y", c.monsterY); number(L"monster_width", c.monsterWidth); number(L"monster_height", c.monsterHeight); number(L"monster_layer", c.monsterLayer);
    number(L"monster2_x", c.monster2X); number(L"monster2_y", c.monster2Y); number(L"monster2_width", c.monster2Width); number(L"monster2_height", c.monster2Height); number(L"monster2_layer", c.monster2Layer);
    number(L"monster3_x", c.monster3X); number(L"monster3_y", c.monster3Y); number(L"monster3_width", c.monster3Width); number(L"monster3_height", c.monster3Height); number(L"monster3_layer", c.monster3Layer);
    number(L"monster4_x", c.monster4X); number(L"monster4_y", c.monster4Y); number(L"monster4_width", c.monster4Width); number(L"monster4_height", c.monster4Height); number(L"monster4_layer", c.monster4Layer);
    number(L"weapon_x", c.weaponX); number(L"weapon_y", c.weaponY); number(L"weapon_layer", c.weaponLayer);
    number(L"hit_effect_x", c.hitEffectX); number(L"hit_effect_y", c.hitEffectY); number(L"hit_effect_layer", c.hitEffectLayer);
    number(L"hit_effect_width", c.hitEffectWidth); number(L"hit_effect_height", c.hitEffectHeight);
    number(L"heal_effect_width", c.healEffectWidth); number(L"heal_effect_height", c.healEffectHeight);
    number(L"heal_effect_offset_x", c.healEffectOffsetX); number(L"heal_effect_offset_y", c.healEffectOffsetY);
    number(L"power_buff_effect_width", c.powerBuffEffectWidth); number(L"power_buff_effect_height", c.powerBuffEffectHeight);
    number(L"power_buff_effect_offset_x", c.powerBuffEffectOffsetX); number(L"power_buff_effect_offset_y", c.powerBuffEffectOffsetY);

    section(L"움직임 범위");
    number(L"hero_breathe_horizontal_range", c.heroBreatheHorizontalRange); number(L"hero_breathe_vertical_range", c.heroBreatheVerticalRange);
    number(L"monster_breathe_horizontal_range", c.monsterBreatheHorizontalRange); number(L"monster_breathe_vertical_range", c.monsterBreatheVerticalRange);
    number(L"breathe_motion_scale", c.breatheMotionScale); number(L"breathe_speed_scale", c.breatheSpeedScale);
    number(L"hero_attack_advance_range", c.heroAttackAdvanceRange); number(L"monster_hit_shake_range", c.monsterHitShakeRange);
    text << L"}\n";
    std::ofstream file(WideToUtf8(fileName), std::ios::binary | std::ios::trunc);
    if (file) file << WideToUtf8(text.str());
}
