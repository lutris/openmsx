# $Id$

include build/node-start.mk

SRC_HDR:= \
	Mixer MSXMixer NullSoundDriver \
	SDLSoundDriver DirectXSoundDriver \
	SoundDevice \
	MSXPSG AY8910 AY8910Periphery \
	DACSound16S DACSound8U \
	KeyClick \
	SCC MSXSCCPlusCart \
	VLM5030 \
	MSXAudio \
	EmuTimer \
	Y8950 Y8950Adpcm Y8950KeyboardConnector Y8950KeyboardDevice \
	MSXFmPac MSXMusic \
	YM2413Core YM2413 YM2413_2 \
	MSXTurboRPCM \
	YMF262 YMF278 MSXMoonSound \
	YM2151 MSXYamahaSFG \
	AudioInputConnector AudioInputDevice \
	DummyAudioInputDevice WavAudioInput \
	WavWriter \
	SamplePlayer \
	WavData \
	Resample 

HDR_ONLY:= \
	ChannelMode \
	SoundDriver \
	Y8950Periphery

DIST:= \
	ResampleCoeffs.ii

include build/node-end.mk

