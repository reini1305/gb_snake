// header for GBDK
#ifndef sound_H
#define sound_H

// call this once before calling any sound functions
void Audio_Init(void);

// call this once per frame
void Audio_FrameProcess(void);

// start playing music <music_number>
void Audio_Music_Play(UINT8 music_number);

// stop (pause) music
void Audio_Music_Stop(void);

// resume playing music after stop
void Audio_Music_Resume(void);

// play a sound effect
void Audio_SFX_Play(UINT8 sfx_number);

// play a sound effect that need a note
void Audio_SFX_PlayNote(UINT8 sfx_number, UINT8 sfx_note);

// turn off all sound effects currently playing
void Audio_SFX_Stop(void);

// lock/unlock channel 3 
// (prevent music and sound effects to play on channel 3
// allowing for an external routine to play digital audio)
void Audio_SFX_LockChnl3(void);
void Audio_SFX_UnlockChnl3(void);

#endif