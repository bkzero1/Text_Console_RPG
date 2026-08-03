// SoundManager.h
#pragma once
#include "miniaudio.h"
#include <string>
#include <map>

enum class SoundStates
{
    INTRO, // INTRO.mp3
    NORMAL_BATTLE, // 
    BOSS_BATTLE, // BOSS.mp3
    SHOP, // charity-shop.mp3
    VILLAGE,// village.mp3
    ATTACK_01, // attack-01.wav
    ATTACK_02, // attack-02.wav
    ATTACK_03 // attack-03.wav
};

extern std::map<SoundStates, std::string> soundMap;

class SoundManager
{
public:
    bool Init();
    void Shutdown();
    void PlayBGM(const std::string& path, bool loop = true);
    void StopBGM();
    void PlaySFX(const std::string& path);

   private:
    ma_engine engine;
    ma_sound currentBgm;
    std::string currentBgmPath;
    bool isBgmPlaying = false;
};