#include "handmade.h"

//
internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
    }
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset, int RedOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory; // cast the void pointer BitmapMemory to unsigned 8 bit int
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row; // pointer to first RGBA 32bit pixel of Row: 0xAARRGGBB
        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8 A = 0x00;                 // Alpha
            uint8 B = (uint8)(X + XOffset); // Blue
            uint8 G = (uint8)(Y + YOffset); // Green
            uint8 R = (uint8)(RedOffset);   // red

            uint32 BGRA = (uint32)((B) | (G << 8) | (R << 16) | (A << 24)); // Comine 8 bit components by bitwise shift and bitwise OR
            *Pixel = BGRA;
            ++Pixel;
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset, int RedOffset, game_sound_output_buffer *SoundBuffer, int toneHz)
{
    //TODO: alllow sample offset
    GameOutputSound(SoundBuffer, toneHz);
    RenderWeirdGradient(Buffer, XOffset, YOffset, RedOffset);
}