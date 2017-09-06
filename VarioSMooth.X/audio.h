/* 
 * File:   audio.h
 * Author: sam
 *
 * Created on 24 juin 2012, 13:37
 */

#ifndef AUDIO_H
#define	AUDIO_H

void audio_Config(void);
void audio_Beep(u32);
void setPreviousVolume(void);
void setNextVolume(void);
void comparatorStop(void);

#endif	/* AUDIO_H */

