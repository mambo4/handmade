/*
Yet another try at audio
First https://youtu.be/qGC3xiliJW8
I wanted to try "play a wav file" not  "generate a sine wave"

GameDev.net posts said "ew, not DirectSound, use Xaudio2"

My first attempt at Xaudio2 had the issue that each wav looped continuously
Wav file data was parsed and fed to the MasterVoice with no provisoin for "play this once then stop"

so now I'm tryign to follow 
https://www.youtube.com/watch?v=38A6WmBvxHM&list=PLUJn3SlcLb962Lm7rrGJoYhYa1LJwWgFL&index=3&ab_channel=Cakez
to get an audio module i can actually use

*/

#define BIT(x) 1 << (x)

// #######################################################
//              sound constants
// #######################################################
static constexpr int MAX_CONCURRENT_SOUNDS = 16;
static constexpr int SOUNDS_BUFFER_SIZE = 128 * 1024 * 1024 ;// 128 MB in bits
static constexpr int MAX_SOUND_PATH_LENGTH = 256;

// #######################################################
//              sound structs
// #######################################################

    enum SoundOptionBits {
        SOUND_OPTION_FADE_OUT = BIT(0),
        SOUND_OPTION_FADE_IN = BIT(1),
        SOUND_OPTION_START = BIT(2),
        SOUND_OPTION_LOOP = BIT(3),
    };

typedef int SoundOptions;

struct Sound
{
    char file[MAX_SOUND_PATH_LENGTH];
    SoundOptions options;
    int size;
    char* data;
}

// #######################################################
//              sound globals
// #######################################################

// #######################################################
//              sound functions
// #######################################################

void play_sound(char* soundName, SoundOptions options=0)
{
    //assert soundName
    options= options ? options : SOUND_OPTION_START;

    if (!(options & SOUND_OPTION_START) &&
        !(options & SOUND_OPTION_FADE_IN) &&
        !(options & SOUND_OPTION_FADE_OUT))
        {
            options |= SOUND_OPTION_START;
        }

    Sound sound={};
    sound.options=options;
    sprintf_s(sound.file)
}