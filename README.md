# handmade
- following handmade hero
- to learn more real programming

# current video spot -start direct sound
https://youtu.be/qGC3xiliJW8?t=1443

# video links
- [Day 005 windows Graphics recap ](https://www.youtube.com/watch?v=w7ay7QXmo_o&t=5998s&ab_channel=MollyRocket)
- [Day 006 game pad input ](https://www.youtube.com/watch?v=J3y1x54vyIQ&ab_channel=MollyRocket)
- [ Day 007 - Initializing DirectSound](https://www.youtube.com/watch?v=qGC3xiliJW8&ab_channel=MollyRocket)

# forum posts
[ a value of type "const char *" cannot be assigned to an entity of type "LPCWSTR"](https://hero.handmade.network/forums/game-discussion/t/8596-day_005___cannot_convert_from_%2527const_wchar_t_24%2527_to_%2527lpcstr%2527)

[day 008: IDirectSoundBuffer::GetCurrentPosition has changed since 2014... How to cope with DWORD vs LPDWORD args]

I 'm following Handmade Hero Day 008 - Writing a Square Wave to DirectSound

It seems like the GlobalSecondaryBuffer->GetCurrentPosition function signature has changed arg types since the stream was recorded in 2014
When I try to pass it DWORD args I get comiple errors.

```
DWORD PlayCursor;
DWORD WriteCursor;
if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(PlayCursor, WriteCursor)))
```
yields this compile error:
```
'HRESULT IDirectSoundBuffer::GetCurrentPosition(LPDWORD,LPDWORD)': cannot convert argument 1 from 'DWORD' to 'LPDWORD'
```

so it seems to IDirectSoundBuffer, in 2022, wants:

```
LPDWORD PlayCursor;
LPDWORD WriteCursor;
if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(PlayCursor, WriteCursor)))
```

there are similar 'DWORD' to 'LPDWORD' issues with GlobalSecondaryBuffer->Lock
```
VOID *Region1;
DWORD Region1Size;
VOID *Region2;
DWORD Region2Size;

if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                            &Region1, Region1Size,
                                            &Region2, Region2Size,
                                            0)))
```
```
'HRESULT IDirectSoundBuffer::Lock(DWORD,DWORD,LPVOID *,LPDWORD,LPVOID *,LPDWORD,DWORD)': cannot convert argument 1 from 'LPDWORD' to 'DWORD'
```

I think: just change DWORD to LPDWORD where the functions complain about it
But this  leads to a host of illegal operations beetween DWORD and LPDWORD that Casey did not handle
```
\W:\handmade\code\win32_handmade.cpp(574): error C2446: '>': no conversion from 'LPDWORD' to 'DWORD'
W:\handmade\code\win32_handmade.cpp(574): note: There is no context in which this conversion is possible
W:\handmade\code\win32_handmade.cpp(577): error C2297: '+=': illegal, right operand has type 'LPDWORD'
W:\handmade\code\win32_handmade.cpp(577): error C2040: '+=': 'DWORD' differs in levels of indirection from 'LPDWORD'
W:\handmade\code\win32_handmade.cpp(582): error C2440: '=': cannot convert from 'LPDWORD' to 'DWORD'
W:\handmade\code\win32_handmade.cpp(582): note: There is no context in which this conversion is possible
```


@ MSDN, DWORD is 32-bit unsigned integer
LPDWORD has no documentation page of its own
just this type declaration which I don't underdstand

```
 typedef unsigned long DWORD, *PDWORD, *LPDWORD;
```

which makes LPDWORD a pointer, I think? Which is also unlike what Casey dealt with.

So I'm tryign to figure what needs to be changed to LPDWORD and what doesn't.
Do all DWORDS need to be LPDWORDS ? Can I Cast DWORDS to LPDWORDS on need?

https://www.youtube.com/watch?v=M3ehI_LpmGw&ab_channel=DoomPenguin

