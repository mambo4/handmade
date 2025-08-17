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
no platform independent stuff
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
DWORD Region1Size; // sizeof global buiffer?
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