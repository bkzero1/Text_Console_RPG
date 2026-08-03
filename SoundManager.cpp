#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <iostream>
#include "SoundManager.h"

std::map<SoundStates, std::string> soundMap = {
    {SoundStates::INTRO,  "assets/intro.mp3"}, 
    {SoundStates::NORMAL_BATTLE,  "assets/INBATTLE.mp3"},
    {SoundStates::BOSS_BATTLE, "assets/BOSS.mp3"},
    {SoundStates::SHOP, "assets/charity-shop.wav"},
    {SoundStates::VILLAGE, "assets/village.mp3"},
    {SoundStates::ATTACK_01, "assets/attack-01.wav"},
    {SoundStates::ATTACK_02, "assets/attack-02.wav"},
    {SoundStates::ATTACK_03, "assets/attack-03.wav"},
};

bool SoundManager::Init()
{
    return ma_engine_init(NULL, &engine) == MA_SUCCESS;
}

void SoundManager:: Shutdown()
{
    StopBGM();
    ma_engine_uninit(&engine);
}

void SoundManager::PlayBGM(const std::string& path, bool loop)
{
    if (currentBgmPath == path && isBgmPlaying)
        return;  

    StopBGM();  

    if (ma_sound_init_from_file(&engine, path.c_str(),
                                MA_SOUND_FLAG_STREAM, NULL, NULL, &currentBgm) == MA_SUCCESS)
    {
        ma_sound_set_looping(&currentBgm, loop ? MA_TRUE : MA_FALSE);
        ma_sound_start(&currentBgm);
        currentBgmPath = path;
        isBgmPlaying = true;
    }
}

 void SoundManager :: StopBGM()
{
    if (isBgmPlaying)
    {
        ma_sound_stop(&currentBgm);
        ma_sound_uninit(&currentBgm);
        isBgmPlaying = false;
        currentBgmPath.clear();
    }
}

void SoundManager::PlaySFX(const std::string& path)
{
    ma_engine_play_sound(&engine, path.c_str(), NULL);
}
