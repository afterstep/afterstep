/*
 * Copyright (C) 2007,2008 Jeremy sG <sg at hacksess>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


typedef unsigned char UCHAR;

// For ALSA calls
struct SND2dev
{
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
};

// Wave File Header Info
// in order they're read
struct WaveHeader
{
    UCHAR *cnkID[4];	// Byte 0-3  : Chunk ID (= "RIFF")
    uint fileSize;	// Byte 4-7  : Fize Size (minus first 8 bytes)
    UCHAR *fmtName[4];	// Byte 8-11 : Format (= "WAVE")
    UCHAR *scnkID[4];	// Byte 12-15: SubChunk 1 ID (= "fmt")
    uint sSize;		// Byte 16-19: SubChunk 1 Size (= 16)
    uint codec;		// Byte 20-21: Audio Format (= 1,++)
    uint chans;		// Byte 22-23: Num of Channels (1,2,++)
    uint sampRate;	// Byte 24-27: Sample Rate (= 22050,44100,+/-)
    uint bRate;		// Byte 28-31: Byte Rate (???)
    uint bAlign;	// Byte 32-33: Block Align (???)
    uint bpsmpl;	// Byte 34-35: Bits Per Sample (???)
    UCHAR *scnk2ID[4];	// Byte 36-39: SubChunk 2 ID (= ???)
    uint scnk2size;	// Byte 40-43: SubChunk 2 Size
};

UCHAR header_byte(UCHAR *buffer, int bytc)
{
    return ((uint)buffer[bytc+3] << 24) | ((uint)buffer[bytc+2] << 16) | ((uint)buffer[bytc+1] << 8) | ((uint)buffer[bytc]);
}

