; C interface to asm audio routines

.area _CODE

_Audio_Music_Stop::
    jp 0x4009

_Audio_Music_Resume::
    jp 0x400C

.globl _Audio_SFX_Stop
_Audio_SFX_Stop:
    jp 0x4012

.globl _Audio_SFX_LockChnl3
_Audio_SFX_LockChnl3:
    jp 0x4015

.globl _Audio_SFX_UnlockChnl3
_Audio_SFX_UnlockChnl3:
    jp 0x4018

; ---------------------------

.globl _Audio_Init
_Audio_Init:
	PUSH BC
    call 0x4000
	POP BC
	RET

.globl _Audio_FrameProcess
_Audio_FrameProcess:
	PUSH BC
    call 0x4003
	POP BC
	RET

.globl _Audio_Music_Play
_Audio_Music_Play:
	LDA HL, 2(SP)
	LD A, (HL)
    jp 0x4006

.globl _Audio_SFX_Play
_Audio_SFX_Play:
	LDA HL, 2(SP)
	LD A, (HL)
    jp 0x400F

.globl _Audio_SFX_PlayNote
_Audio_SFX_PlayNote:
	LDA HL, 2(SP)
	LD A, (HL)
	inc hl
	LD L, (HL)
    jp 0x400F
