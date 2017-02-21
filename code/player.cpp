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

#include <stdio.h>

#include "player.h"
#include "gamethread.h"

// for mt19937ar.c
void init_genrand(unsigned long s);
unsigned long genrand_int32(void);

Player::Player()
{
    initialize();

	clearActions();

    playerName = QString(tr("名前"));
    myDir = 0;
}

void Player::initialize()
{
    for (int i = 0; i < IND_NAKISTATE; i++)
        te[i] = HAI_EMPTY;

    te[IND_NAKISTATE] = 0;
}

void Player::setPlayerName(QString name)
{
    playerName = name;
}

QString Player::getPlayerName()
{
    return playerName;
}

void Player::setUuid(QString _uuid)
{
    uuid = _uuid;
}

QString Player::getUuid()
{
    return uuid;
}

void Player::setPlayerType(int type, QString libFile)
{
    playerType = type;

    if (playerType == PLAYER_LIB_AI) {
        aiLib.setFileName(libFile);
        if (!aiLib.load()) {
#ifdef DEBUG_LIB_AI
            printf("Player: library load failed %s\n", libFile.toLatin1().constData());
            fflush(stdout);
#endif
            playerType = PLAYER_NATIVE_AI;
            return;
        }

        aiLibInit = (AiLibInitFunc)aiLib.resolve("initialize");
        aiLibAsk = (AiLibAskFunc)aiLib.resolve("askAction");

        if (!aiLibInit || !aiLibAsk) {
#ifdef DEBUG_LIB_AI
            printf("Player: function resolve failed\n");
            fflush(stdout);
#endif
            playerType = PLAYER_NATIVE_AI;
            return;
        }

        aiLibInit();
    }
}

void Player::setHaiButton(HaiButton* p)
{
    pHaiButton = p;
}

void Player::setGameInfo(const GameInfo *p)
{
    pGameInfo = p;
}

int Player::registerAction(int actionType, const int* inds)
{
    QMutexLocker locker(&mutex);

	int maxInd, i;
	int* p;

	switch (actionType)
	{
	case ACTION_RONHO:
	case ACTION_DAIMINKAN:
	case ACTION_TSUMOHO:
	case ACTION_KYUSHUKYUHAI:
		actionAvail[actionType] = 1;
		return 0;
		break;
	case ACTION_ANKAN:
		p = ankanInds;
		maxInd = 12;
		break;
	case ACTION_KAKAN:
		p = kakanInds;
		maxInd = 3;
		break;
	case ACTION_RICHI:
		p = richiInds;
		maxInd = 14;
		break;
	case ACTION_DAHAI:
		p = dahaiInds;
		maxInd = 14;
		break;
	case ACTION_PON:
		p = ponInds;
		maxInd = 4;
		break;
	case ACTION_CHI:
		p = chiInds;
		maxInd = 10;
		break;
    default: // this should not happen
        p = NULL;
        maxInd = -1;
        break;
	}

	actionAvail[actionType] = 1;

	for (i = 0; i <= maxInd; i++) {
		p[i] = inds[i];
		if (inds[i] == -1)
			break;
	}

	return 0;
}

int Player::askAction(int* pOptionChosen)
{
    int firstAction, actionButtonShown;
    int i;
    int nOption, optionChosen;

    const int* kawa = pGameInfo->kawa;
    int curDir = pGameInfo->curDir;
    int chanfonpai = pGameInfo->chanfonpai;
    int nHai = 13 - 3 * (te[IND_NAKISTATE] & MASK_TE_NNAKI);

    firstAction = -1;
	for (int i = 0; i < 10; i++)
		if (actionAvail[i] == 1) {
			firstAction = i;
			break;
		}

    if (firstAction == -1)
	{
		clearActions();
        return ACTION_PASS;
	}

    emit updateScreen();

    if ((firstAction <= ACTION_DAHAI) && (pGameInfo->prevNaki != NAKI_CHI) && (pGameInfo->prevNaki != NAKI_PON))
        emit emitSound(SOUND_TSUMO);

    if ((playerType >= 0) && (playerType <= 2)) { // network client
        emit needAnswerViaNetwork(playerType);

        mutex.lock();
        waitAnswer.wait(&mutex);
        mutex.unlock();

        *pOptionChosen = networkAnswerOptionChosen;
        clearActions();
        return networkAnswerAction;
    }

    if (playerType == PLAYER_LIB_AI) {
        updateAiLibData();
        AiAnswer ans = aiLibAsk(aiLibData);
        ans = validateAiLibAnswer(ans, firstAction);

        *pOptionChosen = ans.optionChosen;
        clearActions();
        return ans.action;
    }

    // first half (tsumoho, kyushukyuhai, ankan, kakan, richi, dahai)
    if (firstAction <= ACTION_DAHAI)
    {
        if (playerType == PLAYER_MANUAL) // manual (on screen)
        {
            if ((te[IND_NAKISTATE] & FLAG_TE_RICHI) && (actionAvail[ACTION_TSUMOHO] == 0) && (actionAvail[ACTION_ANKAN] == 0))
            {
                SLEEP_MS(500);

                *pOptionChosen = 0;
                clearActions();
                return ACTION_DAHAI;
            }

            actionButtonShown = 0;

            if (actionAvail[ACTION_TSUMOHO]) {
                pHaiButton->registerActionButton(ACTION_TSUMOHO);
                actionButtonShown = 1;
            }

            if (actionAvail[ACTION_KYUSHUKYUHAI]) {
                pHaiButton->registerActionButton(ACTION_KYUSHUKYUHAI);
                actionButtonShown = 1;
            }

            if (actionAvail[ACTION_ANKAN] || actionAvail[ACTION_KAKAN]) {
                pHaiButton->registerActionButton(ACTION_ANKAN);
                actionButtonShown = 1;
            }

            if (actionAvail[ACTION_RICHI]) {
                pHaiButton->registerActionButton(ACTION_RICHI);
                actionButtonShown = 1;
            }

            if (actionButtonShown)
                emit emitSound(SOUND_ASK);

            // transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] |= FLAG_HAI_TRANSPARENT;
            }

            if (actionAvail[ACTION_DAHAI]) {
                for (i = 0; i <= IND_TSUMOHAI; i++) {
                    if (dahaiInds[i] == -1)
                        break;

                    pHaiButton->registerHaiButton(dahaiInds[i], i, te[IND_NAKISTATE] & MASK_TE_NNAKI, (dahaiInds[i] == IND_TSUMOHAI) && (pGameInfo->prevNaki != NAKI_CHI) && (pGameInfo->prevNaki != NAKI_PON));
                    te[dahaiInds[i]] &= ~FLAG_HAI_TRANSPARENT;
                }
            }

            haiAnswer = -1;
            buttonAnswer = -1;
            emit actionAsked(-1.0f, -1.0f);

            mutex.lock();
            waitAnswer.wait(&mutex);
            mutex.unlock();

            pHaiButton->clearHaiButtons();
            pHaiButton->clearActionButtons();

            // un-transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] &= ~FLAG_HAI_TRANSPARENT;
            }

            if (haiAnswer != -1) // dahai chosen
            {
#ifdef FLAG_DEBUG
                printf("[%d] dahai ", myDir);
                GameThread::printHai(te + dahaiInds[haiAnswer], 1);
                printf("\n");
                fflush(stdout);
#endif

                *pOptionChosen = haiAnswer;
                clearActions();
                return ACTION_DAHAI;
            }

            if ((buttonAnswer == ACTION_TSUMOHO) || (buttonAnswer == ACTION_KYUSHUKYUHAI)) {
                *pOptionChosen = 0;
                clearActions();
                return buttonAnswer;
            }

            // transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] |= FLAG_HAI_TRANSPARENT;
            }

            int nAnkanChoices = 0;
            if ((buttonAnswer == ACTION_ANKAN) && (actionAvail[ACTION_ANKAN])) {
                for (i = 0; i < 3; i++) {
                    if (ankanInds[(i << 2)] == -1)
                        break;

                    nAnkanChoices++;

                    for (int j = 0; j < 4; j++) { // can't kan right after pon/chi
                        pHaiButton->registerHaiButton(ankanInds[(i << 2) + j], i, te[IND_NAKISTATE] & MASK_TE_NNAKI, ankanInds[(i << 2) + j] == IND_TSUMOHAI);
                        te[ankanInds[(i << 2) + j]] &= ~FLAG_HAI_TRANSPARENT;
                    }
                }
            }

            int nKakanChoices = 0;
            if ((buttonAnswer == ACTION_ANKAN) && (actionAvail[ACTION_KAKAN])) {
                for (i = 0; i < 3; i++) {
                    if (kakanInds[i] == -1)
                        break;

                    nKakanChoices++;

                    pHaiButton->registerHaiButton(kakanInds[i], i + IND_NAKISTATE, te[IND_NAKISTATE] & MASK_TE_NNAKI, kakanInds[i] == IND_TSUMOHAI);
                    te[kakanInds[i]] &= ~FLAG_HAI_TRANSPARENT;
                }
            }

            if ((nAnkanChoices == 1) && (nKakanChoices == 0)) {
                // un-transparent
                for (i = 0; i <= nHai; i++) {
                    if (i == nHai)
                        i = IND_TSUMOHAI;
                    te[i] &= ~FLAG_HAI_TRANSPARENT;
                }

                pHaiButton->clearHaiButtons();

                *pOptionChosen = 0;
                clearActions();
                return ACTION_ANKAN;
            }

            if ((nAnkanChoices == 0) && (nKakanChoices == 1)) {
                // un-transparent
                for (i = 0; i <= nHai; i++) {
                    if (i == nHai)
                        i = IND_TSUMOHAI;
                    te[i] &= ~FLAG_HAI_TRANSPARENT;
                }

                pHaiButton->clearHaiButtons();

                *pOptionChosen = 0;
                clearActions();
                return ACTION_KAKAN;
            }

            if (buttonAnswer == ACTION_RICHI) {
                for (i = 0; i <= IND_TSUMOHAI; i++) {
                    if (richiInds[i] == -1)
                        break;

                    pHaiButton->registerHaiButton(richiInds[i], i + 2 * IND_NAKISTATE, te[IND_NAKISTATE] & MASK_TE_NNAKI, richiInds[i] == IND_TSUMOHAI);
                    te[richiInds[i]] &= ~FLAG_HAI_TRANSPARENT;
                }
            }

            haiAnswer = -1;
            emit actionAsked(-1.0f, -1.0f);

            mutex.lock();
            waitAnswer.wait(&mutex);
            mutex.unlock();

            pHaiButton->clearHaiButtons();

            // un-transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] &= ~FLAG_HAI_TRANSPARENT;
            }

            if (haiAnswer < IND_NAKISTATE) {
                *pOptionChosen = haiAnswer;
                clearActions();
                return ACTION_ANKAN;
            }
            else if (haiAnswer < 2 * IND_NAKISTATE) {
                *pOptionChosen = haiAnswer - IND_NAKISTATE;
                clearActions();
                return ACTION_KAKAN;
            }
            else {
                *pOptionChosen = haiAnswer - 2 * IND_NAKISTATE;
                clearActions();
                return ACTION_RICHI;
            }
        }
        else if (playerType == PLAYER_NATIVE_AI) // AI
        {
            SLEEP_MS(500);

            if (actionAvail[ACTION_TSUMOHO]) {
                *pOptionChosen = 0;
                clearActions();
                return ACTION_TSUMOHO;
            }

            if (actionAvail[ACTION_KYUSHUKYUHAI]) {
                *pOptionChosen = 0;
                clearActions();
                return ACTION_KYUSHUKYUHAI;
            }

            if (actionAvail[ACTION_ANKAN]) {
                *pOptionChosen = 0;
                clearActions();
                return ACTION_ANKAN;
            }

            if (actionAvail[ACTION_KAKAN]) {
                *pOptionChosen = 0;
                clearActions();
                return ACTION_KAKAN;
            }

            if (actionAvail[ACTION_RICHI]) {
                *pOptionChosen = 0;
                clearActions();
                return ACTION_RICHI;
            }

            if (actionAvail[ACTION_DAHAI]) {

                if (te[IND_NAKISTATE] & FLAG_TE_RICHI)
                {
                    *pOptionChosen = 0;
                    clearActions();
                    return ACTION_DAHAI;
                }

                for (i = 0; i <= IND_TSUMOHAI; i++)
                    if (dahaiInds[i] == -1)
                        break;
                nOption = i;

                optionChosen = genrand_int32() % nOption;

                // THIS IS TEMPORARY (checking premitive optimal dahai)
                int hai[34], maxMachi, tMachi, maxInds[15], nMaxInds, tTe[15], machi[IND_MACHISIZE], minShanten, tShanten;

                countVisible(hai);

                // try throwing away one hai from 0~IND_TSUMOHAI and see how wide machi is
                maxMachi = nMaxInds = 0;
                minShanten = 6;
                for (i = 0; i <= IND_TSUMOHAI; i++)
                    tTe[i] = te[i] & MASK_HAI_NUMBER;
                tTe[IND_NAKISTATE] = te[IND_NAKISTATE];
                for (i = 0; i < nOption; i++)
                {
                    tTe[dahaiInds[i]] = te[IND_TSUMOHAI] & MASK_HAI_NUMBER;

                    // check machi
                    GameThread::shanten(tTe, machi, NULL);

                    tShanten = machi[IND_NSHANTEN];

                    tMachi = 0;
                    for (int j = 0; j < 34; j++)
                        if (machi[j])
                            tMachi += 4 - hai[j];

                    if ((tShanten < minShanten) || ((tShanten == minShanten) && (tMachi > maxMachi))) // new max
                    {
                        maxMachi = tMachi;
                        minShanten = tShanten;
                        nMaxInds = 0;
                    }
                    if ((tShanten == minShanten) && (tMachi >= maxMachi)) // new or same max
                        maxInds[nMaxInds++] = i;

                    tTe[dahaiInds[i]] = te[dahaiInds[i]] & MASK_HAI_NUMBER;
                }
                if (nMaxInds)
                    optionChosen = maxInds[nMaxInds - 1];

#ifdef FLAG_DEBUG
                optionChosen = nOption - 1; // tsumogiri for debug mode

                printf("[%d] dahai ", myDir);
                GameThread::printHai(te + dahaiInds[optionChosen], 1);
                printf("\n");
                fflush(stdout);
#endif

                *pOptionChosen = optionChosen;
                clearActions();
                return ACTION_DAHAI;
            }
        }
	}
	// second half (ron, daiminkan, pon, chi)
	else
	{
		int sutehai = kawa[IND_KAWASIZE * curDir + kawa[IND_KAWASIZE * curDir + IND_NKAWA] - 1];

        if (playerType == PLAYER_MANUAL)
        {
            pHaiButton->registerActionButton(ACTION_PASS);

            if (actionAvail[ACTION_RONHO])
                pHaiButton->registerActionButton(ACTION_RONHO);

            if (actionAvail[ACTION_DAIMINKAN])
                pHaiButton->registerActionButton(ACTION_DAIMINKAN);

            if (actionAvail[ACTION_PON])
                pHaiButton->registerActionButton(ACTION_PON);

            if (actionAvail[ACTION_CHI])
                pHaiButton->registerActionButton(ACTION_CHI);

            emit emitSound(SOUND_ASK);

            buttonAnswer = -1;
            emit actionAsked(-1.0f, -1.0f);

            mutex.lock();
            waitAnswer.wait(&mutex);
            mutex.unlock();

            pHaiButton->clearActionButtons();

            if ((buttonAnswer == ACTION_PASS) || (buttonAnswer == ACTION_RONHO) || (buttonAnswer == ACTION_DAIMINKAN)) {
                *pOptionChosen = 0;
                clearActions();
                return buttonAnswer;
            }

            // transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] |= FLAG_HAI_TRANSPARENT;
            }

            if (buttonAnswer == ACTION_PON) {
                if (ponInds[(1 << 1)] == -1) // only 1 option
                {
                    // un-transparent
                    for (i = 0; i <= nHai; i++) {
                        if (i == nHai)
                            i = IND_TSUMOHAI;
                        te[i] &= ~FLAG_HAI_TRANSPARENT;
                    }

                    *pOptionChosen = 0;
                    clearActions();
                    return ACTION_PON;
                }

                for (i = 0; i < 2; i++) {
                    pHaiButton->registerHaiButton(ponInds[(i << 1) + 0], i, te[IND_NAKISTATE] & MASK_TE_NNAKI, ponInds[(i << 1) + 0] == IND_TSUMOHAI);
                    pHaiButton->registerHaiButton(ponInds[(i << 1) + 1], i, te[IND_NAKISTATE] & MASK_TE_NNAKI, ponInds[(i << 1) + 1] == IND_TSUMOHAI);
                    te[ponInds[(i << 1) + 0]] &= ~FLAG_HAI_TRANSPARENT;
                    te[ponInds[(i << 1) + 1]] &= ~FLAG_HAI_TRANSPARENT;
                }
            }

            if (buttonAnswer == ACTION_CHI) {
                if (chiInds[(1 << 1)] == -1) // only 1 option
                {
                    // un-transparent
                    for (i = 0; i <= nHai; i++) {
                        if (i == nHai)
                            i = IND_TSUMOHAI;
                        te[i] &= ~FLAG_HAI_TRANSPARENT;
                    }

                    *pOptionChosen = 0;
                    clearActions();
                    return ACTION_CHI;
                }

                for (i = 0; i < 5; i++) {
                    if (chiInds[(i << 1)] == -1)
                        break;
                    pHaiButton->registerHaiButton(chiInds[(i << 1) + 0], i + IND_NAKISTATE, te[IND_NAKISTATE] & MASK_TE_NNAKI, chiInds[(i << 1) + 0] == IND_TSUMOHAI);
                    pHaiButton->registerHaiButton(chiInds[(i << 1) + 1], i + IND_NAKISTATE, te[IND_NAKISTATE] & MASK_TE_NNAKI, chiInds[(i << 1) + 1] == IND_TSUMOHAI);
                    te[chiInds[(i << 1) + 0]] &= ~FLAG_HAI_TRANSPARENT;
                    te[chiInds[(i << 1) + 1]] &= ~FLAG_HAI_TRANSPARENT;
                }
            }

            haiAnswer = -1;
            emit actionAsked(-1.0f, -1.0f);

            mutex.lock();
            waitAnswer.wait(&mutex);
            mutex.unlock();

            pHaiButton->clearHaiButtons();

            // un-transparent
            for (i = 0; i <= nHai; i++) {
                if (i == nHai)
                    i = IND_TSUMOHAI;
                te[i] &= ~FLAG_HAI_TRANSPARENT;
            }

            if (haiAnswer < IND_NAKISTATE) {
                *pOptionChosen = haiAnswer;
                clearActions();
                return ACTION_PON;
            } else {
                *pOptionChosen = haiAnswer - IND_NAKISTATE;
                clearActions();
                return ACTION_CHI;
            }
        }
        else if (playerType == PLAYER_NATIVE_AI) // AI
        {
            if (actionAvail[ACTION_RONHO]) {

                SLEEP_MS(500);

                clearActions();
                return ACTION_RONHO;
            }

            if (actionAvail[ACTION_DAIMINKAN]) {
                // don't kan
                //
                // SLEEP_MS(500);
                //
                // clearActions();
                // return ACTION_DAIMINKAN;
            }

            if (actionAvail[ACTION_PON]) {
                // only pon yakuhais
                if (((sutehai & MASK_HAI_NUMBER) == HAI_HAKU) || ((sutehai & MASK_HAI_NUMBER) == HAI_HATSU) || ((sutehai & MASK_HAI_NUMBER) == HAI_CHUN) || \
                    ((sutehai & MASK_HAI_NUMBER) == chanfonpai) || ((sutehai & MASK_HAI_NUMBER) == HAI_TON + myDir)) {

                    SLEEP_MS(500);

                    *pOptionChosen = 0;
                    clearActions();
                    return ACTION_PON;
                }
            }

            if (actionAvail[ACTION_CHI]) {
                // test: chi all
                //
                // SLEEP_MS(500);
                //
                //*pOptionChosen = 0;
                //clearActions();
                //return ACTION_CHI;
            }
        }
    }

    *pOptionChosen = 0;
	clearActions();
    return ACTION_PASS;
}

int Player::clearActions()
{
    QMutexLocker locker(&mutex);

    for (int i = 0; i < IND_ACTIONAVAILSIZE - 1; i++)
		actionAvail[i] = 0;

    actionAvail[ACTION_PASS] = 1;

	return 0;
}

int Player::countVisible(int* hai)
// hai[34] store how many of each hai is visible to this player
{
	int i, j, nNaki, nKawa, nakiType;
    const int *p;
    const int* kawa = pGameInfo->kawa;
    const int* naki = pGameInfo->naki;
    const int* dorahyouji = pGameInfo->dorahyouji;


	for (i = 0; i < 34; i++)
		hai[i] = 0;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;

	for (i = 0; i < 13 - 3 * nNaki; i++)
		hai[te[i] & MASK_HAI_NUMBER]++;
	if (te[IND_TSUMOHAI] != HAI_EMPTY)
		hai[te[IND_TSUMOHAI] & MASK_HAI_NUMBER]++;

	for (i = 0; i < 4; i++) {
		p = kawa + IND_KAWASIZE * i;
		nKawa = p[IND_NKAWA];
		for (j = 0; j < nKawa; j++)
			hai[p[j] & MASK_HAI_NUMBER]++;

		for (j = 0; j < 4; j++) {
			nakiType = naki[(i << 4) + (j << 2) + 0];
			if (nakiType == NAKI_NO_NAKI)
				break;
			switch (nakiType)
			{
			case NAKI_CHI:
				hai[naki[(i << 4) + (j << 2) + 1] & MASK_HAI_NUMBER]++;
				hai[naki[(i << 4) + (j << 2) + 2] & MASK_HAI_NUMBER]++;
				hai[naki[(i << 4) + (j << 2) + 3] & MASK_HAI_NUMBER]++;
				break;
			case NAKI_PON:
				hai[naki[(i << 4) + (j << 2) + 1] & MASK_HAI_NUMBER] += 3;
				break;
			case NAKI_ANKAN:
			case NAKI_DAIMINKAN:
			case NAKI_KAKAN:
				hai[naki[(i << 4) + (j << 2) + 1] & MASK_HAI_NUMBER] = 4;
				break;
			}
		}

	}

	for (i = 0; i < 5; i++)
	{
        if (dorahyouji[i] == HAI_EMPTY)
			break;
        hai[dorahyouji[i] & MASK_HAI_NUMBER]++;
	}

	return 0;
}


int Player::printNaki(const int* te)
{
    int nNaki, i, temp, nakiKind;

    nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;

    // te[IND_NAKISTATE]
    // 3/3/3/3/1/3
    // (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07

    for (i = nNaki - 1; i >= 0; i--)
    {
        nakiKind = (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07;

        if (nakiKind == NAKI_ANKAN)
        {
            temp = HAI_BACKSIDE;
            printHai(&temp, 1);
            printHai(te + 10 - 3 * i + 1, 1);
        }
        else
        {
            printHai(te + 10 - 3 * i + 0, 2);
        }

        // added hai for kan's
        if ((nakiKind == NAKI_ANKAN) || (nakiKind == NAKI_DAIMINKAN) || (nakiKind == NAKI_KAKAN))
        {
            temp = te[10 - 3 * i + 0] & MASK_HAI_NUMBER;
            printHai(&temp, 1);
        }

        if (nakiKind == NAKI_ANKAN)
        {
            temp = HAI_BACKSIDE;
            printHai(&temp, 1);
        }
        else
        {
            printHai(te + 10 - 3 * i + 2, 1);
        }

        emit dispText(QString("&nbsp;"));
    }

    return 0;
}

int Player::printTe(const int* te)
{
    int nNaki, nHai;
    nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    nHai = 13 - 3 * nNaki;

    printHai(te, nHai);
    emit dispText(QString("&nbsp;"));

    printHai(te + 13, 1);
    emit dispText(QString("&nbsp;"));

    printNaki(te);

    return 0;
}

int Player::printHai(const int* te, int n)
{
	int i;
	char tile;

    char textColor[16], bgColor[16], str[256];

	for (i = 0; i < n; i++)
	{
        if(te[i] & FLAG_HAI_AKADORA)
            sprintf(bgColor, "red");
        else if(te[i] & FLAG_HAI_NOT_TSUMOHAI)
            sprintf(bgColor, "lightgray");
        else
            sprintf(bgColor, "white");

		if ((te[i] & MASK_HAI_NUMBER) <= HAI_9S)
		{
			switch ((te[i] & MASK_HAI_NUMBER) / 9)
			{
			case 0: // manzu
                sprintf(textColor, "black"); // black text
				break;
			case 1: // pinzu
                sprintf(textColor, "dodgerblue"); // bright blue text
				break;
			case 2: // sozu
                sprintf(textColor, "limegreen"); // dark green text
				break;
            }
            sprintf(str, "<h2 style=\"color:%s;background-color:%s;\">%d</h2>", textColor, bgColor, (te[i] & MASK_HAI_NUMBER) % 9 + 1);
		}
		else
		{
			switch (te[i] & MASK_HAI_NUMBER)
			{
			case HAI_TON:
                sprintf(textColor, "black"); // black text
				tile = 'E';
				break;
			case HAI_NAN:
                sprintf(textColor, "black"); // black text
				tile = 'S';
				break;
			case HAI_SHA:
                sprintf(textColor, "black"); // black text
				tile = 'W';
				break;
			case HAI_PE:
                sprintf(textColor, "black"); // black text
				tile = 'N';
				break;
			case HAI_HAKU:
                sprintf(textColor, "gray"); // gray text
                tile = 'D';
				break;
			case HAI_HATSU:
                sprintf(textColor, "limegreen"); // dark green text
                tile = 'D';
				break;
			case HAI_CHUN:
                sprintf(textColor, "red"); // bright red text
                tile = 'D';
				break;
			case HAI_BACKSIDE:
                sprintf(bgColor, "darkkhaki"); // dark yellow background
                sprintf(textColor, "white");
				tile = '_';
				break;
			case HAI_EMPTY:
            default:
                sprintf(textColor, "white"); // white text
				tile = ' ';
				break;
            }
            sprintf(str, "<h2 style=\"color:%s;background-color:%s;\">%c</h2>", textColor, bgColor, tile);
		}
        emit dispText(QString(str));
    }

	return 0;
}

void Player::receiveAnswer(QString str)
{
    QMutexLocker locker(&mutex);

    answer = str;

    waitAnswer.wakeAll();
}

void Player::receiveHaiClick(int _haiAnswer)
{
    QMutexLocker locker(&mutex);

    haiAnswer = _haiAnswer;

    waitAnswer.wakeAll();
}

void Player::receiveButtonClick(int _buttonAnswer)
{
    QMutexLocker locker(&mutex);

    buttonAnswer = _buttonAnswer;

    waitAnswer.wakeAll();
}

int Player::printSituation()
{
    char str[256];
    static int machi[IND_MACHISIZE];
    static int hairi[IND_NHAIRI + 1];

    //pDisp->clear();

    const int* kawa = pGameInfo->kawa;
    const int* naki = pGameInfo->naki;
    const int* omotedora = pGameInfo->omotedora;

    int chanfonpai = pGameInfo->chanfonpai;
    printHai(&chanfonpai, 1);
    sprintf(str, "%d kyoku | richibou %d | tsumibou %d | sai %d | %d left", pGameInfo->kyoku + 1, pGameInfo->nRichibou, pGameInfo->nTsumibou, pGameInfo->sai, pGameInfo->nYama - 14 - pGameInfo->nKan);
    emit dispText(QString(str));
    emit dispText(QString("<br><br>"));

    for (int i = 0; i < 4; i++) {        emit dispText(QString("["));
        int menfonpai = ((HAI_TON + i) | (FLAG_HAI_RICHIHAI * (i == myDir)));
        printHai(&menfonpai, 1);
        int dirToPlayer = (i + pGameInfo->kyoku) & 0x03;
        sprintf(str, "(%d)&nbsp;%d&nbsp;ten", dirToPlayer, pGameInfo->ten[dirToPlayer]);
        emit dispText(QString(str));
        emit dispText(QString("]&nbsp;naki&nbsp;"));


        for (int j = 0; j < 4; j++) {
            if (naki[(i << 4) + (j << 2)] == NAKI_NO_NAKI)
                break;
            printHai(naki + (i << 4) + (j << 2) + 1, 3);
            emit dispText(QString(" "));
        }
        emit dispText(QString("<br>&nbsp;&nbsp;&nbsp;&nbsp;kawa&nbsp;"));
        printHai(kawa + IND_KAWASIZE * i, kawa[IND_KAWASIZE * i + IND_NKAWA]);
        emit dispText(QString("<br>"));
    }
    emit dispText(QString("<br>"));

    emit dispText(QString("te:<br>"));
    printTe(te);
    emit dispText(QString("<br>"));

    sprintf(str, "0x%08x<br>", te[IND_NAKISTATE]);
    emit dispText(QString(str));

    emit dispText(QString("Dora "));
    printHai(omotedora, 5);
    emit dispText(QString("<br>"));

    GameThread::shanten(te, machi, hairi);
    sprintf(str, "%d shanten<br>machi ", machi[IND_NSHANTEN]);
    emit dispText(QString(str));
    printMachi(machi);
    if (te[IND_NAKISTATE] & FLAG_TE_RICHI)
        emit dispText(QString("<br>(richi)"));
    if (te[IND_NAKISTATE] & FLAG_TE_FURITEN)
        emit dispText(QString("<br>(furiten)"));
    if (te[IND_NAKISTATE] & FLAG_TE_AGARIHOUKI)
        emit dispText(QString("<br>(agari houki)"));
    emit dispText(QString("<br><br>"));
    printHairi(te, hairi);
    emit dispText(QString("<br>"));

    return 0;
}


int Player::printMachi(const int* machi)
{
    int i;
    int hai[1];

    for (i = 0; i < 34; i++)
        if (machi[i])
        {
            hai[0] = i;
            printHai(hai, 1);
        }

    return 0;

}


int Player::printHairi(const int* te, const int* hairi)
{
    int i, j, k, curPos;
    int nHai, nNaki; // # of hais not in naki
    int nForm, nHairi;
    char str[256];

    nHairi = hairi[IND_NHAIRI];
    nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    nHai = 13 - 3 * nNaki;

    for (i = 0; i < nHairi; i++)
    {
        sprintf(str, "[%d]&nbsp;", i);
        emit dispText(QString(str));
        curPos = 0;
        for (j = 0; j < 6; j++)
        {
            switch (hairi[i * IND_HAIRISIZE + 14 + j])
            {
            case FORM_SHUNTSU:
            case FORM_KOTSU:
                nForm = 3;
                break;
            case FORM_TOITSU:
            case FORM_RYANMEN:
            case FORM_KANCHAN:
                nForm = 2;
                break;
            case FORM_TANKI:
                nForm = nHai - curPos;
                break;
            default: // this should not happen
                nForm = 1;
                break;
            }
            if (hairi[i * IND_HAIRISIZE + 14 + j] == FORM_TANKI)
            {
                for (k = 0; k < nForm; k++)
                {
                    printHai(hairi + i * IND_HAIRISIZE + curPos + k, 1);
                    emit dispText(QString("&nbsp;"));
                }
                break;
            }
            else
            {
                printHai(hairi + i * IND_HAIRISIZE + curPos, nForm);
                emit dispText(QString("&nbsp;"));
                curPos += nForm;
            }

        }

        // for chitoi tenpai
        if ((j == 6) && (hairi[i * IND_HAIRISIZE + 14 + 5] != FORM_TANKI) && (curPos == 12))
        {
            printHai(hairi + i * IND_HAIRISIZE + curPos, 1);
            emit dispText(QString("&nbsp;"));
        }

        printNaki(te);

        emit dispText(QString("<br>"));
    }

    return 0;
}

void Player::receiveNetworkAnswer(int action, int optionChosen)
{
    networkAnswerAction = action;
    networkAnswerOptionChosen = optionChosen;

    waitAnswer.wakeAll();
}

void Player::updateAiLibData()
{
    int i;

    aiLibData.myDir = myDir;
    for (i = 0; i <= IND_NAKISTATE; i++)
        aiLibData.te[i] = te[i];
    for (i = 0; i < IND_KAWASIZE * 4; i++)
        aiLibData.kawa[i] = pGameInfo->kawa[i];
    for (i = 0; i < 16 * 4; i++)
        aiLibData.naki[i] = pGameInfo->naki[i];
    for (i = 0; i < 5; i++) {
        aiLibData.dorahyouji[i] = pGameInfo->dorahyouji[i];
        aiLibData.dora[i] = pGameInfo->omotedora[i];
    }
    for (i = 0; i < 4; i++) {
        aiLibData.ten[i] = pGameInfo->ten[i];
        aiLibData.richiSuccess[i] = pGameInfo->richiSuccess[i];
    }
    aiLibData.chanfonpai = pGameInfo->chanfonpai;
    aiLibData.kyoku = pGameInfo->kyoku;
    aiLibData.nTsumibou = pGameInfo->nTsumibou;
    aiLibData.nRichibou = pGameInfo->nRichibou;
    aiLibData.curDir = pGameInfo->curDir;
    aiLibData.prevNaki = pGameInfo->prevNaki;
    aiLibData.nYama = pGameInfo->nYama;
    aiLibData.nKan = pGameInfo->nKan;
    aiLibData.ruleFlags = pGameInfo->ruleFlags;

    for (i = 0; i < IND_ACTIONAVAILSIZE; i++)
        aiLibData.actionAvail[i] = actionAvail[i];
    for (i = 0; i < IND_ANKANINDSSIZE; i++)
        aiLibData.ankanInds[i] = ankanInds[i];
    for (i = 0; i < IND_KAKANINDSSIZE; i++)
        aiLibData.kakanInds[i] = kakanInds[i];
    for (i = 0; i < IND_RICHIINDSSIZE; i++)
        aiLibData.richiInds[i] = richiInds[i];
    for (i = 0; i < IND_DAHAIINDSSIZE; i++)
        aiLibData.dahaiInds[i] = dahaiInds[i];
    for (i = 0; i < IND_PONINDSSIZE; i++)
        aiLibData.ponInds[i] = ponInds[i];
    for (i = 0; i < IND_CHIINDSSIZE; i++)
        aiLibData.chiInds[i] = chiInds[i];
}

AiAnswer Player::validateAiLibAnswer(AiAnswer ans, int firstAction)
{
    int i;
    int wrongAns = 0;

    if ((ans.action < 0) || (ans.action >= IND_ACTIONAVAILSIZE) ||
            !actionAvail[(int)ans.action]) {
        ans.action = firstAction;
        ans.optionChosen = 0;
        wrongAns = 1;
    }

#ifdef DEBUG_LIB_AI
    if (!wrongAns) {
        printf("\n[%d] aiLib: ", myDir);
        switch (ans.action)
        {
        case ACTION_TSUMOHO:
            printf("tsumoho ");
            break;
        case ACTION_KYUSHUKYUHAI:
            printf("kyushukyuhai ");
            break;
        case ACTION_ANKAN:
            printf("ankan ");
            break;
        case ACTION_KAKAN:
            printf("kakan ");
            break;
        case ACTION_RICHI:
            printf("richi ");
            break;
        case ACTION_DAHAI:
            printf("dahai ");
            break;
        case ACTION_RONHO:
            printf("ronho ");
            break;
        case ACTION_DAIMINKAN:
            printf("daiminkan ");
            break;
        case ACTION_PON:
            printf("pon ");
            break;
        case ACTION_CHI:
            printf("chi ");
            break;
        case ACTION_PASS:
            printf("pass ");
            break;
        }
    }
#endif

    int stride = 0;
    int lim = -1;
    int *p = NULL;
    switch (ans.action)
    {
    case ACTION_ANKAN:
        stride = 4;
        lim = 3;
        p = ankanInds;
        break;
    case ACTION_KAKAN:
        stride = 1;
        lim = 3;
        p = kakanInds;
        break;
    case ACTION_RICHI:
        stride = 1;
        lim = 14;
        p = richiInds;
        break;
    case ACTION_DAHAI:
        stride = 1;
        lim = 14;
        p = dahaiInds;
        break;
    case ACTION_PON:
        stride =2;
        lim = 2;
        p = ponInds;
        break;
    case ACTION_CHI:
        stride = 2;
        lim = 5;
        p = chiInds;
        break;
    }

    if (stride && ((ans.optionChosen < 0) || (ans.optionChosen >= lim))) {
        wrongAns = 1;
        stride = 0;
    }

    if (stride) {
        for (i = 0; i <= ans.optionChosen; i++)
            if (p[i * stride] == -1) {
                wrongAns = 1;
                break;
            }

#ifdef DEBUG_LIB_AI
        if(!wrongAns) {
            for (i = 0; i < stride; i++) {
                GameThread::printHai(te + p[ans.optionChosen * stride + i], 1);
                printf(" ");
            }
        }
#endif
    }

    if (wrongAns) {
        ans.action = firstAction;
        ans.optionChosen = 0;
#ifdef DEBUG_LIB_AI
        printf("[%d] wrong answer received from aiLib\n", myDir);
#endif
    }

#ifdef DEBUG_LIB_AI
    fflush(stdout);
#endif

    return ans;
}
