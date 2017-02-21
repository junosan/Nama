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

#ifndef GAMETHREAD_H
#define GAMETHREAD_H

#include <QThread>

#include "constants.h"
#include "player.h"
#include "gameinfo.h"

class Player;
class GameInfo;

class GameThread : public QThread
{
    Q_OBJECT

public:
    GameThread(QObject *parent = 0);
    ~GameThread();

	Player* player[4];
    GameInfo* pGameInfo;

    static void shanten(const int* te, int* machi, int* hairi);
    static void sortHai(int* arr, int n);
    static void sortHaiFixAka(int* arr, int n);
    static int calcFu(const int* te, const int* hairi, int agari, int chanfonpai, int menfonpai); // for one hairi
    static int calcFan(const int* te, int yaku, const int* omotedora, const int* uradora, int isNotMenzen);
    static int calcTen(int fu, int fan, int mult);
    static void printHai(const int* te, int n);
    static void printTe(const int* te);
    static void printNaki(const int* te);

signals:
    void emitSound(int soundEnum);
    void updateScreen();

public slots:
    void startKyoku();

protected:
    void run();

private:

	int shipai(int* yama, int* kawa, int bUseAkadora);
	int haipai(int* yama, int* te);
	int tsumo(int* yama, int* te);
	int getNKan(const int* te);
	int getDora(const int* yama, const int* te, int* omotedora, int* uradora, int prevNaki);

	int copyTeN(int* to, const int* from, int n);
	int str2te(int* te, const char* str, int maxN);
	int printMachi(int* machi);
	int printHairi(int* te, int* hairi);
	int printWanpai(int* yama, int nKan, int prevNaki);
	int updateScreen(int* yama, int* kawa, int* te, int nKan, int prevNaki);
	
	static int sortPartition(int* array, int left, int right, int pivotIndex); // internal
    static void searchForm(const int* tTe, int nHai, int nNaki, const int* inds, int depth, int* form, int* cache, int* machi, int* hairi); // internal

	int calcTeyakuman(const int* te, const int* hairi, int agari);
	int markTeyakuman(int* te, int agari);
	int calcTeyaku(const int* te, const int* hairi, int agari, int chanfonpai, int menfonpai); // for one hairi
	int markTeyaku(int* te, int agari, int chanfonpai, int menfonpai);
	int printYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen);
	int calcNotenbappu(const int* te, const int* machi, const int* kawa, int* tenDiff, int bAtamahane);
	
    int updateGameInfo();
	int updatePlayerData(int playerDir);
	int askDahai(int* te, int prevNaki, int optionChosen);
	int askRichi(int* te, int optionChosen);
	int askTsumoho(int* te, int optionChosen);
	int askRonho(int* te, int optionChosen);
	int askPon(int* te, int sutehai, int turnDiff, int optionChosen);
	int askChi(int* te, int sutehai, int optionChosen);
	int askAnkan(int* te, int optionChosen);
	int askDaiminkan(int* te, int sutehai, int turnDiff, int optionChosen);
	int askKakan(int* te, int optionChosen);
	int askKyushukyuhai(int* te, int optionChosen);
	

	int sai;
	int yama[IND_NYAMA + 1], kawa[IND_KAWASIZE * 4];
	int te[IND_TESIZE * 4], nKan, curDir, nakiDir, sutehai, chanfonpai, turnDiff, skipRest, endFlag, prevNaki;
	int machi[IND_MACHISIZE * 4], hairi[IND_NHAIRI + 1], iHairi[4];
	int omotedora[5], uradora[5], tenDiff[4];
	int ruleFlags;
	int dirToPlayer[4];
    int nRichibou, nTsumibou;
    int richiSuccess[4];
	
};

#endif
