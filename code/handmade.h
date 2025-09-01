#if !defined(HANDMADE_H)

/* Services that teh platform layer provides to the game*/
//file io
//send to network
// device input
// timing


/* servcies the game provides to the platform layer*/

// needs four things: controller/keyboard input, bitmap buffer to use, sound buffer to use, timing

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    // int BytesPerPixel; (always 4 bytes 32 bits memoryt order BB GG RR XX)
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset, int RedOffset, game_sound_output_buffer *SoundBuffer, int toneHz);

#define HANDMADE_H
#endif