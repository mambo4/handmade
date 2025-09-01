# e

# audio  notes
https://youtu.be/uiW1D1Vc7IQ

Episodes 007,008,009 : casey intializes directSound and generates a square wave to it. DirectSound lead to compile errors for me, so I switched to Xaudio2 and tried ot get wav files to play. I was 50% successful - they play but with artifatcs, I am too noob to realize why exactly. This led me to pause all HMH work and just stop for 2 years. 

Now, I want to go back and do the Square wave thing, *with* Xaudio2 instead. For insanity's sake I guess. I need to take careful notes to understand precisely what Casey is doing, and careful notes about Xaudio2 to understand exactly how I cna recreate it. It will be frustrating and hard but that's learning I guess.



someone posted a n Xaudio2 implementation it here 
https://hero.handmade.network/forums/code-discussion/t/53-im_way_behind_but_here_comes_xaudio2
----00000000
## The Plan
- Watch each episode  007-009 
- section 1: write  what casey codes & why
  - note the time
  - copy the code 
  - write out what its doing.
  - clearly identify what is DiretcSound and what is not
- section 2: Xaudio2 equivalent
  - use same var names for NOT Directsound data
  - find the Xaudio2 equivalent data where possible
  - were not possible, explain why and posit other approaches.
----
# DAY 007: 
- https://youtu.be/qGC3xiliJW8 
    - DS: You need a window to initilaize DirectSound
    - XA: AI says Xaudio does not require a window, so we can intialize elsewhere?

### Casey code
```
internal void
Win32InitDSound(void)
{
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll")
    if(DsoundLibrary)
    {
        // get a direct sound object (magic macro shit to only import one fn form lib)
        direct_sound_create *DirectSoundCreate =(direct_sound_create *) 
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND *DirectSound; //pointer to DSound object
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0,&DirectSound,0))) //DSoundObj create
        {
          WAVEFORMATEX WaveFormat = {};
          // Set up WAVEFORMATEX for 44.1 kHz 16-bit stereo. 
          WaveFormat.wFormatTag = WAVE_FORMAT_PCM; 
          WaveFormat.nChannels = 2; 
          WaveFormat.nSamplesPerSec = 44100L; 
          WaveFormat.nAvgBytesPerSec = 176400L; 
          WaveFormat.nBlockAlign = 4; 
          WaveFormat.wBitsPerSample = 16; 
          WaveFormat.cbSize = 0;

          if SUCEEDED (DirectSound->SetCooperativeLevel(window, DSSL_PRIORITY)) //give me priority 4 audio
          {

              DSBUFFERDESC BufferDescription ={}; 
              BufferDescription.dwSize=sizeof(BufferDescription);
              BufferDescription.dwFlags=DSBCAPS_PRIMARYBUFFER;

              // create primary buffer (to set mode)
              // All Primary buffer stuff is just to set format
              LPDIRECTSOUNDBUFFER PrimaryBuffer;
              if  (SUCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer,0)))
              {
                if SUCCEEDED (PrimaryBuffer->SetFormat(&WaveFormat))
                {
                  //format is set!!
                }else{
                  //diagnostic fail SetFormat
                }
              }else{
                //diagnostic fail CreateSoundBuffer
              }
          }else{
            //diagnostic fail DirectSound->SetCooperativeLevel
          }

        //create secondary buffer 
        DSBUFFERDESC BufferDescription ={}; 
        BufferDescription.dwSize=sizeof(BufferDescription);
        BufferDescription.dwFlags=0;
        BufferDescription.dwBufferBytes = BufferSize;
        BufferDescription. lpwfxFormat =&WaveFormat;
        LPDIRECTSOUNDBUFFER SecondaryBuffer;
        if  (SUCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer,0)))
        {
          // start it playing
        }
    }else{
      //diagnostic failDirectSoundCreate
>>> https://youtu.be/qGC3xiliJW8?t=3634
    }
}
```
### what this does

- magic macro shit to import DirectSoundCreate()
- DirectSoundCreate() into memory at LPDIRECTSOUND ptr obj
- create WAVEFORMATEX obj to set sound format
- DirectSound->SetCooperativeLevel() to give sound priority to the game
- create DSBUFFERDESC obj to set buffer params    
- create LPDIRECTSOUNDBUFFER PrimaryBuffer ONLY to set format;
- create DSBUFFERDESC obj2 to set  2nd buffer params;
- create LPDIRECTSOUNDBUFFER SecondaryBuffer for atcual soudn use

all of these are DirectSound ojs/methods
no platform independent stuff (yet?)
### Xaudio Equivalents

- DirectSoundCreate() == XAudio2Create()
- create WAVEFORMATEX obj == WAVEFORMATEXTENSIBLE wfx
- DirectSound->SetCooperativeLevel() == NO XAUDIO NEED ?
- create DSBUFFERDESC == XAUDIO2_BUFFER 
- create LPDIRECTSOUNDBUFFER PrimaryBuffer  == XAudio2->CreateMasteringVoice()
- create DSBUFFERDESC  == XAUDIO2_BUFFER 
- create LPDIRECTSOUNDBUFFER SecondaryBuffer == a global a char[] array ?


# DAY 008: 
- https://youtu.be/uiW1D1Vc7IQ?t=539

- buffer gets made global, not a big deal in platform layer
- "I'm going to ask where the write pointer is and fill in some memory with a sound"
- ! you do not need to lock the buffer to write to it in xaudio2 
- use `int SquareWaveCounter` counter to track where in the square wave wave form we are
- `int SamplesPerSecond=41000`
- `int Hz=256`
- `int SquarewavePeriod=SamplesPerSecond/Hz`

### casey code
```
global uint32 RunningSampleIndex;
DWORD WritePointer;
DWORD BytesToWrite;

VOID *Region1; // ptr to global buffer?
DWORD Region1Size; // sizeof global buffer?
int16 *SampleOut=(int16*)Region1
DWORD RegionSampleCount =Region1Size/BytesPerSample //
for(DWORD SampleIndex=0; SampleIndex < Region1Size) // global audio buffer size?
{
  if(squareWaveCounter==0){
    squarewaveCounter=SquarewavePeriod; 
  }
  int16 SampleValue=(squarewaveCounter>(SquarewavePeriod/2)) ? 16000: -160000;
  *SampleOut++=SampleValue;//left
  *SampleOut++=SampleValue;//right
  --squarewaveCounter; // counts down to zero

}


```

### XAUDIO2_BUFFER (fka secondary buffer )

```BYTE *pDataBuffer = new BYTE[dwChunkSize];//BYTE=unsigned char[]
        XAUDIO2_BUFFER buffer; // create struct 
        buffer.AudioBytes = dwChunkSize;	  // size of the audio buffer in bytes
        buffer.pAudioData = pDataBuffer;	  // buffer containing audio data
        buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
        buffer.LoopCount =1;
```

-  Cannot acces the buffer via Xaudio2SourceVoice: only start/stop/get state / manipulate
  1. create XAUDIO2_BUFFER struct
  2. `buffer.pAudioData` ***is a pointer to a `char[]` array that is the actual audio data buffer !!!!***


- get cursor postion:   
  - create an  XAUDIO2_VOICE_STATE struct
  - Xaudio2SourceVoice::GetState()
  
```
XAUDIO2_VOICE_STATE voiceState;
sourceVoice->GetState(&voiceState)
voiceState.SamplesPlayed //playback cursor positon?  in samples,since START

SampleWriteIndex =voiceState.SamplesPlayed*bytesPerSample%BufferSize; //I think this is how in Xaudio2
```

# We need a NOT windows audio buffer to control!!!
## [Day 12]
[Add a sound output fn & variables to GameUpdateAndRender in handmade.h]( https://youtu.be/5YhR2zAkQmo?t=671)

[add a sound buffer struct to handmade.h](https://youtu.be/5yhr2zakqmo?t=927)

[add GameoutputSound to handmad.cpp](https://youtu.be/5YhR2zAkQmo)

[creat sounddbuffer in platformlayer win32_handmade.cpp](https://youtu.be/5YhR2zAkQmo?t=1606)

[allocate sample memeory to the stack](https://youtu.be/5YhR2zAkQmo?t=2760)


https://www.reddit.com/r/gamedev/comments/6bgay2/i_am_creating_an_audio_engine_for_my_game_using/

    Typically that sort of dynamic music would be accomplished by dividing the soundtrack into "stems," separate audio sources containing the individual elements that can be added or removed during the game. You would want to ask your composer to export the soundtrack as separate wavs (or oggs or whatever) with the appropriate tracks toggled for each. These would then be kept in sync at runtime by playing them simultaneously and adjusting their volumes over time to bring elements in as they're desired.

    You could implement this with multiple voices, one per stem, or you could use a single voice and mix the stems yourself, a sample at a time, before submitting it to the voice. The latter might be preferable for ensuring all the stems stay perfectly in sync even if the streaming thread falls behind a bit here or there.

    As to your second question, sound effects don't have to be played in a separate thread if they can be fully loaded into memory and won't be streaming data from disk to a fixed-size ring buffer. But in my experience, it may be helpful to treat all audio the same regardless of source. The most recent version of my engine's audio code handles all playback from all sources in a thread, and abstracts away the location of the audio source (memory or disk), so I have fewer one-off implementations for different use cases.


No, XAudio2 does not have a built-in ring buffer. Instead, it uses a queue of individual audio buffers that an application must manage. This is different from a typical ring buffer, where the buffer is a single, continuous block of memory that wraps around. [1, 2, 3, 4, 5]  
For tasks like streaming audio, you must implement the ring buffer behavior yourself by managing a queue of discrete  objects. 
How to implement streaming with XAudio2 The standard practice for streaming large audio files or continuous audio input in XAudio2 is to use a separate thread to manage the buffer queue. The process involves these steps: 

1. Allocate buffers: Create multiple  structures and their corresponding memory blocks. 
2. Submit to the queue: Fill the first available buffer with audio data and submit it to a source voice using . 
3. Process with callbacks: 

	• Implement an  interface to receive event notifications from the source voice. 
	• When a buffer is finished playing, the  callback is triggered. This notifies your application that the buffer is available to be refilled. 

4. Loop and refill: In your streaming thread, wait for the  event. When it arrives, refill the now-available buffer with new audio data from your source (e.g., from a file or network stream) and re-submit it to the queue. 
5. Stop gracefully: When you reach the end of the audio stream, submit the final buffer with the  flag to signal the voice that no more data will be sent. [2, 6, 7, 8, 9]  

