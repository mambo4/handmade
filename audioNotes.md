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
## DAY 007: 
- https://youtu.be/qGC3xiliJW8 
    - DS: You need a window to initilaize DirectSound
    - XA: AI says Xaudio does not require a window, so we can intialize elsewhere?

### casey code
```
internal void
Win32InitDSound(void)
{
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll")
    if(DsoundLibrary)
    {
>>>https://youtu.be/qGC3xiliJW8?t=1647
        // get a direct sound object

        // create primary buffer (to set mode)

        //creat secondary buffer 

        // start it playing
    }
}
```