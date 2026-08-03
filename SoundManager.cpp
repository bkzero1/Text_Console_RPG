#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "SoundManager.h"

void SoundManager ::StartSound(const std::string filePath)
{
    std::wstring wFilePath(filePath.begin(), filePath.end());

    ::PlaySound(wFilePath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void SoundManager::StopSound()
{
  
    ::PlaySound(NULL, 0, 0);
}