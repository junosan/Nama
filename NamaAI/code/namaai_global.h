/*
   Copyright 2014 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef NAMAAI_GLOBAL_H
#define NAMAAI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(NAMAAI_LIBRARY)
    #define NAMAAI_API Q_DECL_EXPORT
#else
    #define NAMAAI_API Q_DECL_IMPORT
#endif

typedef struct {
    // stores information about current state of the player / game 
    int myDir;
    int te[15];
    int kawa[96];
    int naki[64];
    int dorahyouji[5];
    int dora[5];
    int ten[4];
    int richiSuccess[4];
    int chanfonpai;
    int kyoku;
    int nTsumibou;
    int nRichibou;
    int curDir;
    int prevNaki;
    int nYama;
    int nKan;
    int ruleFlags;

    // stores information about what actions are available
    int actionAvail[11];
    int ankanInds[13];
    int kakanInds[4];
    int richiInds[15];
    int dahaiInds[15];
    int ponInds[5];
    int chiInds[11];
} AiData;

typedef struct {
    int action;
    int optionChosen;
} AiAnswer;

extern "C" NAMAAI_API void initialize();
extern "C" NAMAAI_API AiAnswer askAction(AiData d);

#endif // NAMAAI_GLOBAL_H
