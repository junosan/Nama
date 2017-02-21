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

#include <QMutexLocker>

#include "constants.h"
#include "gamethread.h"
#include "player.h"

// for mt19937ar.c
unsigned long genrand_int32(void);

GameThread::GameThread(QObject *parent)
    : QThread(parent)
{

}

GameThread::~GameThread()
{
    /*
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
    */
    terminate();
}

void GameThread::startKyoku()
{
    if (!isRunning())
        start(QThread::NormalPriority);
}

void GameThread::run()
{
    int i, j, temp, renchan;
    int action, optionChosen;

    // Begin kyoku

    pGameInfo->initializeKyoku();

    ruleFlags = pGameInfo->ruleFlags;
    chanfonpai = pGameInfo->chanfonpai;
    nRichibou = pGameInfo->nRichibou;
    nTsumibou = pGameInfo->nTsumibou;

    shipai(yama, kawa, ruleFlags & RULE_USE_AKADORA);
    haipai(yama, te);


    // for testing //

//    for (i = 0; i < 4; i++) {
//        if (i == ((4 + pGameInfo->dispPlayer - pGameInfo->kyoku) & 0x03)) {
//            str2te(te + IND_TESIZE * i, "111222333444z56z", 14);
//            //te[IND_TESIZE * i + IND_NAKISTATE] = 2;
//            //te[IND_TESIZE * i + IND_NAKISTATE] |= FLAG_TE_NOT_MENZEN;
//            //        te[IND_TESIZE * i + IND_NAKISTATE] |= FLAG_TE_AGARIHOUKI;

//            //te[IND_TESIZE * i + IND_NAKISTATE] |= NAKI_PON << (4 + 3 * 0);
//            //te[IND_TESIZE * i + IND_NAKISTATE] |= NAKI_PON << (4 + 3 * 1);
//            //        te[IND_TESIZE * i + IND_NAKISTATE] |= NAKI_KAKAN << (4 + 3 * 2);
//            //        te[IND_TESIZE * i + IND_NAKISTATE] |= NAKI_KAKAN << (4 + 3 * 3);

//            //te[IND_TESIZE * i + 10 - 3 * 0 + 1] |= FLAG_HAI_NOT_TSUMOHAI;
//            //te[IND_TESIZE * i + 10 - 3 * 1 + 1] |= FLAG_HAI_NOT_TSUMOHAI;
//            //        te[IND_TESIZE * i + 10 - 3 * 2 + 1] |= FLAG_HAI_NOT_TSUMOHAI;
//            //        te[IND_TESIZE * i + 10 - 3 * 3 + 2] |= FLAG_HAI_NOT_TSUMOHAI;

//            //        te[IND_TESIZE * i + 3] |= FLAG_HAI_AKADORA;
//            //        te[IND_TESIZE * i + 6] |= FLAG_HAI_AKADORA;
//            //        te[IND_TESIZE * i + 9] |= FLAG_HAI_AKADORA;
//        }
//    }

//    //yama[yama[IND_NYAMA] - 2] = HAI_2P;
//    //yama[yama[IND_NYAMA] - 3] = HAI_2P;
//    //yama[yama[IND_NYAMA] - 4] = HAI_2P;
//    //yama[yama[IND_NYAMA] - 5] = HAI_2P;
//    yama[yama[IND_NYAMA] - 6] = HAI_2P;
//    yama[yama[IND_NYAMA] - 7] = HAI_2P;

    // for testing //


    curDir = 0;
    prevNaki = NAKI_NO_NAKI;
    nKan = getDora(yama, te, omotedora, uradora, prevNaki);

    for (i = 0; i < 4; i++) {
        sortHaiFixAka(te + IND_TESIZE * i, 13 - 3 * (te[IND_TESIZE * i + IND_NAKISTATE] & MASK_TE_NNAKI));
        shanten(te + IND_TESIZE * i, machi + IND_MACHISIZE * i, NULL);
        dirToPlayer[i] = (i + pGameInfo->kyoku) & 0x03;
        tenDiff[i] = 0; // for howanpai or nagashi-mangan
        richiSuccess[i] = 0;
        iHairi[i] = -1;
        updatePlayerData(i);
    }
    updateGameInfo();
    emit updateScreen();

    while (1)
    {
        nKan = getDora(yama, te, omotedora, uradora, prevNaki);

        sortHaiFixAka(te + IND_TESIZE * curDir, 13 - 3 * (te[IND_TESIZE * curDir + IND_NAKISTATE] & MASK_TE_NNAKI));

        if (nKan == ENDFLAG_SUKAIKAN) // this value is set as 5
        {
            endFlag = ENDFLAG_SUKAIKAN;

            emit emitSound(SOUND_AGARI);
            break;
        }

        if (yama[IND_NYAMA] - 14 - nKan <= 0)
        {
            if (calcNotenbappu(te, machi, kawa, tenDiff, ruleFlags & RULE_ATAMAHANE))
                endFlag = ENDFLAG_NAGASHIMANGAN;
            else
                endFlag = ENDFLAG_HOWANPAIPINCHU;

            emit emitSound(SOUND_AGARI);
            break;
        }

        // tsumo
        tsumo(yama, te + IND_TESIZE * curDir);

#ifdef FLAG_DEBUG
        printf("[%d] ", curDir);
        printTe(te + IND_TESIZE * curDir);
        printf("\n");
        fflush(stdout);
#endif

        // furiten cleared up unless during a richi
        if (!(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI))
            te[IND_TESIZE * curDir + IND_NAKISTATE] &= ~FLAG_TE_FURITEN;

        // check tsumoho
        if ((prevNaki != NAKI_PON) && (prevNaki != NAKI_CHI) && \
            (machi[IND_MACHISIZE * curDir + IND_NSHANTEN] == 0) && \
            machi[IND_MACHISIZE * curDir + (te[IND_TESIZE * curDir + IND_TSUMOHAI] & MASK_HAI_NUMBER)] && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
        {
            te[IND_TESIZE * curDir + IND_YAKUFLAGS] = 0;
            te[IND_TESIZE * curDir + IND_YAKUMANFLAGS] &= 0xF0000000;

            // calc tenho & chiho
            if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_DAIICHITSUMO)
            {
                if (curDir)
                    te[IND_TESIZE * curDir + IND_YAKUMANFLAGS] |= FLAG_YAKUMAN_CHIHO;
                else
                    te[IND_TESIZE * curDir + IND_YAKUMANFLAGS] |= FLAG_YAKUMAN_TENHO;
            }

            // calc teyakuman
            markTeyakuman(te + IND_TESIZE * curDir, AGARI_TSUMO);

            if (!te[IND_TESIZE * curDir + IND_YAKUMANFLAGS])
            {
                // check richi / double-richi
                if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI)
                {
                    if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_DOUBLERICHI)
                        te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_DOUBLERICHI;
                    else
                        te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_RICHI;
                }

                // check ippatsu
                if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_IPPATSU)
                    te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_IPPATSU;

                // check menzentsumo
                if (!(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN))
                    te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_MENZENTSUMO;

                // check rinshan
                if ((prevNaki == NAKI_ANKAN) || (prevNaki == NAKI_DAIMINKAN) || (prevNaki == NAKI_KAKAN))
                    te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_RINSHAN;

                // check haitei (cannot haitei-tsumo with rinshanpai)
                if ((yama[IND_NYAMA] - 14 - nKan == 0) && (prevNaki != NAKI_ANKAN) && (prevNaki != NAKI_DAIMINKAN) && (prevNaki != NAKI_KAKAN))
                    te[IND_TESIZE * curDir + IND_YAKUFLAGS] |= FLAG_YAKU_HAITEI;

                // calc teyaku
                iHairi[curDir] = markTeyaku(te + IND_TESIZE * curDir, AGARI_TSUMO, chanfonpai, HAI_TON + curDir);
            }

            // register tsumoho option if applicable
            askTsumoho(te + IND_TESIZE * curDir, ACTION_REGISTER);
        }

#ifdef FLAG_DEBUG
        printf("[%d] after register tsumoho\n", curDir);
        fflush(stdout);
#endif

        // register kyushukyuhai option if applicable
        if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_DAIICHITSUMO)
            askKyushukyuhai(te + IND_TESIZE * curDir, ACTION_REGISTER);

        // register ankan options if applicable
        if ((yama[IND_NYAMA] - 14 - nKan > 0) && (nKan != 4) && \
            (prevNaki != NAKI_PON) && (prevNaki != NAKI_CHI) && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
            askAnkan(te + IND_TESIZE * curDir, ACTION_REGISTER);

#ifdef FLAG_DEBUG
        printf("[%d] after register ankan\n", curDir);
        fflush(stdout);
#endif

        // register kakan options if applicable
        if (!(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI) && \
            (prevNaki != NAKI_PON) && (prevNaki != NAKI_CHI) && \
            (yama[IND_NYAMA] - 14 - nKan > 0) && (nKan != 4) && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
            askKakan(te + IND_TESIZE * curDir, ACTION_REGISTER);

#ifdef FLAG_DEBUG
        printf("[%d] after register kakan\n", curDir);
        fflush(stdout);
#endif

        // register richi option if applicable
        // menzen && (nShanten == 1 && machi[te[IND_TSUMOHAI] & MASK_HAI_NUMBER]) || (nShanten == 0)
        if ((yama[IND_NYAMA] - 14 - nKan >= 4) && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN) && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI) && \
            (((machi[IND_MACHISIZE * curDir + IND_NSHANTEN] == 1) && machi[IND_MACHISIZE * curDir + (te[IND_TESIZE * curDir + IND_TSUMOHAI] & MASK_HAI_NUMBER)]) || \
            (machi[IND_MACHISIZE * curDir + IND_NSHANTEN] == 0)) && \
            !(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI) &&\
            (pGameInfo->ten[dirToPlayer[curDir]] >= 1000))
            askRichi(te + IND_TESIZE * curDir, ACTION_REGISTER);

#ifdef FLAG_DEBUG
        printf("[%d] after register richi\n", curDir);
        fflush(stdout);
#endif

        // register dahai options
        askDahai(te + IND_TESIZE * curDir, prevNaki, ACTION_REGISTER);

#ifdef FLAG_DEBUG
        printf("[%d] after register dahai\n", curDir);
        fflush(stdout);
#endif

        // OS dependent macro
        SLEEP_MS(500);

        // process actions (tsumoho, kyushukyuhai, ankan, kakan, richi, dahai)
        updateGameInfo();
        for (i = 0; i < 4; i ++)
            updatePlayerData(i);
        action = player[dirToPlayer[curDir]]->askAction(&optionChosen);
        // waits until answered

#ifdef FLAG_DEBUG
        printf("[%d] after action received\n", curDir);
        fflush(stdout);
#endif

        // tsumoho chosen
        if (action == ACTION_TSUMOHO)
        {
            endFlag = ENDFLAG_TSUMOHO;

            emit emitSound(SOUND_AGARI);
            break;
        }
        else
        {
            te[IND_TESIZE * curDir + IND_YAKUFLAGS] = 0;
            te[IND_TESIZE * curDir + IND_YAKUMANFLAGS] &= 0xF0000000;

            // ippatsu disabled
            te[IND_TESIZE * curDir + IND_NAKISTATE] &= ~FLAG_TE_IPPATSU;
        }

        // kyushukyuhai chosen
        if (action == ACTION_KYUSHUKYUHAI)
        {
            endFlag = ENDFLAG_KYUSHUKYUHAI;

            emit emitSound(SOUND_AGARI);
            break;
        }

        // ankan chosen
        if (action == ACTION_ANKAN)
        {
            askAnkan(te + IND_TESIZE * curDir, optionChosen);

            // remove ippatsu
            for (i = 0; i < 4; i++)
                te[IND_TESIZE * i + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

            // rinshanpai order 1,0,3,2 : (5 - nKan) & 0x03
            // move rinshanpai to top of yama
            yama[yama[IND_NYAMA]++] = yama[(5 - nKan) & 0x03];
            yama[(5 - nKan) & 0x03] = HAI_EMPTY;

            prevNaki = NAKI_ANKAN;
            shanten(te + IND_TESIZE * curDir, machi + IND_MACHISIZE * curDir, NULL);

            emit emitSound(SOUND_NAKI);
            continue;
        }

        if (action == ACTION_KAKAN)
        {
            // (if you kakan with an akadora, switch with the previous naki hai) -- this can be handled from display func by checking ruleFlags
            // askKakan returns which hai was used to kakan
            sutehai = askKakan(te + IND_TESIZE * curDir, optionChosen);

            // check chankan
            // cannot overlap with houtei as you need at least 1 hai left to call kakan
            for (i = 1; i <= 3; i++)
            {
                nakiDir = (curDir + i) & 0x03;

                if (!(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_FURITEN) && \
                    (machi[IND_MACHISIZE * nakiDir + IND_NSHANTEN] == 0) && \
                    !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI) && \
                    machi[IND_MACHISIZE * nakiDir + (sutehai & MASK_HAI_NUMBER)])
                {
                    te[IND_TESIZE * nakiDir + IND_TSUMOHAI] = sutehai; // needed to calculate yaku

                    te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] = 0;
                    te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS] &= 0xF0000000;

                    // calc teyakuman
                    markTeyakuman(te + IND_TESIZE * nakiDir, \
                                                    (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN) ? AGARI_KUIRON : AGARI_MENZENRON);

                    if (!te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS])
                    {
                        // check richi & double-richi
                        if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_RICHI)
                        {
                            if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_DOUBLERICHI)
                                te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_DOUBLERICHI;
                            else
                                te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_RICHI;
                        }

                        // check ippatsu
                        if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_IPPATSU)
                            te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_IPPATSU;

                        // mark chankan flag
                        te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_CHANKAN;

                        // calc teyaku
                        iHairi[nakiDir] = markTeyaku(te + IND_TESIZE * nakiDir, \
                            (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN) ? AGARI_KUIRON : AGARI_MENZENRON, \
                            chanfonpai, HAI_TON + nakiDir);
                    }

                    // register ronho option if applicable
                    askRonho(te + IND_TESIZE * nakiDir, ACTION_REGISTER);
                    te[IND_TESIZE * nakiDir + IND_TSUMOHAI] = HAI_EMPTY;
                }
            }

            // process actions (ronho)
            {
                int action[4], optionChosen[4]; // 0th not used

                updateGameInfo();
                for (i = 0; i < 4; i ++)
                    updatePlayerData(i);

                for (i = 1; i <= 3; i++) {
                    action[i] = player[dirToPlayer[(curDir + i) & 0x03]]->askAction(optionChosen + i);
                }
                // wait until answered

                if ((action[1] == ACTION_RONHO) || (action[2] == ACTION_RONHO) || (action[3] == ACTION_RONHO))
                {
                    if ((action[1] == ACTION_RONHO) && (action[2] == ACTION_RONHO) && (action[3] == ACTION_RONHO)) {
                        endFlag = ENDFLAG_SANCHAHO;

                        emit emitSound(SOUND_AGARI);
                        break;
                    }
                    else
                        endFlag = ENDFLAG_RONHO;

                    for (i = 1; i <= 3; i++) {
                        if (action[i] == ACTION_RONHO) {
                            te[IND_TESIZE * ((curDir + i) & 0x03) + IND_TSUMOHAI] = sutehai;
                            if (ruleFlags & RULE_ATAMAHANE) {
                                for (j = 1; j <= 3; j++) {
                                    te[IND_TESIZE * ((curDir + i + j) & 0x03) + IND_YAKUFLAGS] = 0;
                                    te[IND_TESIZE * ((curDir + i + j) & 0x03) + IND_YAKUMANFLAGS] &= 0xF0000000;
                                }
                                break;
                            }

                        }
                    }

                    emit emitSound(SOUND_AGARI);
                    break;
                }
                else
                {
                    for (i = 1; i <= 3; i++) {
                        nakiDir = (curDir + i) & 0x03;

                        te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] = 0;
                        te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS] &= 0xF0000000;

                        // check furiten type 2 & 3
                        if ((machi[IND_MACHISIZE * nakiDir + IND_NSHANTEN] == 0) && \
                            !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI) && \
                            machi[IND_MACHISIZE * nakiDir + (sutehai & MASK_HAI_NUMBER)])
                            te[IND_TESIZE * nakiDir + IND_NAKISTATE] |= FLAG_TE_FURITEN;

                    }
                }
            }

            // remove ippatsu
            // chankan can overlap with ippatsu so don't move this up!
            for (i = 0; i < 4; i++)
                te[IND_TESIZE * i + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

            // rinshanpai order 1,0,3,2 : (5 - nKan) & 0x03
            // move rinshanpai to top of yama
            yama[yama[IND_NYAMA]++] = yama[(5 - nKan) & 0x03];
            yama[(5 - nKan) & 0x03] = HAI_EMPTY;

            prevNaki = NAKI_KAKAN;
            shanten(te + IND_TESIZE * curDir, machi + IND_MACHISIZE * curDir, NULL);

            emit emitSound(SOUND_NAKI);
            continue;
        }

        // richi chosen
        if (action == ACTION_RICHI)
        {
            askRichi(te + IND_TESIZE * curDir, optionChosen);

            te[IND_TESIZE * curDir + IND_NAKISTATE] |= FLAG_TE_IPPATSU;

            // check double-richi and mark FLAG_TE_DOUBLERICHI
            if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_DAIICHITSUMO)
                te[IND_TESIZE * curDir + IND_NAKISTATE] |= FLAG_TE_DOUBLERICHI;

            //printf("(player %d richi)\n", curDir);
        }

        // choosing dahai is implicit
        // optionChosen is ignored during richi
        sutehai = askDahai(te + IND_TESIZE * curDir, prevNaki, optionChosen);
        if (sutehai == HAI_EMPTY) // agari houki penalty because of kuikae
        {
            sutehai = te[IND_TESIZE * curDir + IND_TSUMOHAI] | FLAG_HAI_TSUMOGIRI;
            te[IND_TESIZE * curDir + IND_TSUMOHAI] = HAI_EMPTY;
            te[IND_TESIZE * curDir + IND_NAKISTATE] |= FLAG_TE_AGARIHOUKI;
        }
        kawa[IND_KAWASIZE * curDir + kawa[IND_KAWASIZE * curDir + IND_NKAWA]++] = sutehai | FLAG_HAI_LAST_DAHAI;

        // remove unnecessary flags for asking ron/daiminkan/pon/chi
        sutehai &= ~(FLAG_HAI_RICHIHAI | FLAG_HAI_TSUMOGIRI);

        shanten(te + IND_TESIZE * curDir, machi + IND_MACHISIZE * curDir, NULL);
        sortHaiFixAka(te + IND_TESIZE * curDir, 13 - 3 * (te[IND_TESIZE * curDir + IND_NAKISTATE] & MASK_TE_NNAKI));

#ifdef FLAG_DEBUG
        printf("[%d] after dahai\n", curDir);
        fflush(stdout);
#endif

        // in case someone stole richi-hai
        if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI) {
            int skip = 0;
            for (i = kawa[IND_KAWASIZE * curDir + IND_NKAWA] - 1; i >= 0; i--)
                if (kawa[IND_KAWASIZE * curDir + i] & FLAG_HAI_RICHIHAI) {
                    skip = 1;
                    break;
                }
            if (!skip)
                kawa[IND_KAWASIZE * curDir + kawa[IND_KAWASIZE * curDir + IND_NKAWA] - 1] |= FLAG_HAI_RICHIHAI;
        }

        // check furiten type 1
        if (machi[IND_MACHISIZE * curDir + IND_NSHANTEN] == 0)
        {
            // check kawa
            temp = kawa[IND_KAWASIZE * curDir + IND_NKAWA];
            for (i = 0; i < temp; i++)
                if (machi[IND_MACHISIZE * curDir + (kawa[IND_KAWASIZE * curDir + i] & MASK_HAI_NUMBER)])
                {
                    te[IND_TESIZE * curDir + IND_NAKISTATE] |= FLAG_TE_FURITEN;
                    break;
                }

            // check tacha
            if (!(te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_FURITEN))
            for (i = 1; i <= 3; i++)
            {
                nakiDir = (curDir + i) & 0x03;
                // subindex of hai with FLAG_HAI_NOT_TSUMOHAI is i-1
                for (temp = 0; temp < (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & MASK_TE_NNAKI); temp++)
                if ((te[IND_TESIZE * nakiDir + 10 - 3 * temp + i - 1] & FLAG_HAI_NOT_TSUMOHAI) && \
                    (machi[IND_MACHISIZE * curDir + (te[IND_TESIZE * nakiDir + 10 - 3 * temp + i - 1] & MASK_HAI_NUMBER)]))
                {
                    te[IND_TESIZE * curDir + IND_NAKISTATE] |= FLAG_TE_FURITEN;
                    break;
                }
            }
        }

        prevNaki = NAKI_NO_NAKI;
        turnDiff = 1;
        skipRest = 0;

        getDora(yama, te, omotedora, uradora, prevNaki);

        if (action != ACTION_RICHI)
            emit emitSound(SOUND_DAHAI);
        else
            emit emitSound(SOUND_RICHI);

        updateGameInfo();
        for (i = 0; i < 4; i ++)
            updatePlayerData(i);
        emit updateScreen();

        // update everyone's screen
        //printf("player %d dahai> ", curDir);
        //printHai(&sutehai, 1);
        //printf("\n");


        // check sufonrenta
        if ((curDir == 3) && (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_DAIICHITSUMO) && \
            ((kawa[IND_KAWASIZE * 0] & MASK_HAI_NUMBER) >= HAI_TON) && ((kawa[IND_KAWASIZE * 0] & MASK_HAI_NUMBER) <= HAI_PE) && \
            ((kawa[IND_KAWASIZE * 0] & MASK_HAI_NUMBER) == (kawa[IND_KAWASIZE * 1] & MASK_HAI_NUMBER)) && \
            ((kawa[IND_KAWASIZE * 1] & MASK_HAI_NUMBER) == (kawa[IND_KAWASIZE * 2] & MASK_HAI_NUMBER)) && \
            ((kawa[IND_KAWASIZE * 2] & MASK_HAI_NUMBER) == (kawa[IND_KAWASIZE * 3] & MASK_HAI_NUMBER)))
        {
            endFlag = ENDFLAG_SUFONRENTA;

            emit emitSound(SOUND_AGARI);
            break;
        }

        // disable FLAG_TE_DAIICHITSUMO
        te[IND_TESIZE * curDir + IND_NAKISTATE] &= ~FLAG_TE_DAIICHITSUMO;


        // check ron (options atamahane / double ron)
        // furiten type 1: machi in sutehai + hais that other's naki'd
        // furiten type 2: minogashi after richi (cannot be disabled)
        // furiten type 3: minogashi during damaten (until your next tsumo or your pon/chi/kan (but not other's))
        for (i = 1; i <= 3; i++)
        {
            nakiDir = (curDir + i) & 0x03;

            if (!(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_FURITEN) && \
                (machi[IND_MACHISIZE * nakiDir + IND_NSHANTEN] == 0) && \
                !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI) && \
                machi[IND_MACHISIZE * nakiDir + (sutehai & MASK_HAI_NUMBER)])
            {
                te[IND_TESIZE * nakiDir + IND_TSUMOHAI] = sutehai; // needed to calculate yaku

                te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] = 0;
                te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS] &= 0xF0000000;

                // calc teyakuman
                markTeyakuman(te + IND_TESIZE * nakiDir, \
                                                (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN) ? AGARI_KUIRON : AGARI_MENZENRON);

                if (!te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS])
                {
                    // check richi & double-richi
                    if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_RICHI)
                    {
                        if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_DOUBLERICHI)
                            te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_DOUBLERICHI;
                        else
                            te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_RICHI;
                    }

                    // check ippatsu
                    if (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_IPPATSU)
                        te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_IPPATSU;

                    // check houtei (can ron someone else's rinshanpai and get houtei ron)
                    if (yama[IND_NYAMA] - 14 - nKan == 0)
                        te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] |= FLAG_YAKU_HOUTEI;

                    // calc teyaku
                    iHairi[nakiDir] = markTeyaku(te + IND_TESIZE * nakiDir, \
                        (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_NOT_MENZEN) ? AGARI_KUIRON : AGARI_MENZENRON, \
                        chanfonpai, HAI_TON + nakiDir);
                }

                // register ronho option if applicable
                askRonho(te + IND_TESIZE * nakiDir, ACTION_REGISTER);
                te[IND_TESIZE * nakiDir + IND_TSUMOHAI] = HAI_EMPTY;
            }
        }

#ifdef FLAG_DEBUG
        printf("[%d] after register ron\n", curDir);
        fflush(stdout);
#endif

        for (i = 1; i <= 3; i++)
        {
            nakiDir = (curDir + i) & 0x03;

            // register daiminkan option if applicable
            if (!(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_RICHI) && \
                (yama[IND_NYAMA] - 14 - nKan > 0) && (nKan != 4) && \
                !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
                askDaiminkan(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, ACTION_REGISTER);

            // ask pon
            if (!(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_RICHI) && (yama[IND_NYAMA] - 14 - nKan > 0) && \
                !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
                askPon(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, ACTION_REGISTER);
        }

#ifdef FLAG_DEBUG
        printf("[%d] after register daiminkan/pon\n", curDir);
        fflush(stdout);
#endif

        // ask chi
        nakiDir = (curDir + 1) & 0x03;
        if ((!skipRest) && !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_RICHI) && \
            (sutehai <= HAI_9S) && (yama[IND_NYAMA] - 14 - nKan > 0) && \
            !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI))
            askChi(te + IND_TESIZE * nakiDir, sutehai, ACTION_REGISTER);

#ifdef FLAG_DEBUG
        printf("[%d] after register chi\n", curDir);
        fflush(stdout);
#endif

        // process actions (ronho, daiminkan, pon, chi)
        {
            int action[4], optionChosen[4]; // 0th not used

            updateGameInfo();
            for (i = 0; i < 4; i ++)
                updatePlayerData(i);

            for (i = 1; i <= 3; i++) {
                action[i] = player[dirToPlayer[(curDir + i) & 0x03]]->askAction(optionChosen + i);
            }
            // waits until answered


#ifdef FLAG_DEBUG
        printf("[%d] after actions received\n", curDir);
        fflush(stdout);
#endif

            if ((action[1] == ACTION_RONHO) || (action[2] == ACTION_RONHO) || (action[3] == ACTION_RONHO))
            {
                if ((action[1] == ACTION_RONHO) && (action[2] == ACTION_RONHO) && (action[3] == ACTION_RONHO)) {
                    endFlag = ENDFLAG_SANCHAHO;

                    emit emitSound(SOUND_AGARI);
                    break;
                }
                else
                    endFlag = ENDFLAG_RONHO;

                for (i = 1; i <= 3; i++) {
                    if (action[i] == ACTION_RONHO) {
                        te[IND_TESIZE * ((curDir + i) & 0x03) + IND_TSUMOHAI] = sutehai;
                        if (ruleFlags & RULE_ATAMAHANE) {
                            for (j = 1; j <= 3; j++) {
                                te[IND_TESIZE * ((curDir + i + j) & 0x03) + IND_YAKUFLAGS] = 0;
                                te[IND_TESIZE * ((curDir + i + j) & 0x03) + IND_YAKUMANFLAGS] &= 0xF0000000;
                            }
                            break;
                        }

                    }
                }

                emit emitSound(SOUND_AGARI);
                break;
            }
            else
            {
                for (i = 1; i <= 3; i++) {
                    nakiDir = (curDir + i) & 0x03;

                    te[IND_TESIZE * nakiDir + IND_YAKUFLAGS] = 0;
                    te[IND_TESIZE * nakiDir + IND_YAKUMANFLAGS] &= 0xF0000000;

                    // check furiten type 2 & 3 (keishiki tenpai also counts here)
                    if ((machi[IND_MACHISIZE * nakiDir + IND_NSHANTEN] == 0) && \
                        !(te[IND_TESIZE * nakiDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI) && \
                        machi[IND_MACHISIZE * nakiDir + (sutehai & MASK_HAI_NUMBER)])
                        te[IND_TESIZE * nakiDir + IND_NAKISTATE] |= FLAG_TE_FURITEN;

                }
            }

            // add richibou if not ron
            if((te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_RICHI) && (richiSuccess[curDir] == 0)) {
                nRichibou++;
                pGameInfo->ten[dirToPlayer[curDir]] -= 1000;
                richiSuccess[curDir] = 1;
            }


            // check sucharichi (needs to be after checking rons)
            if ((te[IND_TESIZE * 0 + IND_NAKISTATE] & FLAG_TE_RICHI) && (te[IND_TESIZE * 1 + IND_NAKISTATE] & FLAG_TE_RICHI) && \
                (te[IND_TESIZE * 2 + IND_NAKISTATE] & FLAG_TE_RICHI) && (te[IND_TESIZE * 3 + IND_NAKISTATE] & FLAG_TE_RICHI))
            {
                endFlag = ENDFLAG_SUCHARICHI;

                emit emitSound(SOUND_AGARI);
                break;
            }

            // check i == 2, 3 beforehand for kan/pon's priority over chi
            for (i = 2; i <= 3; i++) {
                nakiDir = (curDir + i) & 0x03;

                if (action[i] == ACTION_DAIMINKAN) {

                    askDaiminkan(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, optionChosen[i]);

                    // remove ippatsu
                    for (j = 0; j < 4; j++)
                        te[IND_TESIZE * j + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

                    // rinshanpai order 1,0,3,2 : (5 - nKan) & 0x03
                    // move rinshanpai to top of yama
                    yama[yama[IND_NYAMA]++] = yama[(5 - nKan) & 0x03];
                    yama[(5 - nKan) & 0x03] = HAI_EMPTY;

                    kawa[IND_KAWASIZE * curDir + IND_NKAWA]--;
                    turnDiff = (nakiDir + 4 - curDir) & 0x03;
                    prevNaki = NAKI_DAIMINKAN;
                    skipRest = 1;

                    emit emitSound(SOUND_NAKI);
                    break;
                }

                if (action[i] == ACTION_PON) {

                    askPon(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, optionChosen[i]);

                    // remove ippatsu
                    for (j = 0; j < 4; j++)
                        te[IND_TESIZE * j + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

                    kawa[IND_KAWASIZE * curDir + IND_NKAWA]--;
                    turnDiff = (nakiDir + 4 - curDir) & 0x03;
                    prevNaki = NAKI_PON;
                    skipRest = 1;

                    emit emitSound(SOUND_NAKI);
                    break;
                }
            }

            // i == 1, check daiminkan, pon, chi
            if (!skipRest) {
                i = 1;
                nakiDir = (curDir + i) & 0x03;

                if (action[i] == ACTION_DAIMINKAN) {

                    askDaiminkan(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, optionChosen[i]);

                    // remove ippatsu
                    for (j = 0; j < 4; j++)
                        te[IND_TESIZE * j + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

                    // rinshanpai order 1,0,3,2 : (5 - nKan) & 0x03
                    // move rinshanpai to top of yama
                    yama[yama[IND_NYAMA]++] = yama[(5 - nKan) & 0x03];
                    yama[(5 - nKan) & 0x03] = HAI_EMPTY;

                    kawa[IND_KAWASIZE * curDir + IND_NKAWA]--;
                    turnDiff = 1;
                    prevNaki = NAKI_DAIMINKAN;

                    emit emitSound(SOUND_NAKI);
                }

                if (action[i] == ACTION_PON) {

                    askPon(te + IND_TESIZE * nakiDir, sutehai, (curDir + 4 - nakiDir) & 0x03, optionChosen[i]);

                    // remove ippatsu
                    for (j = 0; j < 4; j++)
                        te[IND_TESIZE * j + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

                    kawa[IND_KAWASIZE * curDir + IND_NKAWA]--;
                    turnDiff = 1;
                    prevNaki = NAKI_PON;

                    emit emitSound(SOUND_NAKI);
                }

                if (action[i] == ACTION_CHI)
                {
                    askChi(te + IND_TESIZE * nakiDir, sutehai, optionChosen[i]);

                    // remove ippatsu
                    for (i = 0; i < 4; i++)
                        te[IND_TESIZE * i + IND_NAKISTATE] &= ~(FLAG_TE_IPPATSU | FLAG_TE_DAIICHITSUMO);

                    kawa[IND_KAWASIZE * curDir + IND_NKAWA]--;
                    turnDiff = 1;
                    prevNaki = NAKI_CHI;

                    emit emitSound(SOUND_NAKI);
                }
            }
        }

        kawa[IND_KAWASIZE * curDir + kawa[IND_KAWASIZE * curDir + IND_NKAWA] - 1] &= ~FLAG_HAI_LAST_DAHAI;
        curDir = (curDir + turnDiff) & 0x03;

    }

    int howanpai = 0;
    switch (endFlag)
    {
    case ENDFLAG_HOWANPAIPINCHU:
    case ENDFLAG_NAGASHIMANGAN:
        renchan = (machi[IND_MACHISIZE * 0 + IND_NSHANTEN] == 0); // renchan if oya tenpai
        howanpai = 1;
        break;
    case ENDFLAG_TSUMOHO:
    case ENDFLAG_RONHO:
        renchan = (te[IND_TESIZE * 0 + IND_YAKUMANFLAGS] & 0x0FFFFFFF) || te[IND_TESIZE * 0 + IND_YAKUFLAGS]; // renchan if oya agari
        break;
    case ENDFLAG_KYUSHUKYUHAI:
    case ENDFLAG_SUKAIKAN:
    case ENDFLAG_SUFONRENTA:
    case ENDFLAG_SUCHARICHI:
    case ENDFLAG_SANCHAHO:
        renchan = 1; // renchan
        break;
    default: // this should not happen
        renchan = 0;
        break;
    }

    int tenDiffForPlayer[4];

    for (i = 0; i < 4; i++)
        tenDiffForPlayer[dirToPlayer[i]] = tenDiff[i]; // only stores tens if ryuukyoku


#ifdef FLAG_DEBUG
    for (i = 0; i < 4; i++)
        printf("iHairi[%d] %d\n", i, iHairi[i]);
#endif

    updateGameInfo();
    for (i = 0; i < 4; i ++)
        updatePlayerData(i);

    pGameInfo->endKyoku(endFlag, renchan, howanpai, tenDiffForPlayer);
}

int GameThread::updateGameInfo()
{
    QMutexLocker locker(&pGameInfo->mutex);

    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < IND_TESIZE; j++)
            pGameInfo->te[IND_TESIZE * i + j] = te[IND_TESIZE * i + j];

        int nKawa = pGameInfo->kawa[IND_KAWASIZE * i + IND_NKAWA] = kawa[IND_KAWASIZE * i + IND_NKAWA];
        for (j = 0; j < nKawa; j++)
            pGameInfo->kawa[IND_KAWASIZE * i + j] = kawa[IND_KAWASIZE * i + j];

        int nNaki = te[IND_TESIZE * i + IND_NAKISTATE] & MASK_TE_NNAKI;
        for (j = 0; j < nNaki; j++) {
            pGameInfo->naki[(((i << 2) + j) << 2) + 0] = (te[IND_TESIZE * i + IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
            pGameInfo->naki[(((i << 2) + j) << 2) + 1] = te[IND_TESIZE * i + 10 - 3 * j + 0];
            pGameInfo->naki[(((i << 2) + j) << 2) + 2] = te[IND_TESIZE * i + 10 - 3 * j + 1];
            pGameInfo->naki[(((i << 2) + j) << 2) + 3] = te[IND_TESIZE * i + 10 - 3 * j + 2];
        }
        for (; j < 4; j++)
            pGameInfo->naki[(((i << 2) + j) << 2) + 0] = NAKI_NO_NAKI;

        pGameInfo->richiSuccess[i] = richiSuccess[i];

        pGameInfo->iHairi[i] = iHairi[i];

        pGameInfo->isTsumohaiAvail[i] = (te[IND_TESIZE * i + IND_TSUMOHAI] != HAI_EMPTY);
    }

    for (j = 0; j < 14; j++)
        pGameInfo->wanpai[j] = yama[j];

    for (i = 0; i < 5; i++) {
        pGameInfo->omotedora[i] = omotedora[i];
        pGameInfo->uradora[i] = uradora[i];
        if (omotedora[i] != HAI_EMPTY) {
            pGameInfo->dorahyouji[i] = pGameInfo->wanpai[5 + (i << 1)];
            pGameInfo->uradorahyouji[i] = pGameInfo->wanpai[4 + (i << 1)];
        }
        else {
            pGameInfo->dorahyouji[i] = HAI_EMPTY;
            pGameInfo->uradorahyouji[i] = HAI_EMPTY;
        }
    }

    pGameInfo->nYama = yama[IND_NYAMA];
    //pGameInfo->nHaiLeft = yama[IND_NYAMA] - 14 - nKan;
    pGameInfo->nKan = nKan;
    pGameInfo->curDir = curDir;
    pGameInfo->prevNaki = prevNaki;
    pGameInfo->sai = sai;

    pGameInfo->nRichibou = nRichibou;
    pGameInfo->nTsumibou = nTsumibou;

    return 0;
}

int GameThread::updatePlayerData(int playerDir)
{
    Player* pPlayer = player[dirToPlayer[playerDir]];
    QMutexLocker locker(&pPlayer->mutex);

    for (int i = 0; i <= IND_NAKISTATE; i++)
		pPlayer->te[i] = te[IND_TESIZE * playerDir + i];

	pPlayer->myDir = playerDir;

    return 0;
}

int GameThread::calcNotenbappu(const int* te, const int* machi, const int* kawa, int* tenDiff, int bAtamahane)
// return 1 for nagashimangan
{
	int curDir, nakiDir, i, skip, temp, nKawa, tenpai[4], nagashi[4];
	int minus, plus;

	for (curDir = 0; curDir < 4; curDir++)
	{
		if (te[IND_TESIZE * curDir + IND_NAKISTATE] & FLAG_TE_AGARIHOUKI)
		{
			tenpai[curDir] = nagashi[curDir] = 0;
			continue;
		}

		tenpai[curDir] = (machi[IND_MACHISIZE * curDir + IND_NSHANTEN] == 0);

		skip = 0;
		nKawa = kawa[IND_KAWASIZE * curDir + IND_NKAWA];
		for (i = 0; i < nKawa; i++)
		{
			temp = (kawa[IND_KAWASIZE * curDir + i] & MASK_HAI_NUMBER);
			if (((temp >= HAI_2M) && (temp <= HAI_8M)) || ((temp >= HAI_2P) && (temp <= HAI_8P)) || ((temp >= HAI_2S) && (temp <= HAI_8S)))
			{
				skip = 1;
				break;
			}
		}

		// check tacha
		for (i = 1; i <= 3; i++)
		{
			nakiDir = (curDir + i) & 0x03;
			// subindex of hai with FLAG_HAI_NOT_TSUMOHAI is i-1
			for (temp = 0; temp < (te[IND_TESIZE * nakiDir + IND_NAKISTATE] & MASK_TE_NNAKI); temp++)
				if (te[IND_TESIZE * nakiDir + 10 - 3 * temp + i - 1] & FLAG_HAI_NOT_TSUMOHAI)
				{
					skip = 1;
					break;
				}
		}

		nagashi[curDir] = (!skip);
	}

	if (nagashi[0] || nagashi[1] || nagashi[2] || nagashi[3]) // nagashimangan
	{
		tenDiff[0] = tenDiff[1] = tenDiff[2] = tenDiff[3] = 0;
		for (curDir = 0; curDir < 4; curDir++)
			if (nagashi[curDir])
			{
				tenDiff[curDir] += 8000 + 4000 * (curDir == 0);
				for (i = 1; i <= 3; i++)
				{
					nakiDir = (curDir + i) & 0x03;

					int agariOya = (curDir == 0);
					int imOya = (nakiDir == 0);

					tenDiff[nakiDir] -= ((2000 + 2000 * agariOya) << imOya);
				}
				if (bAtamahane)
					break;
			}
	}
	else
	{
		switch (tenpai[0] + tenpai[1] + tenpai[2] + tenpai[3])
		{
		case 1:
			minus = -1000;
			plus = 4000;
			break;
		case 2:
			minus = -1500;
			plus = 3000;
			break;
		case 3:
			minus = -3000;
			plus = 4000;
			break;
        case 0:
        case 4:
        default:
            minus = plus = 0;
            break;
		}
		for (curDir = 0; curDir < 4; curDir++)
			tenDiff[curDir] = minus + tenpai[curDir] * plus;
	}

	return nagashi[0] || nagashi[1] || nagashi[2] || nagashi[3];
}

int GameThread::askKyushukyuhai(int* te, int optionChosen)
{
	int yc, ycCheck[34], cur, i;

	// initialize
	yc = 0;
	for (i = 0; i < 34; i++)
		ycCheck[i] = 0;

	for (i = 0; i < 14; i++)
	{
		cur = te[i] & MASK_HAI_NUMBER; // masking is not needed in principle, but just in case...
		if ((cur == HAI_1M) || (cur == HAI_9M) || (cur == HAI_1P) || (cur == HAI_9P) || (cur == HAI_1S) || (cur == HAI_9S) || \
			((cur >= HAI_TON) && (cur <= HAI_CHUN)))
			ycCheck[cur]++;
	}

	for (i = 0; i < 34; i++)
		yc += (ycCheck[i] > 0); // count # of kinds of yaochuhai

	if (yc < 9)
		return 0;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_KYUSHUKYUHAI, NULL);
	}

	return 0;
}

int GameThread::getDora(const int* yama, const int* te, int* omotedora, int* uradora, int prevNaki)
{
	int nKan, nDoraHyouji, i;

	nKan = getNKan(te);

	nDoraHyouji = 1 + nKan - ((prevNaki == NAKI_DAIMINKAN) || (prevNaki == NAKI_KAKAN));

	//  [5, 7, 9, 11, 13]: dora hyoujihai
	// 	[4, 6, 8, 10, 12] : uradora

	for (i = 0; i < 5; i++)
	{
		if (i < nDoraHyouji)
		{
			omotedora[i] = (yama[5 + (i << 1)] & MASK_HAI_NUMBER);
			switch (omotedora[i])
			{
			case HAI_9M:
			case HAI_9P:
			case HAI_9S:
				omotedora[i] -= 8;
				break;
			case HAI_PE:
				omotedora[i] = HAI_TON;
				break;
			case HAI_CHUN:
				omotedora[i] = HAI_HAKU;
				break;
			default:
				omotedora[i] += 1;
				break;
			}
			uradora[i] = (yama[4 + (i << 1)] & MASK_HAI_NUMBER);
			switch (uradora[i])
			{
			case HAI_9M:
			case HAI_9P:
			case HAI_9S:
				uradora[i] -= 8;
				break;
			case HAI_PE:
				uradora[i] = HAI_TON;
				break;
			case HAI_CHUN:
				uradora[i] = HAI_HAKU;
				break;
			default:
				uradora[i] += 1;
				break;
			}
		}
		else
		{
			omotedora[i] = HAI_EMPTY;
			uradora[i] = HAI_EMPTY;
		}
	}

	return nKan;
}


int GameThread::markTeyakuman(int* te, int agari)
{
	int yakuman, nYakuman, tYakuman, tNYakuman, iHairi, i, j, skip, curPos, nToitsu;
	int machi[IND_MACHISIZE], hairi[IND_NHAIRI + 1];
	int tsumohai;

	shanten(te, machi, hairi);

	tsumohai = (te[IND_TSUMOHAI] & MASK_HAI_NUMBER);
	yakuman = nYakuman = 0;
	iHairi = -1;
	for (i = 0; i < hairi[IND_NHAIRI]; i++)
	{
		skip = 1;
		curPos = 0;

		nToitsu = 0;
        for (j = 0; j < 6; j++) {
            if (hairi[IND_HAIRISIZE * i + 14 + j] == FORM_TANKI)
                break;
			if (hairi[IND_HAIRISIZE * i + 14 + j] == FORM_TOITSU)
				nToitsu++;
        }

		for (j = 0; j < 6; j++)
		{
			switch (hairi[IND_HAIRISIZE * i + 14 + j])
			{
			case FORM_SHUNTSU:
			case FORM_KOTSU:
				curPos += 3;
				break;
			case FORM_TOITSU:
				if ((hairi[IND_HAIRISIZE * i + curPos] == tsumohai) && (nToitsu != 1))
					skip = 0;
				curPos += 2;
				break;
			case FORM_RYANMEN:
				if ((tsumohai <= HAI_9S) && \
					(((hairi[IND_HAIRISIZE * i + curPos] == tsumohai + 1) && (tsumohai % 9 != 8)) || \
					((hairi[IND_HAIRISIZE * i + curPos + 1] == tsumohai - 1) && (tsumohai % 9 != 0))))
					skip = 0;
				curPos += 2;
				break;
			case FORM_KANCHAN:
				if ((tsumohai <= HAI_9S) && (hairi[IND_HAIRISIZE * i + curPos] == tsumohai - 1))
					skip = 0;
				curPos += 2;
				break;
			case FORM_TANKI:
				if (hairi[IND_HAIRISIZE * i + curPos] == tsumohai)
					skip = 0;
				if (j == 0) // kokushi
					skip = 0;
				curPos += 1;
				break;
			}
			if (!skip)
				break;
		}
		if ((j == 6) && (hairi[IND_HAIRISIZE * i + 14 + 5] != FORM_TANKI) && (hairi[IND_HAIRISIZE * i + curPos] == tsumohai)) // tsuiso-chitoi
			skip = 0;
		if (skip)
			continue;

		tYakuman = calcTeyakuman(te, hairi + IND_HAIRISIZE * i, agari);
		tNYakuman = ((tYakuman & FLAG_YAKUMAN_KOKUSHI) != 0) + ((tYakuman & FLAG_YAKUMAN_SUANKO) != 0) + ((tYakuman & FLAG_YAKUMAN_DAISANGEN) != 0) + \
					((tYakuman & FLAG_YAKUMAN_TSUISO) != 0) + ((tYakuman & FLAG_YAKUMAN_SHOUSUSHI) != 0) + ((tYakuman & FLAG_YAKUMAN_DAISUSHI) != 0) + \
					((tYakuman & FLAG_YAKUMAN_RYUISO) != 0) + ((tYakuman & FLAG_YAKUMAN_CHINROTO) != 0) + ((tYakuman & FLAG_YAKUMAN_SUKANTSU) != 0) + \
                    ((tYakuman & FLAG_YAKUMAN_CHURENPOTO) != 0) + \
                    ((te[IND_YAKUMANFLAGS] & FLAG_YAKUMAN_TENHO) != 0) + ((te[IND_YAKUMANFLAGS] & FLAG_YAKUMAN_CHIHO) != 0);

		if (tNYakuman > nYakuman)
		{
			iHairi = i;
			yakuman = tYakuman;
			nYakuman = tNYakuman;
		}
	}

	if (iHairi == -1)
		te[IND_YAKUMANFLAGS] &= 0xF0000000;
	else
        te[IND_YAKUMANFLAGS] = te[IND_YAKUMANFLAGS] | yakuman;

	return iHairi;
}

int GameThread::markTeyaku(int* te, int agari, int chanfonpai, int menfonpai)
{
	int yaku, fu, fan, tFu, tFan, tYaku, iHairi, i, j, skip, curPos, nToitsu;
	int machi[IND_MACHISIZE], hairi[IND_NHAIRI + 1];
	int tsumohai;

	shanten(te, machi, hairi);
	
	tsumohai = (te[IND_TSUMOHAI] & MASK_HAI_NUMBER);
	fu = fan = yaku = 0;
	iHairi = -1;

	for (i = 0; i < hairi[IND_NHAIRI]; i++)
	{
		skip = 1;
		curPos = 0;

		nToitsu = 0;
        for (j = 0; j < 6; j++) {
            if (hairi[IND_HAIRISIZE * i + 14 + j] == FORM_TANKI)
                break;
			if (hairi[IND_HAIRISIZE * i + 14 + j] == FORM_TOITSU)
				nToitsu++;
        }

		for (j = 0; j < 6; j++)
		{
			switch (hairi[IND_HAIRISIZE * i + 14 + j])
			{
			case FORM_SHUNTSU:
			case FORM_KOTSU:
				curPos += 3;
				break;
			case FORM_TOITSU:
				if ((hairi[IND_HAIRISIZE * i + curPos] == tsumohai) && (nToitsu != 1))
					skip = 0;
				curPos += 2;
				break;
			case FORM_RYANMEN:
				if ((tsumohai <= HAI_9S) && \
					(((hairi[IND_HAIRISIZE * i + curPos] == tsumohai + 1) && (tsumohai % 9 != 8)) || \
					((hairi[IND_HAIRISIZE * i + curPos + 1] == tsumohai - 1) && (tsumohai % 9 != 0))))
					skip = 0;
				curPos += 2;
				break;
			case FORM_KANCHAN:
				if ((tsumohai <= HAI_9S) && (hairi[IND_HAIRISIZE * i + curPos] == tsumohai - 1))
					skip = 0;
				curPos += 2;
				break;
			case FORM_TANKI:
				if (hairi[IND_HAIRISIZE * i + curPos] == tsumohai)
					skip = 0;
				curPos += 1;
				break;
			}
			if (!skip)
				break;
		}
		if ((j == 6) && (hairi[IND_HAIRISIZE * i + 14 + 5] != FORM_TANKI) && (hairi[IND_HAIRISIZE * i + curPos] == tsumohai))
			skip = 0;
		if (skip)
			continue;

		tFu = calcFu(te, hairi + IND_HAIRISIZE * i, agari, chanfonpai, menfonpai);
		tYaku = calcTeyaku(te, hairi + IND_HAIRISIZE * i, agari, chanfonpai, menfonpai);
        tFan = calcFan(te, tYaku | te[IND_YAKUFLAGS], NULL, NULL, te[IND_NAKISTATE] & FLAG_TE_NOT_MENZEN);
		if (calcTen(tFu, tFan, 4) > calcTen(fu, fan, 4))
		{
			iHairi = i;
			fu = tFu;
			fan = tFan;
			yaku = tYaku;
		}
	}

	if (iHairi == -1)
		te[IND_YAKUFLAGS] = 0x00000000;
	else
        te[IND_YAKUFLAGS] |= yaku;

	return iHairi;
}

int GameThread::calcTen(int fu, int fan, int mult)
// ko tsumo ko payment mult = 1, oya payment mult = 2
// ko ron payment mult = 4
// oya tsumo ko payment mult = 2
// oya ron payment mult = 6
{
	int ten;

	if (fan <= 0)
		return 0;

	if (fan <= 4)
	{
		if (fu != 25)
			fu = ((fu + 9) / 10) * 10;

		ten = fu * (1 << (2 + fan));

		if (ten > 2000)
			ten = 2000;

		ten = ((ten * mult + 99) / 100) * 100;

		return ten;
	}

	if (fan <= 5)
		return 2000 * mult;

	if (fan <= 7)
		return 3000 * mult;

	if (fan <= 10)
		return 4000 * mult;

	if (fan <= 12)
		return 6000 * mult;

	return 8000 * mult;
}


int GameThread::calcTeyakuman(const int* te, const int* hairi, int agari)
// get hairi from outsite (only 1 entry)
// returns int containing flags of yakuman's except tenho & chiho
{
    int yakuman, nNaki, i, temp, skip, tTe[IND_TESIZE];
	const int *form;
    int nakiType[4];//, nakiHai[4];

	yakuman = 0x00000000;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    //nHai = 13 - 3 * nNaki;
	form = hairi + 14;

	for (i = 0; i <= IND_TSUMOHAI; i++)
		tTe[i] = (te[i] & MASK_HAI_NUMBER);

	for (i = 0; i < nNaki; i++)
	{
		nakiType[i] = ((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07);
        //nakiHai[i] = tTe[10 - (3 * i) + 0];
	}

	// sort (convenient for certain yaku's)
	sortHai(tTe, 14);

	// kokushi (cannot overlap with any other)
	if (nNaki == 0)
	{
		int yc, yctoi, ycCheck[34];

		// initialize
		yc = 0;
		yctoi = 0;
		for (i = 0; i < 34; i++)
			ycCheck[i] = 0;

		for (i = 0; i <= IND_TSUMOHAI; i++)
		{
			temp = tTe[i];
			if ((temp == HAI_1M) || (temp == HAI_9M) || (temp == HAI_1P) || (temp == HAI_9P) || (temp == HAI_1S) || (temp == HAI_9S) || \
				((temp >= HAI_TON) && (temp <= HAI_CHUN)))
			{
				ycCheck[temp]++;
				yctoi += (!yctoi) && (ycCheck[temp] > 1); // only increase yctoi once
			}
		}

		for (i = 0; i < 34; i++)
			yc += (ycCheck[i] > 0); // count # of kinds of yaochuhai

		if ((yc == 13) && (yctoi))
			return FLAG_YAKUMAN_KOKUSHI;
	}

	// churen (cannot overlap with any other)
	if ((nNaki == 0) && (tTe[0] <= HAI_9S) && (tTe[0] / 9 == tTe[IND_TSUMOHAI] / 9) && \
		(tTe[0] % 9 == 0) && (tTe[1] % 9 == 0) && (tTe[2] % 9 == 0) && \
		(tTe[IND_TSUMOHAI - 0] % 9 == 8) && (tTe[IND_TSUMOHAI - 1] % 9 == 8) && (tTe[IND_TSUMOHAI - 2] % 9 == 8))
		// 2~8 in [3]~[IND_TSUMOHAI-3]
	{
		int toi, low, high, chunchan[7];

		low = 3 + (tTe[3] % 9 == 0);
		high = IND_TSUMOHAI - 3 - (tTe[IND_TSUMOHAI - 3] % 9 == 8);

		toi = (low != 3) + (high != IND_TSUMOHAI - 3);

		if (toi <= 1)
		{
			for (i = 0; i < 7; i++)
				chunchan[i] = 0;

			for (i = low; i <= high; i++)
				chunchan[tTe[i] % 9 - 1]++;
			
			for (i = 0; i < 7; i++)
				if (!chunchan[i])
				{
					toi = 2;
					break;
				}
		}

		if (toi <= 1)
			return FLAG_YAKUMAN_CHURENPOTO;
	}

	// suanko & sukantsu
	{
        int curPos, nAnko, nKantsu;

        int nToitsu = 0;
        for (i = 0; i < 6; i++) {
            if (form[i] == FORM_TANKI)
                break;
            if (form[i] == FORM_TOITSU)
                nToitsu++;
        }

		nAnko = nKantsu = 0;
		curPos = 0;

		for (i = 0; i < 6; i++)
		{
			switch (form[i])
			{
			case FORM_KOTSU:
                nAnko++;
			case FORM_SHUNTSU:
				curPos += 3;
				break;
			case FORM_TOITSU:
                if ((agari == AGARI_TSUMO) && (hairi[curPos] == (te[IND_TSUMOHAI] & MASK_HAI_NUMBER)) && (nToitsu != 1))
                        nAnko++;
			case FORM_RYANMEN:
			case FORM_KANCHAN:
				curPos += 2;
				break;
			}
			if (form[i] == FORM_TANKI)
				break;
		}
		for (i = 0; i < nNaki; i++)
		{
			if ((nakiType[i] == NAKI_ANKAN) || (nakiType[i] == NAKI_DAIMINKAN) || (nakiType[i] == NAKI_KAKAN))
			{
                nKantsu++;
				if (nakiType[i] == NAKI_ANKAN)
                    nAnko++;
			}
		}

		if (nAnko == 4)
			yakuman |= FLAG_YAKUMAN_SUANKO;

		if (nKantsu == 4)
			yakuman |= FLAG_YAKUMAN_SUKANTSU;
	}

	// daisangen & shousushi & daisushi
	{
		int nHaku, nHatsu, nChun, nTon, nNan, nSha, nPe;
		nHaku = nHatsu = nChun = nTon = nNan = nSha = nPe = 0;

		for (i = 0; i <= IND_TSUMOHAI; i++)
		{
			nHaku += (tTe[i] == HAI_HAKU);
			nHatsu += (tTe[i] == HAI_HATSU);
			nChun += (tTe[i] == HAI_CHUN);
			nTon += (tTe[i] == HAI_TON);
			nNan += (tTe[i] == HAI_NAN);
			nSha += (tTe[i] == HAI_SHA);
			nPe += (tTe[i] == HAI_PE);
		}

		if ((nHaku == 3) && (nHatsu == 3) && (nChun == 3))
			yakuman |= FLAG_YAKUMAN_DAISANGEN;

		if ((nTon == 3) && (nNan == 3) && (nSha == 3) && (nPe == 3))
			yakuman |= FLAG_YAKUMAN_DAISUSHI;

		if (((nTon == 2) && (nNan == 3) && (nSha == 3) && (nPe == 3)) || \
			((nTon == 3) && (nNan == 2) && (nSha == 3) && (nPe == 3)) || \
			((nTon == 3) && (nNan == 3) && (nSha == 2) && (nPe == 3)) || \
			((nTon == 3) && (nNan == 3) && (nSha == 3) && (nPe == 2)))
			yakuman |= FLAG_YAKUMAN_SHOUSUSHI;
	}

	// tsuiso
	if ((tTe[0] >= HAI_TON) && (tTe[0] <= HAI_CHUN))
		yakuman |= FLAG_YAKUMAN_TSUISO;

	// ryuiso (23468s,hatsu)
	skip = 0;
	for (i = 0; i <= IND_TSUMOHAI; i++)
		if ((tTe[i] != HAI_2S) && (tTe[i] != HAI_3S) && (tTe[i] != HAI_4S) && (tTe[i] != HAI_6S) && (tTe[i] != HAI_8S) && (tTe[i] != HAI_HATSU))
		{
			skip = 1;
			break;
		}
	if (!skip)
		yakuman |= FLAG_YAKUMAN_RYUISO;

	// chinroto
	skip = 0;
	for (i = 0; i <= IND_TSUMOHAI; i++)
		if ((tTe[i] != HAI_1M) && (tTe[i] != HAI_9M) && (tTe[i] != HAI_1P) && (tTe[i] != HAI_9P) && (tTe[i] != HAI_1S) && (tTe[i] != HAI_9S))
		{
			skip = 1;
			break;
		}
	if (!skip)
		yakuman |= FLAG_YAKUMAN_CHINROTO;		

	return yakuman;
}


int GameThread::calcTeyaku(const int* te, const int* hairi, int agari, int chanfonpai, int menfonpai)
// get hairi from outsite (only 1 entry)
// returns int containing flags of teyaku's
// actual # of fans will be calculated in a separate function (inc. akadora)
{
    int yaku, nNaki, i, j, temp, skip, tTe[IND_TESIZE], atamaHai;
	const int *form;
	int nakiType[4], nakiHai[4][3];

	yaku = 0x00000000;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    //nHai = 13 - 3 * nNaki;
	form = hairi + 14;

	for (i = 0; i <= IND_TSUMOHAI; i++)
		tTe[i] = (te[i] & MASK_HAI_NUMBER);

	for (i = 0; i < nNaki; i++)
	{
		nakiType[i] = ((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07);
		nakiHai[i][0] = tTe[10 - (3 * i) + 0];
		nakiHai[i][1] = tTe[10 - (3 * i) + 1];
		nakiHai[i][2] = tTe[10 - (3 * i) + 2];

		// sort
		if (nakiHai[i][1] < nakiHai[i][0])
		{
			temp = nakiHai[i][0];
			nakiHai[i][0] = nakiHai[i][1];
			nakiHai[i][1] = temp;
		}

		if (nakiHai[i][2] < nakiHai[i][0])
		{
			temp = nakiHai[i][0];
			nakiHai[i][0] = nakiHai[i][2];
			nakiHai[i][2] = temp;
		}

		if (nakiHai[i][2] < nakiHai[i][1])
		{
			temp = nakiHai[i][1];
			nakiHai[i][1] = nakiHai[i][2];
			nakiHai[i][2] = temp;
		}
	}

	// sort (convenient for certain yaku's)
	sortHai(tTe, 14);


	// tanyao
	skip = 0;
	for (i = 0; i <= IND_TSUMOHAI; i++)
	{
		temp = tTe[i];
		if ((temp == HAI_1M) || (temp == HAI_9M) || (temp == HAI_1P) || (temp == HAI_9P) || (temp == HAI_1S) || (temp == HAI_9S) || \
			((temp >= HAI_TON) && (temp <= HAI_CHUN)))
		{
			skip = 1;
			break;
		}
	}
	if (!skip)
		yaku |= FLAG_YAKU_TANYAO;

	// pinfu
	// 3/3/3/toi/2/1
	atamaHai = hairi[9];
	if ((nNaki == 0) && \
		(form[0] == FORM_SHUNTSU) && (form[1] == FORM_SHUNTSU) && (form[2] == FORM_SHUNTSU) && (form[3] == FORM_TOITSU) && \
		(atamaHai != chanfonpai) && (atamaHai != menfonpai) && (atamaHai != HAI_HAKU) && (atamaHai != HAI_HATSU) && (atamaHai != HAI_CHUN) && \
		(form[4] == FORM_RYANMEN) && (hairi[11] % 9 != 0) && (hairi[11] % 9 != 7))
		yaku |= FLAG_YAKU_PINFU;

	// ipeko & ryanpeko (menzen only)
	// 3/3/3/toi/2/1 ipeko & ryanpeko 
	// 3/3/3/3/1/1 ryanpeko 
	if (nNaki == 0)
	{
		int nShuntsu, curPos, minHai, nPeko;

		nShuntsu = 0;
		curPos = 0;
		minHai = HAI_EMPTY;
		for (i = 0; i < 6; i++)
		{
			switch (form[i])
			{
			case FORM_SHUNTSU:
				nShuntsu++;
			case FORM_KOTSU:
				curPos += 3;
				break;
			case FORM_RYANMEN:
			case FORM_KANCHAN:
				minHai = hairi[curPos];
				if (hairi[curPos + 1] < minHai)
					minHai = hairi[curPos + 1];
				if ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) < minHai)
					minHai = (te[IND_TSUMOHAI] & MASK_HAI_NUMBER);
			case FORM_TOITSU:
				curPos += 2;
				break;
			}
			if (form[i] == FORM_TANKI)
				break;
		}
		nPeko = 0;
		skip = 0;
		if (minHai != HAI_EMPTY)
		{
			for (i = 0; i < nShuntsu; i++)
				if (hairi[3 * i] == minHai)
					nPeko++;

			if (nPeko == 2) // 22233344+4 case
			{
				nPeko = 1;
				skip = 1;
			}

			if (nPeko == 3) // 22223333444+4 case
			{
				nPeko = 2;
				skip = 1;
			}
		}
		if (!skip)
		{
			for (i = 0; i < nShuntsu - 1; i++)
				if (hairi[3 * i] == hairi[3 * (i + 1)])
				{
					nPeko++;
					i++;
				}
		}
		if (nPeko == 1)
			yaku |= FLAG_YAKU_IPEKO;
		else if (nPeko == 2)
			yaku |= FLAG_YAKU_RYANPEKO;
	}

	// yakuhai (haku, hatsu, chun, chanfonpai, menfonpai)
	for (i = 0; i <= IND_TSUMOHAI - 2; i++)
	{
		if ((tTe[i] == HAI_HAKU) || (tTe[i] == HAI_HATSU) || (tTe[i] == HAI_CHUN) || (tTe[i] == chanfonpai) || (tTe[i] == menfonpai))
		{
			if ((tTe[i] == tTe[i + 1]) && (tTe[i + 1] == tTe[i + 2]))
			{
				temp = tTe[i];
				switch (temp)
				{
				case HAI_HAKU:
					yaku |= FLAG_YAKU_HAKU;
					break;
				case HAI_HATSU:
					yaku |= FLAG_YAKU_HATSU;
					break;
				case HAI_CHUN:
					yaku |= FLAG_YAKU_CHUN;
					break;
				}
				if (temp == chanfonpai)
					yaku |= FLAG_YAKU_CHANFONPAI;
				if (temp == menfonpai)
					yaku |= FLAG_YAKU_MENFONPAI;

				i += 2;
			}
		}
	}
	
	// sanshoku & ittsu & chanta & junchan
	{
		int nShuntsu, curPos, minHai, shuntsu[4], temp;
		int nOthers, others[5], jihai;

		nShuntsu = nOthers = 0;
		curPos = 0;
		for (i = 0; i < 6; i++)
		{
			switch (form[i])
			{
			case FORM_SHUNTSU:
				shuntsu[nShuntsu++] = hairi[curPos];
				curPos += 3;
				break;
			case FORM_KOTSU:
				others[nOthers++] = hairi[curPos];
				curPos += 3;
				break;
			case FORM_RYANMEN:
			case FORM_KANCHAN:
				minHai = hairi[curPos];
				if (hairi[curPos + 1] < minHai)
					minHai = hairi[curPos + 1];
				if ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) < minHai)
					minHai = (te[IND_TSUMOHAI] & MASK_HAI_NUMBER);
				shuntsu[nShuntsu++] = minHai;
				curPos += 2;
				break;
			case FORM_TOITSU:
				others[nOthers++] = hairi[curPos];
				curPos += 2;
				break;
			}
			if (form[i] == FORM_TANKI)
			{
				if (hairi[curPos] != HAI_EMPTY)
					others[nOthers++] = hairi[curPos];
				break;
			}
		}
		for (i = 0; i < nNaki; i++)
			if (nakiType[i] == NAKI_CHI)
				shuntsu[nShuntsu++] = nakiHai[i][0];
			else
				others[nOthers++] = nakiHai[i][0];

		if (nShuntsu)
		{
			sortHai(shuntsu, nShuntsu);

			// ittsu
			for (i = 0; i < nShuntsu - 2; i++)
			{
				if ((shuntsu[i + 0] / 9 == shuntsu[i + 1] / 9) && (shuntsu[i + 1] / 9 == shuntsu[i + 2] / 9) && \
					(shuntsu[i + 0] % 9 == 0) && (shuntsu[i + 1] % 9 == 3) && (shuntsu[i + 2] % 9 == 6))
				{
					yaku |= FLAG_YAKU_ITTSU;
					break;
				}
			}

			// chanta & junchan
			skip = 0;
			for (i = 0; i < nShuntsu; i++)
				if ((shuntsu[i] % 9 != 0) && (shuntsu[i] % 9 != 6))
				{
					skip = 1;
					break;
				}
			if(!skip)
			{
				skip = 0;
				jihai = 0;
				for (i = 0; i < nOthers; i++)
				{
					if (((others[i] >= HAI_2M) && (others[i] <= HAI_8M)) || ((others[i] >= HAI_2P) && (others[i] <= HAI_8P)) || ((others[i] >= HAI_2S) && (others[i] <= HAI_8S)))
					{
						skip = 1;
						break;
					}
					if (others[i] >= HAI_TON)
						jihai = 1;
				}
                if (!skip) {
					if (jihai)
						yaku |= FLAG_YAKU_CHANTA;
					else
						yaku |= FLAG_YAKU_JUNCHAN;
                }
			}

			// sanshoku
			for (i = 0; i < nShuntsu - 1; i++)
				if (shuntsu[i] / 9 == shuntsu[i + 1] / 9)
				{
					temp = HAI_EMPTY;
					for (j = 1; j < nShuntsu - 1; j++)
					{
						if (shuntsu[(i + 1 + j) % nShuntsu] / 9 != shuntsu[i + 1] / 9)
						{
							temp = shuntsu[(i + 1 + j) % nShuntsu];
							break;
						}
					}

					if ((temp != HAI_EMPTY) && (temp % 9 != shuntsu[i] % 9))
						for (j = i + 1; j < nShuntsu; j++)
							shuntsu[j - 1] = shuntsu[j];
					else
						for (j = i + 2; j < nShuntsu; j++)
							shuntsu[j - 1] = shuntsu[j];
					nShuntsu--;
					break;
				}
			if ((nShuntsu == 3) && (shuntsu[0] / 9 == 0) && (shuntsu[1] / 9 == 1) && (shuntsu[2] / 9 == 2) && \
				(shuntsu[0] % 9 == shuntsu[1] % 9) && (shuntsu[1] % 9 == shuntsu[2] % 9))
				yaku |= FLAG_YAKU_SANSHOKU;
		}
	}

	// toitoi & sananko & sankantsu & sandoko & honroto
	{
        int curPos, nKotsu, kotsu[4], nAnko, nKantsu, nShupai;

        int nToitsu = 0;
        for (i = 0; i < 6; i++) {
            if (form[i] == FORM_TANKI)
                break;
            if (form[i] == FORM_TOITSU)
                nToitsu++;
        }

		nKotsu = nAnko = nKantsu = 0;
		curPos = 0;
		for (i = 0; i < 6; i++)
		{
			switch (form[i])
			{
			case FORM_KOTSU:
				kotsu[nKotsu++] = hairi[curPos];
                nAnko++;
			case FORM_SHUNTSU:
				curPos += 3;
				break;
			case FORM_TOITSU:
                if ((hairi[curPos] == (te[IND_TSUMOHAI] & MASK_HAI_NUMBER)) && (nToitsu != 1))
				{
					kotsu[nKotsu++] = hairi[curPos];
					if (agari == AGARI_TSUMO)
                        nAnko++;
				}
			case FORM_RYANMEN:
			case FORM_KANCHAN:
				curPos += 2;
				break;
			}
			if (form[i] == FORM_TANKI)
				break;
		}
		for (i = 0; i < nNaki; i++)
		{
			if (nakiType[i] == NAKI_PON)
				kotsu[nKotsu++] = nakiHai[i][0];
			else if ((nakiType[i] == NAKI_ANKAN) || (nakiType[i] == NAKI_DAIMINKAN) || (nakiType[i] == NAKI_KAKAN))
			{
				kotsu[nKotsu++] = nakiHai[i][0];
                nKantsu++;
				if (nakiType[i] == NAKI_ANKAN)
                    nAnko++;
			}
		}

		if (nKotsu == 4)
		{
			yaku |= FLAG_YAKU_TOITOI;

			// honroto & toitoi
			skip = 0;
			for (i = 0; i <= IND_TSUMOHAI; i++)
				if (((tTe[i] >= HAI_2M) && (tTe[i] <= HAI_8M)) || ((tTe[i] >= HAI_2P) && (tTe[i] <= HAI_8P)) || ((tTe[i] >= HAI_2S) && (tTe[i] <= HAI_8S)))
				{
					skip = 1;
					break;
				}
			if (!skip)
				yaku |= FLAG_YAKU_HONROTO;
		}

		if (nAnko == 3)
			yaku |= FLAG_YAKU_SANANKO;

		if (nKantsu == 3)
			yaku |= FLAG_YAKU_SANKANTSU;

		// sandoko
		if (nKotsu)
		{
			sortHai(kotsu, nKotsu);

			nShupai = 0;
			for (i = 0; i < nKotsu; i++)
				if (kotsu[i] <= HAI_9S)
					nShupai++;

			if (nShupai >= 3)
			{
				for (i = 0; i < nShupai - 1; i++)
					if (kotsu[i] / 9 == kotsu[i + 1] / 9)
					{
						temp = HAI_EMPTY;
						for (j = 1; j < nShupai - 1; j++)
						{
							if (kotsu[(i + 1 + j) % nShupai] / 9 != kotsu[i + 1] / 9)
							{
								temp = kotsu[(i + 1 + j) % nShupai];
								break;
							}
						}

						if ((temp != HAI_EMPTY) && (temp % 9 != kotsu[i] % 9))
							for (j = i + 1; j < nShupai; j++)
								kotsu[j - 1] = kotsu[j];
						else
							for (j = i + 2; j < nShupai; j++)
								kotsu[j - 1] = kotsu[j];
						nShupai--;
						break;
					}
				if ((nShupai == 3) && (kotsu[0] / 9 == 0) && (kotsu[1] / 9 == 1) && (kotsu[2] / 9 == 2) && \
					(kotsu[0] % 9 == kotsu[1] % 9) && (kotsu[1] % 9 == kotsu[2] % 9))
					yaku |= FLAG_YAKU_SANDOKO;
			}
		}

	}

	// chitoi (menzen only)
	if ((nNaki == 0) && (form[0] == FORM_TOITSU))
	{
		yaku |= FLAG_YAKU_CHITOI;

		// honroto & chitoi
		skip = 0;
		for (i = 0; i <= IND_TSUMOHAI; i++)
			if (((tTe[i] >= HAI_2M) && (tTe[i] <= HAI_8M)) || ((tTe[i] >= HAI_2P) && (tTe[i] <= HAI_8P)) || ((tTe[i] >= HAI_2S) && (tTe[i] <= HAI_8S)))
			{
				skip = 1;
				break;
			}
		if (!skip)
			yaku |= FLAG_YAKU_HONROTO;
	}

	// shousangen
	{
		int nHaku, nHatsu, nChun;
		nHaku = nHatsu = nChun = 0;

		for (i = 0; i <= IND_TSUMOHAI; i++)
		{
			nHaku += (tTe[i] == HAI_HAKU);
			nHatsu += (tTe[i] == HAI_HATSU);
			nChun += (tTe[i] == HAI_CHUN);
		}

		if (((nHaku == 3) && (nHatsu == 3) && (nChun == 2)) || \
			((nHaku == 3) && (nHatsu == 2) && (nChun == 3)) || \
			((nHaku == 2) && (nHatsu == 3) && (nChun == 3)))
			yaku |= FLAG_YAKU_SHOUSANGEN;
	}

	// honitsu
	if ((tTe[0] <= HAI_9S) && (tTe[IND_TSUMOHAI] >= HAI_TON))
	{
		temp = tTe[0] / 9;
		skip = 0;
		for (i = 0; i <= IND_TSUMOHAI; i++)
		{
			if ((tTe[i] <= HAI_9S) && (tTe[i] / 9 != temp))
			{
				skip = 1;
				break;
			}
		}
		if (!skip)
			yaku |= FLAG_YAKU_HONITSU;
	}

	// chinitsu
	if (tTe[IND_TSUMOHAI] <= HAI_9S)
	{
		temp = tTe[0] / 9;
		skip = 0;
		for (i = 0; i <= IND_TSUMOHAI; i++)
			if (tTe[i] / 9 != temp)
			{
				skip = 1;
				break;
			}
		if (!skip)
			yaku |= FLAG_YAKU_CHINITSU;
	}

	return yaku;
}

int GameThread::calcFan(const int* te, int yaku, const int* omotedora, const int* uradora, int isNotMenzen)
{
	int fan, i, j, naki;

	fan = 0;

	if (yaku)
	{
		for (i = 0; i <= IND_TSUMOHAI; i++)
			if (te[i] & FLAG_HAI_AKADORA)
				fan += 1;

		if (omotedora != NULL)
			for (i = 0; i < 5; i++)
			{
				if (omotedora[i] == HAI_EMPTY)
					break;
				for (j = 0; j <= IND_TSUMOHAI; j++)
					if ((te[j] & MASK_HAI_NUMBER) == omotedora[i])
						fan += 1;
				for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
				{
					naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
					if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
						((te[10 - 3 * j] & MASK_HAI_NUMBER) == omotedora[i]))
						fan += 1;
				}
			}

		if ((uradora != NULL) && (te[IND_NAKISTATE] & FLAG_TE_RICHI))
			for (i = 0; i < 5; i++)
			{
				if (uradora[i] == HAI_EMPTY)
					break;
				for (j = 0; j <= IND_TSUMOHAI; j++)
					if ((te[j] & MASK_HAI_NUMBER) == uradora[i])
						fan += 1;
				for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
				{
					naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
					if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
						((te[10 - 3 * j] & MASK_HAI_NUMBER) == uradora[i]))
						fan += 1;
				}
			}

		if (yaku & FLAG_YAKU_RICHI)
			fan += 1;

		if (yaku & FLAG_YAKU_DOUBLERICHI)
			fan += 2;

		if (yaku & FLAG_YAKU_IPPATSU)
			fan += 1;

		if (yaku & FLAG_YAKU_MENZENTSUMO)
			fan += 1;

		if (yaku & FLAG_YAKU_TANYAO)
			fan += 1;

		if (yaku & FLAG_YAKU_PINFU)
			fan += 1;

		if (yaku & FLAG_YAKU_IPEKO)
			fan += 1;

		if (yaku & FLAG_YAKU_RYANPEKO)
			fan += 3;

		if (yaku & FLAG_YAKU_SHOUSANGEN)
			fan += 2;

		if (yaku & FLAG_YAKU_HAKU)
			fan += 1;

		if (yaku & FLAG_YAKU_HATSU)
			fan += 1;

		if (yaku & FLAG_YAKU_CHUN)
			fan += 1;

		if (yaku & FLAG_YAKU_CHANFONPAI)
			fan += 1;

		if (yaku & FLAG_YAKU_MENFONPAI)
			fan += 1;

		if (yaku & FLAG_YAKU_RINSHAN)
			fan += 1;

		if (yaku & FLAG_YAKU_CHANKAN)
			fan += 1;

		if (yaku & FLAG_YAKU_HAITEI)
			fan += 1;

		if (yaku & FLAG_YAKU_HOUTEI)
			fan += 1;

        if (yaku & FLAG_YAKU_SANSHOKU) {
			if (isNotMenzen)
				fan += 1;
			else
				fan += 2;
        }

        if (yaku & FLAG_YAKU_ITTSU) {
			if (isNotMenzen)
				fan += 1;
			else
				fan += 2;
        }

        if (yaku & FLAG_YAKU_CHANTA) {
			if (isNotMenzen)
				fan += 1;
			else
				fan += 2;
        }

        if (yaku & FLAG_YAKU_JUNCHAN) {
			if (isNotMenzen)
				fan += 2;
			else
				fan += 3;
        }

		if (yaku & FLAG_YAKU_CHITOI)
			fan += 2;

		if (yaku & FLAG_YAKU_TOITOI)
			fan += 2;

		if (yaku & FLAG_YAKU_SANANKO)
			fan += 2;

		if (yaku & FLAG_YAKU_HONROTO)
			fan += 2;

		if (yaku & FLAG_YAKU_SANDOKO)
			fan += 2;

		if (yaku & FLAG_YAKU_SANKANTSU)
			fan += 2;

        if (yaku & FLAG_YAKU_HONITSU) {
			if (isNotMenzen)
				fan += 2;
			else
				fan += 3;
        }

        if (yaku & FLAG_YAKU_CHINITSU) {
			if (isNotMenzen)
				fan += 5;
			else
				fan += 6;
        }
	}

	return fan;
}

int GameThread::printYaku(const int* te, int yaku, int yakuman, const int* omotedora, const int* uradora, int isNotMenzen)
{
	int i, j, nDora, naki;

	if (yakuman == 0x00000000)
	{
		if (yaku & FLAG_YAKU_RICHI)
			printf("richi\t\t1 fan\n");

		if (yaku & FLAG_YAKU_DOUBLERICHI)
			printf("double richi\t2 fan\n");

		if (yaku & FLAG_YAKU_IPPATSU)
			printf("ippatsu\t\t1 fan\n");

		if (yaku & FLAG_YAKU_MENZENTSUMO)
			printf("menzen tsumo\t1 fan\n");

		if (yaku & FLAG_YAKU_TANYAO)
			printf("tanyao\t\t1 fan\n");

		if (yaku & FLAG_YAKU_PINFU)
			printf("pinfu\t\t1 fan\n");

		if (yaku & FLAG_YAKU_IPEKO)
			printf("ipeko\t\t1 fan\n");

		if (yaku & FLAG_YAKU_RYANPEKO)
			printf("ryanpeko\t3 fan\n");

		if (yaku & FLAG_YAKU_SHOUSANGEN)
			printf("shousangen\t2 fan\n");

		if (yaku & FLAG_YAKU_HAKU)
			printf("yakuhai: haku\t1 fan\n");

		if (yaku & FLAG_YAKU_HATSU)
			printf("yakuhai: hatsu\t1 fan\n");

		if (yaku & FLAG_YAKU_CHUN)
			printf("yakuhai: chun\t1 fan\n");

		if (yaku & FLAG_YAKU_CHANFONPAI)
			printf("chanfonpai\t1 fan\n");

		if (yaku & FLAG_YAKU_MENFONPAI)
			printf("menfonpai\t1 fan\n");

		if (yaku & FLAG_YAKU_RINSHAN)
			printf("rinshan\t\t1 fan\n");

		if (yaku & FLAG_YAKU_CHANKAN)
			printf("chankan\t\t1 fan\n");

		if (yaku & FLAG_YAKU_HAITEI)
			printf("haitei\t\t1 fan\n");

		if (yaku & FLAG_YAKU_HOUTEI)
			printf("houtei\t\t1 fan\n");

        if (yaku & FLAG_YAKU_SANSHOKU) {
			if (isNotMenzen)
				printf("sanshoku\t1 fan\n");
			else
				printf("sanshoku\t2 fan\n");
        }

        if (yaku & FLAG_YAKU_ITTSU) {
			if (isNotMenzen)
				printf("ittsu\t\t1 fan\n");
			else
				printf("ittsu\t\t2 fan\n");
        }

        if (yaku & FLAG_YAKU_CHANTA) {
			if (isNotMenzen)
				printf("chanta\t\t1 fan\n");
			else
				printf("chanta\t\t2 fan\n");
        }

        if (yaku & FLAG_YAKU_JUNCHAN) {
			if (isNotMenzen)
				printf("junchan\t\t2 fan\n");
			else
				printf("junchan\t\t3 fan\n");
        }

		if (yaku & FLAG_YAKU_CHITOI)
			printf("chitoi\t\t2 fan\n");

		if (yaku & FLAG_YAKU_TOITOI)
			printf("toitoi\t\t2 fan\n");

		if (yaku & FLAG_YAKU_SANANKO)
			printf("sananko\t\t2 fan\n");

		if (yaku & FLAG_YAKU_HONROTO)
			printf("honroto\t\t2 fan\n");

		if (yaku & FLAG_YAKU_SANDOKO)
			printf("sandoko\t\t2 fan\n");

		if (yaku & FLAG_YAKU_SANKANTSU)
			printf("sankantsu\t2 fan\n");

        if (yaku & FLAG_YAKU_HONITSU) {
			if (isNotMenzen)
				printf("honitsu\t\t2 fan\n");
			else
				printf("honitsu\t\t3 fan\n");
        }

        if (yaku & FLAG_YAKU_CHINITSU) {
			if (isNotMenzen)
				printf("chinitsu\t5 fan\n");
			else
				printf("chinitsu\t6 fan\n");
        }

		nDora = 0;
		if (omotedora != NULL)
			for (i = 0; i < 5; i++)
			{
				if (omotedora[i] == HAI_EMPTY)
					break;
				for (j = 0; j <= IND_TSUMOHAI; j++)
					if ((te[j] & MASK_HAI_NUMBER) == omotedora[i])
						nDora += 1;
				for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
				{
					naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
					if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
						((te[10 - 3 * j] & MASK_HAI_NUMBER) == omotedora[i]))
						nDora += 1;
				}
			}
		if ((uradora != NULL) && (te[IND_NAKISTATE] & FLAG_TE_RICHI))
			for (i = 0; i < 5; i++)
			{
				if (uradora[i] == HAI_EMPTY)
					break;
				for (j = 0; j <= IND_TSUMOHAI; j++)
					if ((te[j] & MASK_HAI_NUMBER) == uradora[i])
						nDora += 1;
				for (j = 0; j < (te[IND_NAKISTATE] & MASK_TE_NNAKI); j++)
				{
					naki = (te[IND_NAKISTATE] >> (4 + 3 * j)) & 0x07;
					if (((naki == NAKI_ANKAN) || (naki == NAKI_DAIMINKAN) || (naki == NAKI_KAKAN)) && \
						((te[10 - 3 * j] & MASK_HAI_NUMBER) == uradora[i]))
						nDora += 1;
				}
			}
		for (i = 0; i <= IND_TSUMOHAI; i++)
			if (te[i] & FLAG_HAI_AKADORA)
				nDora += 1;
		if (nDora)
			printf("dora\t\t%d fan\n", nDora);

	}
	else // print yakumans
	{
		if (yakuman & FLAG_YAKUMAN_KOKUSHI)
			printf("kokushimusou\n");

		if (yakuman & FLAG_YAKUMAN_SUANKO)
			printf("suanko\n");

		if (yakuman & FLAG_YAKUMAN_DAISANGEN)
			printf("daisangen\n");

		if (yakuman & FLAG_YAKUMAN_TSUISO)
			printf("tsuiso\n");

		if (yakuman & FLAG_YAKUMAN_SHOUSUSHI)
			printf("shousushi\n");

		if (yakuman & FLAG_YAKUMAN_DAISUSHI)
			printf("daisushi\n");

		if (yakuman & FLAG_YAKUMAN_RYUISO)
			printf("ryuiso\n");

		if (yakuman & FLAG_YAKUMAN_CHINROTO)
			printf("chinroto\n");

		if (yakuman & FLAG_YAKUMAN_SUKANTSU)
			printf("sukantsu\n");

		if (yakuman & FLAG_YAKUMAN_CHURENPOTO)
			printf("churenpoto\n");

		if (yakuman & FLAG_YAKUMAN_TENHO)
			printf("tenho\n");

		if (yakuman & FLAG_YAKUMAN_CHIHO)
			printf("chiho\n");
	}

	return 0;
}

int GameThread::calcFu(const int* te, const int* hairi, int agari, int chanfonpai, int menfonpai)
// get hairi from outsite (only 1 entry)
// special cases: chitoi (25), tsumo-pinfu (20), kui-pinfu (30)
// hairi ordering: shuntsu -> kotsu -> toitsu -> ryanmen -> kanchan -> tanki 
// this function returns the raw fu without kiriage (kiriage will be done in the function that actually calculates ten)
{
    int nNaki, skip, i, temp, fu, tFu, subind, atamaHai;

#ifdef FLAG_DEBUG
    printTe(te);
    printf("\n");

    for (int i = 0; i < 6; i++) {
        switch (hairi[14 + i])
        {
        case FORM_SHUNTSU:
            printf("shuntsu ");
            break;
        case FORM_KOTSU:
            printf("kotsu ");
            break;
        case FORM_TOITSU:
            printf("toitsu ");
            break;
        case FORM_RYANMEN:
            printf("ryanmen ");
            break;
        case FORM_KANCHAN:
            printf("kanchan ");
            break;
        }
        if (hairi[14 + i] == FORM_TANKI)
            break;
    }
    printf("(concealed hais only)\n");

    if (agari == AGARI_KUIRON)
        printf("kui ron 0 fu\n");
    if (agari == AGARI_TSUMO)
        printf("tsumo 2 fu\n");
    if (agari == AGARI_MENZENRON)
        printf("menzen ron 10 fu\n");

    fflush(stdout);
#endif

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
    //nHai = 13 - 3 * nNaki;

	// chitoi (25 fu)
	if ((nNaki == 0) && \
		(hairi[14 + 0] == FORM_TOITSU) && (hairi[14 + 1] == FORM_TOITSU) && (hairi[14 + 2] == FORM_TOITSU))
		return 25;
	
	// tsumo-pinfu (20 fu)
	// nNaki == 0
	// shuntsu shuntsu shuntsu toitsu ryanmen
	// toitsu must not be sangenpai / chanfonpai / menfonpai
	if ((nNaki == 0) && (agari == AGARI_TSUMO) && \
		(hairi[14 + 0] == FORM_SHUNTSU) && (hairi[14 + 1] == FORM_SHUNTSU) && (hairi[14 + 2] == FORM_SHUNTSU) && (hairi[14 + 3] == FORM_TOITSU) && \
		(hairi[9] != chanfonpai) && (hairi[9] != menfonpai) && (hairi[9] != HAI_HAKU) && (hairi[9] != HAI_HATSU) && (hairi[9] != HAI_CHUN) && \
		(hairi[14 + 4] == FORM_RYANMEN) && (hairi[11] % 9 != 0) && (hairi[11] % 9 != 7))
		return 20;

	// kui-pinfu (30 fu)
	// all naki's are chis (1 <= nNaki <= 3)
	// shuntsu x(3 - nNaki) toitsu ryanmen
	// toitsu must not be sangenpai / chanfonpai / menfonpai
	if ((nNaki >= 1) && (nNaki <= 3))
	{
		skip = 0;
		for (i = 0; i < nNaki; i++)
			if (((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07) != NAKI_CHI)
			{
				skip = 1;
				break;
			}

		if (!skip)
		{
			skip = 0;
			for (i = 0; i < 3 - nNaki; i++)
				if (hairi[14 + i] != FORM_SHUNTSU)
				{
					skip = 1;
					break;
				}

			if (!skip)
			{
				atamaHai = hairi[3 * (3 - nNaki)]; // toitsu hai

				if ((hairi[14 + (3 - nNaki) + 0] == FORM_TOITSU) && \
					(atamaHai != chanfonpai) && (atamaHai != menfonpai) && (atamaHai != HAI_HAKU) && (atamaHai != HAI_HATSU) && (atamaHai != HAI_CHUN) && \
					(hairi[14 + (3 - nNaki) + 1] == FORM_RYANMEN) && (hairi[3 * (3 - nNaki) + 2] % 9 != 0) && (hairi[3 * (3 - nNaki) + 2] % 9 != 7))
					return 30;
			}
		}
	}

	// general form
	// 3/3/3/3/1/1 tanki
	// 3/3/3/toi/2/1 shanpon/ryanmen/penchan/kanchan
	fu = 20 + agari;
	for (i = 0; i < nNaki; i++)
	{
		tFu = 0;
		switch ((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07)
		{
		case NAKI_PON: // minko
#ifdef FLAG_DEBUG
            printf("pon ");
#endif
			tFu = 2;
			break;
		case NAKI_DAIMINKAN:
		case NAKI_KAKAN:
#ifdef FLAG_DEBUG
            printf("kan ");
#endif
			tFu = 8;
			break;
		case NAKI_ANKAN:
#ifdef FLAG_DEBUG
            printf("kan ");
#endif
			tFu = 16;
			break;
		}
		temp = te[10 - 3 * i] & MASK_HAI_NUMBER;
		if (tFu && ((temp == HAI_1M) || (temp == HAI_9M) || (temp == HAI_1P) || (temp == HAI_9P) || \
			(temp == HAI_1S) || (temp == HAI_9S) || ((temp >= HAI_TON) && (temp <= HAI_CHUN))))
			tFu <<= 1;
		fu += tFu;
#ifdef FLAG_DEBUG
        if (tFu > 0)
            printf("%d fu\n", tFu);
#endif
	}

	for (i = 0; i < 4; i++)
	{
        if (hairi[14 + i] < FORM_KOTSU)
			break;

		if (hairi[14 + i] == FORM_KOTSU) // anko
		{
			tFu = 4;
			temp = hairi[3 * i];
			if ((temp == HAI_1M) || (temp == HAI_9M) || (temp == HAI_1P) || (temp == HAI_9P) || \
				(temp == HAI_1S) || (temp == HAI_9S) || ((temp >= HAI_TON) && (temp <= HAI_CHUN)))
				tFu <<= 1;
			fu += tFu;
#ifdef FLAG_DEBUG
            printf("anko %d fu\n", tFu);
#endif
		}
	}

	if (hairi[14 + (3 - nNaki) + 0] == FORM_TOITSU)
	// 3/3/3/toi/2/1 shanpon/ryanmen/penchan/kanchan
	{
        int isTsumo = (agari == AGARI_TSUMO);

		switch (hairi[14 + (3 - nNaki) + 1])
		{
		case FORM_KANCHAN:
#ifdef FLAG_DEBUG
            printf("kanchan 2 fu\n");
#endif
			fu += 2;
            atamaHai = hairi[3 * (3 - nNaki)];
            break;

		case FORM_RYANMEN: 
            if ((hairi[3 * (3 - nNaki) + 2 + 0] % 9 == 0)  || (hairi[3 * (3 - nNaki) + 2 + 0] % 9 == 7)) {
				fu += 2; // penchan
#ifdef FLAG_DEBUG
                printf("penchan 2 fu\n");
#endif
            }
			atamaHai = hairi[3 * (3 - nNaki)];
			break;

		case FORM_TOITSU: // shanpon
			if ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) == hairi[3 * (3 - nNaki) + 0])
				subind = 0;
			else
				subind = 1;

			atamaHai = hairi[3 * (3 - nNaki) + 2 * (!subind)]; 

			// extra kotsu (treat as tsumo--anko, ron--minko)
			tFu = (2 << isTsumo);
#ifdef FLAG_DEBUG
            if (tFu == 2)
                printf("minko ");
            else
                printf("anko ");
#endif
			temp = hairi[3 * (3 - nNaki) + 2 * (subind)];
			if ((temp == HAI_1M) || (temp == HAI_9M) || (temp == HAI_1P) || (temp == HAI_9P) || \
				(temp == HAI_1S) || (temp == HAI_9S) || ((temp >= HAI_TON) && (temp <= HAI_CHUN)))
				tFu <<= 1;
			fu += tFu;
#ifdef FLAG_DEBUG
            printf("%d fu\n", tFu);
#endif
			break;
        default: // this should not happen
            atamaHai = HAI_EMPTY;
            break;
		}
	}
	else // 3/3/3/3/1/1 tanki
	{
		fu += 2; // tanki machi

#ifdef FLAG_DEBUG
        printf("tanki 2 fu\n");
#endif

		atamaHai = hairi[3 * (3 - nNaki + 1)];
	}

    if ((atamaHai == HAI_HAKU) || (atamaHai == HAI_HATSU) || (atamaHai == HAI_CHUN) || (atamaHai == chanfonpai)) {
		fu += 2;
#ifdef FLAG_DEBUG
        printf("jantou 2 fu\n");
#endif
    }
    if (atamaHai == menfonpai) {
		fu += 2; // separeted so that renfonpai becomes 4 fu
#ifdef FLAG_DEBUG
        printf("jantou 2 fu\n");
#endif
    }

#ifdef FLAG_DEBUG
    fflush(stdout);
#endif

	return fu;
}

int GameThread::askRichi(int* te, int optionChosen)
{
	// Can assume:
	// already tenpai
	// there is something in IND_TSUMOHAI
	// no naki

	int tTe[15], i, inds[14 + 1], nOptions, machi[IND_MACHISIZE];

	nOptions = 0;
	copyTeN(tTe, te, 13);
	tTe[IND_TSUMOHAI] = HAI_EMPTY;
	tTe[IND_NAKISTATE] = 0;

	// try throwing away one hai from 0~IND_TSUMOHAI and see if tenpai
	for (i = 0; i <= IND_TSUMOHAI; i++)
	{
        //if ((nOptions > 0) && (tTe[i] == te[inds[nOptions - 1]]))
        //	continue;

		tTe[i] = te[IND_TSUMOHAI];

		// check tenpai
		shanten(tTe, machi, NULL);

		if (machi[IND_NSHANTEN] == 0)
			inds[nOptions++] = i;

		tTe[i] = te[i];
	}
	inds[nOptions] = -1;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_RICHI, inds);
	}
	else
	{
		tTe[0] = te[inds[optionChosen]];
		te[inds[optionChosen]] = te[IND_TSUMOHAI];
		te[IND_TSUMOHAI] = tTe[0];

		te[IND_TSUMOHAI] |= FLAG_HAI_RICHIHAI;

        if (inds[optionChosen] != IND_TSUMOHAI)
            te[IND_TSUMOHAI] |= FLAG_HAI_TSUMOGIRI; // this is intentionally reversed -- see askDahai function for why

		te[IND_NAKISTATE] |= FLAG_TE_RICHI;
	}

	return 0;
}

int GameThread::askChi(int* te, int sutehai, int optionChosen)
{
	int nNaki, nHai, nOptions, num, tNum, hai, temp;
	int inds[5], i, j, optionInds[10 + 1], hai0, hai1;

	// shupai only
	// assuming sutehai == i
	// 34 46 67
	// if aka is in 3 or 7 && more than one 3 or 7: 34 3*4 / 67 67*
	// if aka is in 4 or 6 && more than one 4 or 6: 34 34* 46 4*6 / 46 46* 67 6*7
	// inds[0]: i - 2
	// inds[1]: i - 1
	// inds[2]: akadora, if any
	// inds[3]: i + 1
	// inds[4]: i + 2
	// there can be two akadoras!
	

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	if (nNaki == 4)
		return 0;

	num = (sutehai & MASK_HAI_NUMBER) % 9; // 0 ~ 8
	inds[0] = inds[1] = inds[2] = inds[3] = inds[4] = -1; // default for not found

	if ((num > 1) && (num < 7) && (num != 4)) // 23 56
		for (i = 0; i < nHai; i++)
			if (((te[i] & MASK_HAI_NUMBER) / 9 == (sutehai & MASK_HAI_NUMBER) / 9) && (te[i] & FLAG_HAI_AKADORA))
			{
				inds[2] = i;
				inds[4 - num + 2] = i;
				break;
			}

	for (i = 0; i < 5; i++)
	{
		tNum = num - 2 + i;
		if ((tNum < 0) || (tNum > 8) || (i == 2))
			continue;
		hai = (sutehai & MASK_HAI_NUMBER) + tNum - num;
		for (j = 0; j < nHai; j++)
			if (((te[j] & MASK_HAI_NUMBER) == hai) && !(te[j] & FLAG_HAI_AKADORA))
			{
				inds[i] = j;
				break;
			}
	}

	// shift inds so that it now becomes
	// inds[0]: i - 2
	// inds[1]: i - 1
	// inds[2]: i + 1
	// inds[3]: i + 2
	// inds[4]: akadora, if any
	temp = inds[2];
	inds[2] = inds[3];
	inds[3] = inds[4];
	inds[4] = temp;

	nOptions = 0;
	// optionInds are grouped into 2, max 5 entries
	for (i = 0; i < 3; i++)
	{
		if ((inds[i] != -1) && (inds[i + 1] != -1))
		{
			optionInds[2 * nOptions + 0] = inds[i];
			optionInds[2 * nOptions + 1] = inds[i + 1];
			nOptions++;

			if (((te[inds[i]] & MASK_HAI_NUMBER) % 9 == 4) && \
				((te[inds[i]] & FLAG_HAI_AKADORA) == 0) && \
				(inds[4] != -1))
			{
				optionInds[2 * nOptions + 0] = inds[4];
				optionInds[2 * nOptions + 1] = inds[i + 1];
				nOptions++;
			}

			if (((te[inds[i + 1]] & MASK_HAI_NUMBER) % 9 == 4) && \
				((te[inds[i + 1]] & FLAG_HAI_AKADORA) == 0) && \
				(inds[4] != -1))
			{
				optionInds[2 * nOptions + 0] = inds[i];
				optionInds[2 * nOptions + 1] = inds[4];
				nOptions++;
			}
		}
	}
	optionInds[nOptions << 1] = -1;

	if (nOptions == 0)
		return 0;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[nakiDir]]->registerAction(ACTION_CHI, optionInds);
		return 0;
	}
	else
	{
		// process chi

		// back up option1 hais and replace with HAI_EMPTY
		hai0 = te[optionInds[2 * optionChosen + 0]];
		hai1 = te[optionInds[2 * optionChosen + 1]];
		te[optionInds[2 * optionChosen + 0]] = HAI_EMPTY;
		te[optionInds[2 * optionChosen + 1]] = HAI_EMPTY;

		// put last non-empty hai in IND_TSUMOHAI
		for (i = nHai - 1; i >= 0; i--)
			if (te[i] != HAI_EMPTY)
			{
				te[IND_TSUMOHAI] = te[i];
				te[i] = HAI_EMPTY;
				break;
			}

		// shift to fill space (but keep order)
		for (i = 0; i < nHai - 3; i++)
			if (te[i] == HAI_EMPTY)
				for (j = i + 1; j < nHai; j++)
					if (te[j] != HAI_EMPTY)
					{
						te[i] = te[j];
						te[j] = HAI_EMPTY;
						break;
					}

		// put back pon hais in right indices
		te[nHai - 3 + 0] = sutehai | FLAG_HAI_NOT_TSUMOHAI;
		te[nHai - 3 + 1] = hai0;
		te[nHai - 3 + 2] = hai1;

		// fill in te[IND_NAKISTATE] (3/3/3/3/1/3 bit) shift is (4 + 3 * nNaki) 
		te[IND_NAKISTATE]++;
		te[IND_NAKISTATE] |= FLAG_TE_NOT_MENZEN;
		te[IND_NAKISTATE] |= NAKI_CHI << (4 + 3 * nNaki);

		return 0;
	}

	return 0;
}

int GameThread::askRonho(int* te, int optionChosen)
{
	// check yaku
	if (!te[IND_YAKUFLAGS] && !te[IND_YAKUMANFLAGS]) // no yaku
		return 0;

	if (optionChosen == ACTION_REGISTER)
		player[dirToPlayer[nakiDir]]->registerAction(ACTION_RONHO, NULL);

	return 0;
}

int GameThread::askTsumoho(int* te, int optionChosen)
{
	// check yaku
	if (!te[IND_YAKUFLAGS] && !te[IND_YAKUMANFLAGS]) // no yaku
		return 0;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_TSUMOHO, NULL);
	}

	return 0;
}

int GameThread::updateScreen(int* yama, int* kawa, int* te, int nKan, int prevNaki) // display game info
{
	int i;

	//system("cls");
	printf("\n\n------------------------\n");

	printWanpai(yama, nKan, prevNaki);

	printf("\n%d hais left\n\n", yama[IND_NYAMA] - 14 - nKan);
	printf("te / pao state / kawa:\n");

	for (i = 0; i < 4; i++)
	{
		switch (i)
		{
		case 0:
			printf("me\t");
			break;
		case 1:
			printf("shimocha");
			break;
		case 2:
			printf("toimen\t");
			break;
		case 3:
			printf("kamicha\t");
			break;
		}
		sortHai(te + IND_TESIZE * i, 13 - 3 * (te[IND_TESIZE * i + IND_NAKISTATE] & MASK_TE_NNAKI));
		printTe(te + IND_TESIZE * i);
		printf(" / ");
		printf("0x%.1X / ", (te[IND_TESIZE * i + IND_YAKUMANFLAGS] >> 28) & 0x0F);
		printHai(kawa + IND_KAWASIZE * i, kawa[IND_KAWASIZE * i + IND_NKAWA]);
		printf("\n");
	}
	printf("\n");

	return 0;
}

int GameThread::getNKan(const int* te) // return ENDFLAG_SUKAIKAN to flag sukansanra (value is 5)
{
	int i, j, nakiState, nakiFlag, kan[4], nKan;
	
	// 3/3/3/3/1/3 shift: 4 + 3 * j

	for (i = 0; i < 4; i++)
	{
		kan[i] = 0;
		nakiState = te[IND_TESIZE * i + IND_NAKISTATE];
		for (j = 0; j < 4; j++)
		{
			nakiFlag = (nakiState >> (4 + 3 * j)) & 0x07;
			kan[i] += (nakiFlag == NAKI_ANKAN) || (nakiFlag == NAKI_DAIMINKAN) || (nakiFlag == NAKI_KAKAN);
		}
	}

	nKan = kan[0] + kan[1] + kan[2] + kan[3];

    if ((nKan >= 4) && (kan[0] != 4) && (kan[1] != 4) && (kan[2] != 4) && (kan[3] != 4))
		return ENDFLAG_SUKAIKAN; 
	else
		return nKan;
}

int GameThread::askAnkan(int* te, int optionChosen)
{
	int nNaki, nHai, i, j, k, hai[4], temp, skip;
	int tTe[IND_TESIZE], quadHai[3], nQuadHai, inds[12 + 1], nInd;
	int machi[IND_MACHISIZE], tMachi[IND_MACHISIZE];

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	if (nNaki == 4)
		return 0;

	// need to check if any hai is quadruple
	// there can be a maximum of 3 quadruples
	copyTeN(tTe, te, nHai);
	tTe[nHai] = te[IND_TSUMOHAI];
	sortHai(tTe, nHai + 1);

	nQuadHai = 0;
	for (i = 0; i <= nHai - 3; i++)
		if (((tTe[i + 0] & MASK_HAI_NUMBER) == (tTe[i + 1] & MASK_HAI_NUMBER)) && \
			((tTe[i + 1] & MASK_HAI_NUMBER) == (tTe[i + 2] & MASK_HAI_NUMBER)) && \
            ((tTe[i + 2] & MASK_HAI_NUMBER) == (tTe[i + 3] & MASK_HAI_NUMBER))) {
			quadHai[nQuadHai++] = tTe[i + 0] & MASK_HAI_NUMBER;
            i += 3;
        }

	for (i = 0; i < nQuadHai; i++)
	{
		nInd = 0;
		for (j = 0; j < nHai; j++)
            if ((te[j] & MASK_HAI_NUMBER) == quadHai[i])
				inds[4 * i + nInd++] = j;
		
        if ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) == quadHai[i])
			inds[4 * i + nInd++] = IND_TSUMOHAI;
	}

	// during richi, only ones that don't change machi survive 
	if (te[IND_NAKISTATE] & FLAG_TE_RICHI)
	{
		for (i = 0; i < nQuadHai; i++)
		{
			skip = 1;
			for (j = 0; j < 4; j++)
				if (inds[4 * i + j] == IND_TSUMOHAI)
				{
					skip = 0;
					break;
				}
			if (skip)
            {
				// shift all the rest inds by 4 indices and reduce nQuadHai by 1
				for (j = 4 * i + 4; j < 12; j++)
					inds[j - 4] = inds[j];
				nQuadHai--;
                i--;
                continue;
			}
				

			shanten(te, machi, NULL);
			copyTeN(tTe, te, nHai);
			for (j = 0; j < 4; j++)
				tTe[inds[4 * i + j]] = HAI_EMPTY;

			// shift to fill space (but keep order)
			for (k = 0; k < nHai - 3; k++)
				if (tTe[k] == HAI_EMPTY)
					for (j = k + 1; j < nHai; j++)
						if (tTe[j] != HAI_EMPTY)
						{
							tTe[k] = tTe[j];
							tTe[j] = HAI_EMPTY;
							break;
						}

			tTe[IND_NAKISTATE] = nNaki + 1;
			shanten(tTe, tMachi, NULL);

			skip = 0;
			for (j = 0; j < IND_MACHISIZE; j++)
				if ((machi[j] > 0) != (tMachi[j] > 0))
				{
					skip = 1;
					break;
				}
			if (skip)
			{
				// shift all the rest inds by 4 indices and reduce nQuadHai by 1
				for (j = 4 * i + 4; j < 12; j++)
					inds[j - 4] = inds[j];
				nQuadHai--;
				break;
			}
		}

	}
	inds[nQuadHai << 2] = -1;

	if (nQuadHai == 0)
		return 0;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_ANKAN, inds);
		return 0;
	}
	else
	{
		hai[0] = te[inds[4 * optionChosen + 0]];
		hai[1] = te[inds[4 * optionChosen + 1]];
		hai[2] = te[inds[4 * optionChosen + 2]];
		hai[3] = te[inds[4 * optionChosen + 3]]; // will be thrown away unless akadora
		te[inds[4 * optionChosen + 0]] = HAI_EMPTY;
		te[inds[4 * optionChosen + 1]] = HAI_EMPTY;
		te[inds[4 * optionChosen + 2]] = HAI_EMPTY;
		te[inds[4 * optionChosen + 3]] = HAI_EMPTY;

		// put akadora in the middle if present
		for (i = 0; i < 4; i++)
		if (hai[i] & FLAG_HAI_AKADORA)
		{
			temp = hai[1];
			hai[1] = hai[i];
			hai[i] = temp;
		}

		// shift to fill space (but keep order)
		for (i = 0; i < nHai - 3; i++)
			if (te[i] == HAI_EMPTY)
				for (j = i + 1; j < nHai; j++)
					if (te[j] != HAI_EMPTY)
					{
						te[i] = te[j];
						te[j] = HAI_EMPTY;
						break;
					}

		if (te[IND_TSUMOHAI] != HAI_EMPTY)
		{
			te[nHai - 4] = te[IND_TSUMOHAI];
			te[IND_TSUMOHAI] = HAI_EMPTY;
		}

		// put back kan hais in right indices (an extra hai will be implicitly added by display function)
		te[nHai - 3 + 0] = hai[0];
		te[nHai - 3 + 1] = hai[1];
		te[nHai - 3 + 2] = hai[2];

		// fill in te[IND_NAKISTATE] (3/3/3/3/1/3 bit) shift is (4 + 3 * nNaki) 
		te[IND_NAKISTATE]++;
		te[IND_NAKISTATE] |= NAKI_ANKAN << (4 + 3 * nNaki);

		return 0;
	}

	return 0;
}

int GameThread::askKakan(int* te, int optionChosen)
// return which hai was chosen to kakan, and HAI_EMPTY if not doing kakan
{
	int nNaki, nHai, nKanzai, i, j, ponHai, inds[3 + 1], nakiID[3];
	int hai;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	nKanzai = 0;
	for (i = 0; i < nNaki; i++)
	{
		if (((te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07) == NAKI_PON)
		{
			ponHai = te[10 - 3 * i] & MASK_HAI_NUMBER;
			for (j = 0; j < nHai; j++)
				if ((te[j] & MASK_HAI_NUMBER) == ponHai)
				{
					nakiID[nKanzai] = i;
					inds[nKanzai++] = j;
					break;
				}				
			if ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) == ponHai)
			{
				nakiID[nKanzai] = i;
				inds[nKanzai++] = IND_TSUMOHAI;
			}
		}
	}
	inds[nKanzai] = -1;

	if (nKanzai == 0)
		return HAI_EMPTY;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_KAKAN, inds);
		return HAI_EMPTY;
	}
	else
	{
		hai = te[inds[optionChosen]];

		// if akadora, switch with the hai with FLAG_HAI_NOT_TSUMOHAI up
        // (this can become problematic in rare cases if 2 akadoras of the same tile are allowed)
		if (hai & FLAG_HAI_AKADORA)
		{
			for (i = 0; i < 3; i++)
                if (te[10 - 3 * nakiID[optionChosen] + i] & FLAG_HAI_NOT_TSUMOHAI)
                {
                    te[10 - 3 * nakiID[optionChosen] + i] |= FLAG_HAI_AKADORA;
                    break;
                }
		}

		// remove hai
		te[inds[optionChosen]] = HAI_EMPTY;

		// shift to fill space if it wasn't tsumohai
		if (inds[optionChosen] != IND_TSUMOHAI)
		{
            for (i = 0; i < nHai - 1; i++)
                if (te[i] == HAI_EMPTY)
                    for (j = i + 1; j < nHai; j++)
                        if (te[j] != HAI_EMPTY)
                        {
                            te[i] = te[j];
                            te[j] = HAI_EMPTY;
                            break;
                        }
			te[nHai - 1] = te[IND_TSUMOHAI];
			te[IND_TSUMOHAI] = HAI_EMPTY;
		}

		te[IND_NAKISTATE] &= ~(0x07 << (4 + 3 * nakiID[optionChosen]));
        te[IND_NAKISTATE] |= (NAKI_KAKAN << (4 + 3 * nakiID[optionChosen]));

		return hai;
	}

	return HAI_EMPTY;
}

int GameThread::printWanpai(int* yama, int nKan, int prevNaki)
{
	int i;
	int nDoraHyouji;
	int lineTop[8], lineBot[10];

	nDoraHyouji = 1 + nKan - ((prevNaki == NAKI_DAIMINKAN) || (prevNaki == NAKI_KAKAN));

	// 17 15 13 11 9 7 5 3 
	// 16 14 12 10 8 6 4 2 0 1

	// [1, 0, 3, 2]: rinshanpai(in that order)
	// [5, 7, 9, 11, 13] : dora hyoujihai
	// [4, 6, 8, 10, 12] : uradora
	// [14, 15, 16, 17] : nKan # of extra wanpais

	for (i = 0; i < 8; i++)
	{
		lineTop[i] = HAI_BACKSIDE;
		lineBot[i] = HAI_BACKSIDE;
	}
	lineBot[8] = lineBot[9] = HAI_BACKSIDE;

	if (nKan < 4)
		lineTop[0] = HAI_EMPTY;
	if (nKan < 3)
		lineBot[0] = HAI_EMPTY;
	if (nKan < 2)
		lineTop[1] = HAI_EMPTY;
	if (nKan < 1)
		lineBot[1] = HAI_EMPTY;
	if (nDoraHyouji > 4)
		lineTop[2] = yama[13];
	if (nDoraHyouji > 3)
		lineTop[3] = yama[11];
	if (nDoraHyouji > 2)
		lineTop[4] = yama[9];
	if (nDoraHyouji > 1)
		lineTop[5] = yama[7];
	if (nDoraHyouji > 0)
		lineTop[6] = yama[5];
	if (nKan > 3)
		lineBot[7] = HAI_EMPTY;
	if (nKan > 2)
		lineTop[7] = HAI_EMPTY;
	if (nKan > 1)
		lineBot[8] = HAI_EMPTY;
	if (nKan > 0)
		lineBot[9] = HAI_EMPTY;

	printHai(lineTop, 8);
	printf("\n");
	printHai(lineBot, 10);
	printf("\n");

	return 0;
}


int GameThread::askDaiminkan(int* te, int sutehai, int turnDiff, int optionChosen)
{
	int nNaki, nHai, nSame, inds[3], i, j, temp;
	int hai0, hai1, hai2;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	if (nNaki == 4)
		return 0;

	nSame = 0;
	for (i = 0; i < nHai; i++)
		if ((te[i] & MASK_HAI_NUMBER) == (sutehai & MASK_HAI_NUMBER))
			inds[nSame++] = i;

	if (nSame != 3)
		return 0;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[nakiDir]]->registerAction(ACTION_DAIMINKAN, NULL);
		return 0;
	}
	else
	{
		// process kan

		// turnDiff = (curDir + 4 - myDir) & 0x03; // 1 shimocha 2 toimen 3 kamicha; subindex for sutehai is (3 - turnDiff); base index nHai - 3

		// back up hais and replace with HAI_EMPTY
		hai0 = te[inds[0]];
		hai1 = te[inds[1]];
		hai2 = te[inds[2]];
		te[inds[0]] = HAI_EMPTY;
		te[inds[1]] = HAI_EMPTY;
		te[inds[2]] = HAI_EMPTY;

		// if hai2 is an akadora, swap with hai1 (hai2 will be thrown away)
		if (hai2 & FLAG_HAI_AKADORA)
		{
			temp = hai2;
			hai2 = hai1;
			hai1 = temp;
		}

		// shift to fill space (but keep order)
		for (i = 0; i < nHai - 3; i++)
			if (te[i] == HAI_EMPTY)
				for (j = i + 1; j < nHai; j++)
					if (te[j] != HAI_EMPTY)
					{
						te[i] = te[j];
						te[j] = HAI_EMPTY;
						break;
					}

		// put back kan hais in right indices (hai2 will be implicitly added by display function)
		te[nHai - 3 + ((3 - turnDiff + 0) % 3)] = sutehai | FLAG_HAI_NOT_TSUMOHAI;
		te[nHai - 3 + ((3 - turnDiff + 1) % 3)] = hai0;
		te[nHai - 3 + ((3 - turnDiff + 2) % 3)] = hai1;

		// fill in te[IND_NAKISTATE] (3/3/3/3/1/3 bit) shift is (4 + 3 * nNaki) 
		te[IND_NAKISTATE]++;
		te[IND_NAKISTATE] |= FLAG_TE_NOT_MENZEN;
		te[IND_NAKISTATE] |= NAKI_DAIMINKAN << (4 + 3 * nNaki);

		return 0;
	}

	return 0;
}

int GameThread::askPon(int* te, int sutehai, int turnDiff, int optionChosen)
{
	int nNaki, nHai, i, j, nSame, inds[4 + 1], akaInd, temp;
	int hai0, hai1;
	int jihai[7];

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	if (nNaki == 4)
		return 0;

	nSame = 0;
	akaInd = IND_NAKISTATE; // default for not found
	for (i = 0; i < nHai; i++)
	{
		if ((te[i] & MASK_HAI_NUMBER) == (sutehai & MASK_HAI_NUMBER))
		{
			inds[nSame++] = i;
			if (te[i] & FLAG_HAI_AKADORA)
				akaInd = i;
		}
			
	}

	/*
	if nSame >= 2, ask pon
	option 0: don't pon
	option 1: pon, not showing akadora, if any
	option 2: pon showing akadora (only for nSame == 3 && akadora present)
	*/

	if (nSame < 2)
		return 0;

	if ((nSame == 3) && (akaInd != IND_NAKISTATE)) 
	{
		// send akadora's index to [2]
		for (i = 0; i < 2; i++)
			if (inds[i] == akaInd)
			{
				temp = inds[2];
				inds[2] = akaInd;
				inds[i] = temp;
				break;
			}

		inds[4] = -1;
		inds[3] = inds[2];
		inds[2] = inds[1];
	}
	else
	{
		inds[2] = -1;
	}

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[nakiDir]]->registerAction(ACTION_PON, inds);
		return 0;
	}
	else
	{
		// process pon

		// turnDiff = (curDir + 4 - myDir) & 0x03; // 1 shimocha 2 toimen 3 kamicha; subindex for sutehai is (3 - turnDiff); base index nHai - 3
		
		// back up option1 hais and replace with HAI_EMPTY
		hai0 = te[inds[(optionChosen << 1) + 0]];
		hai1 = te[inds[(optionChosen << 1) + 1]];
		te[inds[(optionChosen << 1) + 0]] = HAI_EMPTY;
		te[inds[(optionChosen << 1) + 1]] = HAI_EMPTY;

		// put last non-empty hai in IND_TSUMOHAI
		for (i = nHai - 1; i >= 0; i--)
			if (te[i] != HAI_EMPTY)
			{
				te[IND_TSUMOHAI] = te[i];
				te[i] = HAI_EMPTY;
				break;
			}

		// shift to fill space (but keep order)
		for (i = 0; i < nHai - 3; i++)
			if (te[i] == HAI_EMPTY)
				for (j = i + 1; j < nHai; j++)
					if (te[j] != HAI_EMPTY)
					{
						te[i] = te[j];
						te[j] = HAI_EMPTY;
						break;
					}

		// put back pon hais in right indices
		te[nHai - 3 + ((3 - turnDiff + 0) % 3)] = sutehai | FLAG_HAI_NOT_TSUMOHAI;
		te[nHai - 3 + ((3 - turnDiff + 1) % 3)] = hai0;
		te[nHai - 3 + ((3 - turnDiff + 2) % 3)] = hai1;

		// fill in te[IND_NAKISTATE] (3/3/3/3/1/3 bit) shift is (4 + 3 * nNaki) 
		te[IND_NAKISTATE]++;
		te[IND_NAKISTATE] |= FLAG_TE_NOT_MENZEN;
		te[IND_NAKISTATE] |= NAKI_PON << (4 + 3 * nNaki);

		// check pao
		nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
		for (i = 0; i < 7; i++)
			jihai[i] = 0;
		for (i = 0; i < nNaki; i++)
		{
			if ((te[10 - 3 * i] & MASK_HAI_NUMBER) >= HAI_TON)
				jihai[(te[10 - 3 * i] & MASK_HAI_NUMBER) - HAI_TON] = 1;
		}
		if ((jihai[0] && jihai[1] && jihai[2] && jihai[3] && ((te[10 - 3 * (nNaki - 1)] & MASK_HAI_NUMBER) >= HAI_TON) && ((te[10 - 3 * (nNaki - 1)] & MASK_HAI_NUMBER) <= HAI_PE)) || \
			(jihai[4] && jihai[5] && jihai[6] && ((te[10 - 3 * (nNaki - 1)] & MASK_HAI_NUMBER) >= HAI_HAKU)))
			// MASK_YAUMAN_PAOTURNDIFF	0x30000000
			// FLAG_YAKUMAN_PAO			0x80000000
		{
			te[IND_YAKUMANFLAGS] |= FLAG_YAKUMAN_PAO;
			te[IND_YAKUMANFLAGS] |= (turnDiff << 28); // make sure these don't get deleted when markTeyakuman

            //printf("pao\n\n");
		}

		return 0;
	}

	return 0;
}

int GameThread::askDahai(int* te, int prevNaki, int optionChosen)
// if this function is called after a tsumo, tsumohai is in IND_TSUMOHAI
// if this function is called after a chi or pon, the extra hai is in IND_TSUMOHAI
// if this function is called after a kan, rinshanpai is in IND_TSUMOHAI
{
	int sutehai;
	int nNaki, nHai, i;
	int kuikae[2], skip, nOption, optionInds[14 + 1];

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	if (te[IND_NAKISTATE] & FLAG_TE_RICHI)
	{
		if (optionChosen == ACTION_REGISTER)
		{
			optionInds[0] = IND_TSUMOHAI;
			optionInds[1] = -1;

			player[dirToPlayer[curDir]]->registerAction(ACTION_DAHAI, optionInds);
			return HAI_EMPTY;
		}
		else
		{
			sutehai = te[IND_TSUMOHAI];

            if (sutehai & FLAG_HAI_TSUMOGIRI)
                sutehai &= ~FLAG_HAI_TSUMOGIRI; // this was intentionally reversed in askRichi
            else
                sutehai |= FLAG_HAI_TSUMOGIRI;

			te[IND_TSUMOHAI] = HAI_EMPTY;
			return sutehai;
		}
	}

	// kuikae check
	kuikae[0] = kuikae[1] = HAI_EMPTY;
	if ((prevNaki == NAKI_CHI) || (prevNaki == NAKI_PON))
	{
		for (i = 0; i < 3; i++)
			if (te[13 - nNaki * 3 + i] & FLAG_HAI_NOT_TSUMOHAI)
			{
				kuikae[0] = (te[13 - nNaki * 3 + i] & MASK_HAI_NUMBER); // genbutsu
				break;
			}
		if (prevNaki == NAKI_CHI) 
		{
			if ((kuikae[0] % 9 <= 5) && \
				(((te[13 - nNaki * 3 + ((i + 1) % 3)] & MASK_HAI_NUMBER) % 9 == (kuikae[0] % 9) + 2) || \
				((te[13 - nNaki * 3 + ((i + 2) % 3)] & MASK_HAI_NUMBER) % 9 == (kuikae[0] % 9) + 2)))
				// higher suji up to chi (6)78 dahai 9
				kuikae[1] = kuikae[0] + 3;
			if ((kuikae[0] % 9 >= 3) && \
				(((te[13 - nNaki * 3 + ((i + 1) % 3)] & MASK_HAI_NUMBER) % 9 == (kuikae[0] % 9) - 2) || \
				((te[13 - nNaki * 3 + ((i + 2) % 3)] & MASK_HAI_NUMBER) % 9 == (kuikae[0] % 9) - 2)))
				// lower suji down to chi 23(4) dahai 1
				kuikae[1] = kuikae[0] - 3;
		}
		// if there are no hais to dahai because of kuikae, then you enter agariHouki as a penalty
		// e.g., 3345 chi 3/6 or 3456 chi 3/6, but no other hai left
		// you will automatically dahai IND_TSUMOHAI
		skip = 1;
		for (i = 0; i <= nHai; i++)
		{
			if (i == nHai)
				i = IND_TSUMOHAI;
			if (((te[i] & MASK_HAI_NUMBER) == kuikae[0]) || ((te[i] & MASK_HAI_NUMBER) == kuikae[1]))
				continue;
			skip = 0;
			break;
		}
		if (skip)
			return HAI_EMPTY;
	}

	// Prepare options
	nOption = 0;
    for (i = 0; i < nHai; i++) {
		if (((te[i] & MASK_HAI_NUMBER) != kuikae[0]) && ((te[i] & MASK_HAI_NUMBER) != kuikae[1]))
			optionInds[nOption++] = i;
	}
	if (((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) != kuikae[0]) && ((te[IND_TSUMOHAI] & MASK_HAI_NUMBER) != kuikae[1]))
		optionInds[nOption++] = IND_TSUMOHAI;
	optionInds[nOption] = -1;

	if (optionChosen == ACTION_REGISTER)
	{
		player[dirToPlayer[curDir]]->registerAction(ACTION_DAHAI, optionInds);
		return HAI_EMPTY;
	}
	else
	{
		sutehai = te[optionInds[optionChosen]];
		te[optionInds[optionChosen]] = te[IND_TSUMOHAI];
		te[IND_TSUMOHAI] = HAI_EMPTY;

        if ((optionInds[optionChosen] == IND_TSUMOHAI) && (prevNaki != NAKI_PON) && (prevNaki != NAKI_CHI))
            sutehai |= FLAG_HAI_TSUMOGIRI;

		return sutehai;
	}

	return HAI_EMPTY;
}

int GameThread::printHairi(int* te, int* hairi)
{
	int i, j, k, curPos;
	int nHai, nNaki; // # of hais not in naki
	int nForm, nHairi;

	nHairi = hairi[IND_NHAIRI];
	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	for (i = 0; i < nHairi; i++)
	{
		printf("[%d] ", i);
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
					printf(" ");
				}
				break;
			}
			else
			{
				printHai(hairi + i * IND_HAIRISIZE + curPos, nForm);
				printf(" ");
				curPos += nForm;
			}
				
		}

		// for chitoi tenpai
		if ((j == 6) && (hairi[i * IND_HAIRISIZE + 14 + 5] != FORM_TANKI) && (curPos == 12))
		{
			printHai(hairi + i * IND_HAIRISIZE + curPos, 1);
			printf(" ");
		}

		printNaki(te);
		
		printf("\n");
	}

	return 0;
}

int GameThread::str2te(int* te, const char* str, int maxN)
// format 5667m23577p3s366z (0m, 0p, 0s akadora)
{
	int len, lastPos, i, j, nHai, base;

	if (maxN > 14)
		return 1;

	len = strlen(str);

	if (len > 1)
	{
		for (i = 0; i < maxN; i++)
			te[i] = HAI_EMPTY;

		lastPos = 0;
		nHai = 0;
		for (i = 0; i < len; i++)
		{
			if (str[i] > '9')
			{
				switch (str[i])
				{
				case 'm':
				case 'M':
					base = HAI_1M;
					break;
				case 'p':
				case 'P':
					base = HAI_1P;
					break;
				case 's':
				case 'S':
					base = HAI_1S;
					break;
				case 'z':
				case 'Z':
					base = HAI_TON;
					break;
				default:
					return 1;
				}

				for (j = lastPos; j < i; j++)
				if (nHai < maxN)
				{
					te[nHai++] = base + str[j] - '1' + (FLAG_HAI_AKADORA + 5) * (str[j] == '0');
					if ((te[nHai - 1] & MASK_HAI_NUMBER) > HAI_CHUN)
						return 1;
				}
				else
				{
					return 1;
				}
				lastPos = i + 1;
			}
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

int GameThread::shipai(int* yama, int* kawa, int bUseAkadora)
{
	int i, n, hai, indSwap, temp;
	int count[34];
	int aka[3];

	yama[IND_NYAMA] = 136;
	kawa[IND_KAWASIZE * 0 + IND_NKAWA] = 0;
	kawa[IND_KAWASIZE * 1 + IND_NKAWA] = 0;
	kawa[IND_KAWASIZE * 2 + IND_NKAWA] = 0;
	kawa[IND_KAWASIZE * 3 + IND_NKAWA] = 0;
	
	for(i = 0; i < 34; i++)
		count[i] = 0;

	aka[0] = aka[1] = aka[2] = 0; // m, p, s

	n = 0;
	hai = 0;
	do
	{
		hai &= MASK_HAI_NUMBER;

		while(count[hai] == 4)
			hai = (hai + 1) % 34;

		count[hai]++;
		
		if (bUseAkadora)
		{
			switch (hai)
			{
			case HAI_5M: 
				if (aka[0]++ < 1)
					hai |= FLAG_HAI_AKADORA;
				break;
			case HAI_5P: 
				if (aka[1]++ < 1) // some rules use 2 of these
					hai |= FLAG_HAI_AKADORA;
				break;
			case HAI_5S: 
				if (aka[2]++ < 1)
					hai |= FLAG_HAI_AKADORA;
				break;
			}
		}

		yama[n++] = hai;
	} while(n < 136);

	for (i = 0; i < 136; i++)
	{
		indSwap = genrand_int32() % 136; // Mersenne Twister algorithm
		temp = yama[i];
		yama[i] = yama[indSwap];
		yama[indSwap] = temp;
	}

	return 0;
}

int GameThread::haipai(int* yama, int* te)
{
	int i, k;

	sai = (genrand_int32() % 6) + (genrand_int32() % 6) + 2;

	for (k = 0; k < 4; k++)
	{
        for (i = 0; i < IND_TSUMOHAI; i++)
			te[IND_TESIZE * k + i] = yama[--yama[IND_NYAMA]];

		te[IND_TESIZE * k + IND_TSUMOHAI] = HAI_EMPTY;
		te[IND_TESIZE * k + IND_NAKISTATE] = FLAG_TE_DAIICHITSUMO;

		te[IND_TESIZE * k + IND_YAKUFLAGS] = 0;
		te[IND_TESIZE * k + IND_YAKUMANFLAGS] = 0;
	}

	return 0;
}

int GameThread::tsumo(int* yama, int* te)
{
	if (te[IND_TSUMOHAI] == HAI_EMPTY)
	{
		te[IND_TSUMOHAI] = yama[--yama[IND_NYAMA]];
		return 0;
	}
	else
	{
		return 1; // after chi or pon
	}

	return 0;
}

void GameThread::printHai(const int *te, int n)
{
	int i;
	char tile;
    int color;
//	WORD color;
//	HANDLE hConsole;
//	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	for(i = 0; i < n; i++)
	{
		color = 0xF0; // white background
		if ((te[i] & MASK_HAI_NUMBER) <= HAI_9S)
		{
			switch ((te[i] & MASK_HAI_NUMBER) / 9)
			{
			case 0: // manzu
				color += 0x00; // black text
				break;
			case 1: // pinzu
				color += 0x09; // bright blue text
				break;
			case 2: // sozu
				color += 0x02; // dark green text
				break;
			}
			color -= 0x30 * ((te[i] & FLAG_HAI_AKADORA) != 0) + 0x80 * ((te[i] & FLAG_HAI_NOT_TSUMOHAI) != 0);
//			SetConsoleTextAttribute(hConsole, color);
			printf("%d", (te[i] & MASK_HAI_NUMBER) % 9 + 1);
//#ifdef FLAG_DEBUG
            switch((te[i] & MASK_HAI_NUMBER) / 9)
            {
            case 0:
                tile = 'm';
                break;
            case 1:
                tile = 'p';
                break;
            case 2:
                tile = 's';
                break;
            default: // this should not happen
                tile = 'x';
                break;
            }
            printf("%c", tile);
//#endif
		}
		else
		{
			switch (te[i] & MASK_HAI_NUMBER)
			{
			case HAI_TON:
				color += 0x00; // black text
                tile = 'E';
				break;
			case HAI_NAN:
				color += 0x00; // black text
				tile = 'S';
				break;
			case HAI_SHA:
				color += 0x00; // black text
				tile = 'W';
				break;
			case HAI_PE:
				color += 0x00; // black text
				tile = 'N';
				break;
			case HAI_HAKU:
				color += 0x07; // gray text
                tile = 'D';
				break;
			case HAI_HATSU:
				color += 0x02; // dark green text
                tile = 'D';
				break;
			case HAI_CHUN:
				color += 0x0C; // bright red text
                tile = 'D';
				break;
			case HAI_BACKSIDE:
				color = 0x6F; // dark yellow background
				tile = '_';
				break;
			case HAI_EMPTY:
            default:
				color = 0x00; // black text
				tile = ' ';
				break;
			}
			color -= 0x80 * ((te[i] & FLAG_HAI_NOT_TSUMOHAI) != 0);
//			SetConsoleTextAttribute(hConsole, color);
//#ifndef FLAG_DEBUG
//             printf("%c", tile);
//#else
            printf("%dz", (te[i] & MASK_HAI_NUMBER) - HAI_TON + 1);
//#endif
		}
	}
	
//	SetConsoleTextAttribute(hConsole, 7);

}

void GameThread::printNaki(const int *te)
{
	int nNaki, i, temp, nakiKind;

	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;

	// te[IND_NAKISTATE]
	// 3/3/3/3/1/3
	// (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07

	for (i = nNaki - 1; i >= 0; i--)
	{
		nakiKind = (te[IND_NAKISTATE] >> (4 + 3 * i)) & 0x07;

#ifdef FLAG_DEBUG
        switch(nakiKind)
        {
        case NAKI_ANKAN:
            printf("akan ");
            break;
        case NAKI_KAKAN:
            printf("kkan ");
            break;
        case NAKI_DAIMINKAN:
            printf("dkan ");
            break;
        case NAKI_PON:
            printf("pon ");
            break;
        case NAKI_CHI:
            printf("chi ");
            break;
        }
#endif

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
		
		printf(" ");
	}

}

void GameThread::printTe(const int *te)
{
	int nNaki, nHai;
	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;

	printHai(te, nHai);
	printf(" ");

	printHai(te + 13, 1);
	printf(" ");

	printNaki(te);
	
}

int GameThread::printMachi(int* machi)
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

int GameThread::copyTeN(int* to, const int* from, int n)
{
	int i;

	for (i = 0; i < n; i++)
		to[i] = from[i];

	return 0;
}

int GameThread::sortPartition(int* array, int left, int right, int pivotIndex) // partition function for quicksort
{
	int i, temp, storeIndex;
	int pivotValue;

	pivotValue = (array[pivotIndex] & MASK_HAI_NUMBER);

	// swap
	temp = array[pivotIndex];
	array[pivotIndex] = array[right];
	array[right] = temp;

	storeIndex = left;

	for(i = left; i < right; i++)
		if ((array[i] & MASK_HAI_NUMBER) <= pivotValue)
		{
			// swap
			temp = array[i];
			array[i] = array[storeIndex];
			array[storeIndex++] = temp;
		}
	
	// swap
	temp = array[storeIndex];
	array[storeIndex] = array[right];
	array[right] = temp;

	return storeIndex;	
}

/*
int sortQuick(int* array, int left, int right) // recursive quicksort
{
	int inds[3], minI, midI, pivotIndex, pivotNewIndex;

	if(left < right)
	{
		// choose pivot (index for the median of left, middle, right values)
		inds[0] = left;
		inds[1] = (left + right) / 2;
		inds[2] = right;

		minI = 0;
		minI = (array[inds[1]] < array[inds[minI]]);
		minI += (array[inds[2]] < array[inds[minI]]) << (!minI);

		midI = minI + 1 - (minI & 0x2) - ((minI & 0x2) >> 1); // midI = (minI + 1) % 3;
		pivotIndex = inds[(midI + (array[inds[midI]] > array[inds[midI + 1 - (midI & 0x2) - ((midI & 0x2) >> 1)]])) % 3];
		// fast code for getting median


		// choose pivot (middle index)
		// pivotIndex = (left + right) / 2;

		pivotNewIndex = sortPartition(array, left, right, pivotIndex);
		sortQuick(array, left, pivotNewIndex - 1);
		sortQuick(array, pivotNewIndex + 1, right);
	}

	return 0;
}
*/

void GameThread::sortHai(int* arr, int n) // nonrecursive quicksort
{
	// sortQuick(arr, 0, n - 1);

    //int inds[3], minI, midI;
    int pivotIndex;
    //int depth, left[136], right[136], pivotNewIndex[136], seq[136];
    int depth, left[IND_NAKISTATE], right[IND_NAKISTATE], pivotNewIndex[IND_NAKISTATE], seq[IND_NAKISTATE];

	// initialize
	depth = 0;
	left[0] = 0;
	right[0] = n - 1;
	seq[0] = 0;

	do
	{
		if(left[depth] < right[depth])
		{
			if(seq[depth] == 0)
			{
//				// choose pivot (index for the median of left, middle, right values)
//				inds[0] = left[depth];
//				inds[1] = (left[depth] + right[depth]) / 2;
//				inds[2] = right[depth];

//				minI = 0;
//				minI = ((arr[inds[1]] & MASK_HAI_NUMBER) < (arr[inds[minI]] & MASK_HAI_NUMBER));
//				minI += ((arr[inds[2]] & MASK_HAI_NUMBER) < (arr[inds[minI]] & MASK_HAI_NUMBER)) << static_cast<int>(!minI);

//				midI = minI + 1 - (minI & 0x2) - ((minI & 0x2) >> 1); // midI = (minI + 1) % 3;
//				pivotIndex = inds[(midI + ((arr[inds[midI]] & MASK_HAI_NUMBER) > (arr[inds[midI + 1 - (midI & 0x2) - ((midI & 0x2) >> 1)]] & MASK_HAI_NUMBER))) % 3];
//				// fast code for getting median

				// choose pivot (middle index)
                pivotIndex = (left[depth] + right[depth]) / 2;

				pivotNewIndex[depth] = sortPartition(arr, left[depth], right[depth], pivotIndex);

				left[depth + 1] = left[depth];
				right[depth + 1] = pivotNewIndex[depth] - 1;
				seq[depth + 1] = 0;

				seq[depth]++;
				depth++;
			}
			else if(seq[depth] == 1)
			{
				left[depth + 1] = pivotNewIndex[depth] + 1;
				right[depth + 1] = right[depth];
				seq[depth + 1] = 0;

				seq[depth]++;
				depth++;
			}
			else
			{
				depth--;
			}
		}
		else
		{
			depth--;
		}
	} while(depth >= 0);

    return;
}

void GameThread::sortHaiFixAka(int* arr, int n)
{
    for (int hai = HAI_5M; hai <= HAI_5S; hai += 9) {
        int n5 = 0;
        int akaPos = -1;

        for (int i = 0; i < n; i++) {
            if ((arr[i] & MASK_HAI_NUMBER) == hai) {
                if (arr[i] & FLAG_HAI_AKADORA) {
                    akaPos = n5;
                    break;
                }
                n5++;
            }
        }

        sortHai(arr, n);

        if (akaPos >= 0) {
            for (int i = 0; i < n; i++) {
                if ((arr[i] & MASK_HAI_NUMBER) == hai) {
                    if (akaPos == n5)
                        arr[i] |= FLAG_HAI_AKADORA;
                    else
                        arr[i] &= ~FLAG_HAI_AKADORA;
                    n5++;
                }
            }
        }
    }

    return;
}

void GameThread::searchForm(const int* tTe, int nHai, int nNaki, const int* inds, int depth, int* form, int* cache, int* machi, int* hairi)
// Assumes sorted tTe
{
    int i, j, k, l, n, prevDepth, found;
	int nextInds[13], nextN;
	int t, d, toi, shanten;
	int smaller;
	int tempHai, curPos, significance;

	n = nHai;
	for (i = 0; i < depth; i++)
		n -= 2 + ((form[i] & FORM_KOTSU) >> 2);
	// n : number of hais at this depth


	// form should be going deeper, or if the same, hai should be increasing or the same to obtain sorted & non-redundant results
	// at the same form & depth, two consequtive entries should not be identical (e.g., 1233 case)

	if (n > 2)
	{

		// shuntsu (shupai only)
        if ((depth == 0) || (FORM_SHUNTSU <= form[depth - 1])) {
			cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = -1;

			if (depth == 0)
                prevDepth = HAI_1M;
			else
                prevDepth = cache[(depth - 1) * 3 + 0];

			for (i = 0; i <= n - 3; i++) {
				if ((tTe[inds[i]] >= HAI_8S))
					break;
                if ((tTe[inds[i]] < prevDepth) || (tTe[inds[i]] <= cache[depth * 3 + 0]) || ((tTe[inds[i]] % 9) >= 7))
					continue;

                found = 0;
                for (j = i + 1; (j <= i + 4) && (j < n); j++) {
                    if (tTe[inds[j]] == tTe[inds[i]] + 1) {
                        for (k = j + 1; (k <= j + 4) && (k < n); k++) {
                            if (tTe[inds[k]] == tTe[inds[j]] + 1) {
                                nextN = 0;
                                for (l = 0; l < n; l++)
                                    if ((l != i) && (l != j) && (l != k))
                                        nextInds[nextN++] = inds[l];

                                cache[depth * 3 + 0] = tTe[inds[i]];
                                cache[depth * 3 + 1] = tTe[inds[j]];
                                cache[depth * 3 + 2] = tTe[inds[k]];

                                form[depth] = FORM_SHUNTSU;
                                searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);

                                found = 1;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                }
			}
		}


		// kotsu
        if ((depth == 0) || (FORM_KOTSU <= form[depth - 1])) {
            cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = -1;

            if ((depth == 0) || (FORM_KOTSU != form[depth - 1]))
                prevDepth = HAI_1M;
            else
                prevDepth = cache[(depth - 1) * 3 + 0];

            for (i = 0; i <= n - 3; i++) {
                if ((tTe[inds[i]] < prevDepth) || (tTe[inds[i]] <= cache[depth * 3 + 0]))
                    continue;

                if ((tTe[inds[i]] == tTe[inds[i + 1]]) && (tTe[inds[i + 1]] == tTe[inds[i + 2]])) {
                    nextN = 0;
                    for (l = 0; l < n; l++)
                        if ((l < i) || (l > i + 2))
                            nextInds[nextN++] = inds[l];

                    cache[depth * 3 + 0] = tTe[inds[i]];
                    cache[depth * 3 + 1] = tTe[inds[i]];
                    cache[depth * 3 + 2] = tTe[inds[i]];

                    form[depth] = FORM_KOTSU;
                    searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);

                    i += 2;
                }
            }
        }
	}

	if (n > 1)
	{
		// toitsu
        if ((depth == 0) || (FORM_TOITSU <= form[depth - 1])) {
            cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = -1;

            if ((depth == 0) || (FORM_TOITSU != form[depth - 1]))
                prevDepth = HAI_1M;
            else
                prevDepth = cache[(depth - 1) * 3 + 0];

            for (i = 0; i <= n - 2; i++) {
                if ((tTe[inds[i]] < prevDepth) || (tTe[inds[i]] <= cache[depth * 3 + 0]))
                    continue;

                if (tTe[inds[i]] == tTe[inds[i + 1]]) {
                    nextN = 0;
                    for (l = 0; l < n; l++)
                        if ((l != i) && (l != i + 1))
                            nextInds[nextN++] = inds[l];

                    cache[depth * 3 + 0] = tTe[inds[i]];
                    cache[depth * 3 + 1] = tTe[inds[i]];
                    cache[depth * 3 + 2] = HAI_EMPTY;

                    form[depth] = FORM_TOITSU;
                    searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);

                    i += 1;
                }
            }
        }

		// ryanmen (shupai only)
        if ((depth == 0) || (FORM_RYANMEN <= form[depth - 1])) {
            cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = -1;

            if ((depth == 0) || (FORM_RYANMEN != form[depth - 1]))
                prevDepth = HAI_1M;
            else
                prevDepth = cache[(depth - 1) * 3 + 0];

            for (i = 0; i <= n - 2; i++) {
                if ((tTe[inds[i]] >= HAI_9S))
                    break;
                if ((tTe[inds[i]] < prevDepth) || (tTe[inds[i]] <= cache[depth * 3 + 0]) || ((tTe[inds[i]] % 9) == 8))
                    continue;

                for (j = i + 1; (j <= i + 4) && (j < n); j++) {
                    if (tTe[inds[j]] == tTe[inds[i]] + 1) {
                        nextN = 0;
                        for (l = 0; l < n; l++)
                            if ((l != i) && (l != j))
                                nextInds[nextN++] = inds[l];

                        cache[depth * 3 + 0] = tTe[inds[i]];
                        cache[depth * 3 + 1] = tTe[inds[j]];
                        cache[depth * 3 + 2] = HAI_EMPTY;

                        form[depth] = FORM_RYANMEN;
                        searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);

                        break;
                    }
                }
            }
        }


		// kanchan (shupai only)
        if ((depth == 0) || (FORM_KANCHAN <= form[depth - 1])) {
            cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = -1;

            if ((depth == 0) || (FORM_KANCHAN != form[depth - 1]))
                prevDepth = HAI_1M;
            else
                prevDepth = cache[(depth - 1) * 3 + 0];

            for (i = 0; i <= n - 2; i++) {
                if ((tTe[inds[i]] >= HAI_8S))
                    break;
                if ((tTe[inds[i]] < prevDepth) || (tTe[inds[i]] <= cache[depth * 3 + 0]) || ((tTe[inds[i]] % 9) >= 7))
                    continue;

                for (j = i + 1; (j <= i + 8) && (j < n); j++) {
                    if (tTe[inds[j]] == tTe[inds[i]] + 2) {
                        nextN = 0;
                        for (l = 0; l < n; l++)
                            if ((l != i) && (l != j))
                                nextInds[nextN++] = inds[l];

                        cache[depth * 3 + 0] = tTe[inds[i]];
                        cache[depth * 3 + 1] = tTe[inds[j]];
                        cache[depth * 3 + 2] = HAI_EMPTY;

                        form[depth] = FORM_KANCHAN;
                        searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);

                        break;
                    }
                }
            }
        }
	}

	t = d = toi = 0;
	for (i = 0; i < depth; i++)
	{
		switch (form[i])
		{
		case FORM_TOITSU:
			toi++;
		case FORM_RYANMEN:
		case FORM_KANCHAN:
			d++;
			break;
		case FORM_SHUNTSU:
		case FORM_KOTSU:
			t++;
			break;
		}
	}

	// toitsu present: 2(4-n-t)-min(d-1,4-n-t)-1
	// no toitsu: 2(4-n-t)-min(d,4-n-t)

	int toiPresent = (toi != 0);
	int nt = 4 - nNaki - t;

	if (d - toiPresent < nt)
		smaller = d - toiPresent;
	else
		smaller = nt;
	shanten = (nt << 1) - smaller - toiPresent;


	// this is used to weed out insignificant interpretations 
	// e.g., breaking up 34 as 3 + 4 when doing so doesn't affect shanten
	significance = (t << 1) + d;

	if (shanten < machi[IND_NSHANTEN]) // new minimum
	{
		machi[IND_NSHANTEN] = shanten;
		if (machi != NULL)
		for (i = 0; i < 34; i++)
			machi[i] = 0;
		if (hairi != NULL)
			hairi[IND_NHAIRI] = 0;
	}

	if ((machi != NULL) && (shanten == machi[IND_NSHANTEN]) && (significance >= cache[21])) // new minimum or same as previous minimum
	{
		cache[21] = significance;

		for (i = 0; i < depth; i++)
		{
			switch (form[i])
			{
			case FORM_TOITSU:
				// if (shanten == 0) && (toi != 2), this cannot be a machi (this case is subsumed by below)
				// or if (toi == 1) && d-1 >= nt, this cannot be a machi
				if ((toi != 1) || (d - 1 < nt))
					machi[cache[3 * i + 0]]++;
				break;
			case FORM_RYANMEN:
				tempHai = cache[3 * i + 0];
				if ((tempHai != HAI_1M) && (tempHai != HAI_1P) && (tempHai != HAI_1S))
					machi[tempHai - 1]++;
				tempHai = cache[3 * i + 1];
				if ((tempHai != HAI_9M) && (tempHai != HAI_9P) && (tempHai != HAI_9S))
					machi[tempHai + 1]++;
				// if (!toi) && (d-1 >= 4-n-t), ryanmen can reduce shanten by becoming toitsu
				if ((!toi) && (d - 1 >= nt)) {
					machi[cache[3 * i + 0]]++;
					machi[cache[3 * i + 1]]++;
				}
				break;
			case FORM_KANCHAN:
				machi[cache[3 * i + 0] + 1]++;
				// if (!toi) && (d-1 >= 4-n-t), kanchan can reduce shanten by becoming toitsu
				if ((!toi) && (d - 1 >= nt)) {
					machi[cache[3 * i + 0]]++;
					machi[cache[3 * i + 1]]++;
				}
				break;
			}
		}

		// tanki -> toitsu condition
		// if (toi) && (d - 1 < 4 - n - t) or
		// if (!toi) && (d <= 4 - n - t), then getting a toitsu reduces shanten (note <=)
		// i.e., if (((toi) && (d - 1 < 4 - nNaki - t)) || ((!toi) && (d <= 4 - nNaki - t)) || (4 - nNaki - t == 0))
		// can be simplified to:
        if (d <= 4 - nNaki - t)
            for (i = 0; i < n; i++)
                machi[tTe[inds[i]]]++;

		// tanki -> kanchan or tanki -> ryanmen/penchan condition
		// d - (toi > 0) < 4 - n - t
		if (d - toiPresent < nt)
		{
			for (i = 0; i < n; i++)
			{
				if (tTe[inds[i]] >= HAI_TON)
					break;

				if ((tTe[inds[i]] % 9) >= 2)
					machi[tTe[inds[i]] - 2]++;
				if ((tTe[inds[i]] % 9) >= 1)
					machi[tTe[inds[i]] - 1]++;
				if ((tTe[inds[i]] % 9) <= 7)
					machi[tTe[inds[i]] + 1]++;
				if ((tTe[inds[i]] % 9) <= 6)
					machi[tTe[inds[i]] + 2]++;
			}

		}

		if (hairi != NULL)
		{
			curPos = 0;
			for (i = 0; i < depth; i++)
			{
				switch (form[i])
				{
				case FORM_SHUNTSU:
				case FORM_KOTSU:
					k = 3;
					break;
				case FORM_TOITSU:
				case FORM_RYANMEN:
				case FORM_KANCHAN:
					k = 2;
					break;
				default: // this should not happen
					k = 1;
					break;
				}
				for (j = 0; j < k; j++)
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + j] = cache[3 * i + j];
				curPos += k;
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + i] = form[i];
			}
			if (depth != 6)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + depth] = FORM_TANKI;
			if (n == 0)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + 0] = HAI_EMPTY;
			for (j = 0; j < n; j++)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + j] = tTe[inds[j]];
			hairi[IND_NHAIRI]++;
		}
	}

    return;
}

/*
int GameThread::searchForm(int* tTe, int nHai, int nNaki, int* inds, int depth, int* form, int* cache, int* machi, int* hairi)
// Assumes sorted tTe
{
	int i, j, k, l, n;
	int nextInds[13], nextN;
	int t, d, toi, shanten;
	int smaller;
	int tempHai, curPos, significance;

	n = nHai;
	for (i = 0; i < depth; i++)
		n -= 2 + ((form[i] & FORM_KOTSU) >> 2);
	// n : number of hais at this depth

		
	// form should be going deeper, or if the same, hai should be increasing or the same to obtain sorted & non-redundant results
	// at the same form & depth, two consequtive entries should not be identical (e.g., 1233 case)

	if (n > 2)
	{
		// shuntsu (shupai only)
		cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = HAI_EMPTY;

		for (i = 0; i <= n - 3; i++)
			if (tTe[inds[i]] > HAI_9S)
				break;
			else
				for (j = i + 1; (j <= i + 4) && (j < n); j++)
					if ((tTe[inds[j]] == tTe[inds[i]] + 1) && (tTe[inds[j]] / 9 == tTe[inds[i]] / 9))
						for (k = j + 1; (k <= j + 4) && (k < n); k++)
							if ((tTe[inds[k]] == tTe[inds[j]] + 1) && (tTe[inds[k]] / 9 == tTe[inds[i]] / 9))
								if ((depth == 0) || (FORM_SHUNTSU < form[depth - 1]) || ((FORM_SHUNTSU == form[depth - 1]) && (tTe[inds[i]] >= cache[(depth - 1) * 3 + 0])))
									if ((tTe[inds[k]] != cache[depth * 3 + 2]) || (tTe[inds[j]] != cache[depth * 3 + 1]) || (tTe[inds[i]] != cache[depth * 3 + 0]))
									{
										nextN = 0;
										for (l = 0; l < n; l++)
										if ((l != i) && (l != j) && (l != k))
											nextInds[nextN++] = inds[l];

										cache[depth * 3 + 0] = tTe[inds[i]];
										cache[depth * 3 + 1] = tTe[inds[j]];
										cache[depth * 3 + 2] = tTe[inds[k]];

										form[depth] = FORM_SHUNTSU;
										searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);
									}


		// kotsu
		cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = HAI_EMPTY;

		for (i = 0; i <= n - 3; i++)
			if ((tTe[inds[i]] == tTe[inds[i + 1]]) && (tTe[inds[i + 1]] == tTe[inds[i + 2]]))
				if ((depth == 0) || (FORM_KOTSU < form[depth - 1]) || ((FORM_KOTSU == form[depth - 1]) && (tTe[inds[i]] >= cache[(depth - 1) * 3 + 0])))
					if (tTe[inds[i]] != cache[depth * 3 + 0])
					{
						nextN = 0;
						for (l = 0; l < n; l++)
						if ((l != i) && (l != i + 1) && (l != i + 2))
							nextInds[nextN++] = inds[l];

						cache[depth * 3 + 0] = tTe[inds[i]];
						cache[depth * 3 + 1] = tTe[inds[i]];
						cache[depth * 3 + 2] = tTe[inds[i]];

						form[depth] = FORM_KOTSU;
						searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);
					}
	}

	if (n > 1)
	{
		// toitsu
		cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = HAI_EMPTY;

		for (i = 0; i <= n - 2; i++)
			if (tTe[inds[i]] == tTe[inds[i + 1]])
				if ((depth == 0) || (FORM_TOITSU < form[depth - 1]) || ((FORM_TOITSU == form[depth - 1]) && (tTe[inds[i]] >= cache[(depth - 1) * 3 + 0])))
					if (tTe[inds[i]] != cache[depth * 3 + 0])
					{
						nextN = 0;
						for (l = 0; l < n; l++)
						if ((l != i) && (l != i + 1))
							nextInds[nextN++] = inds[l];

						cache[depth * 3 + 0] = tTe[inds[i]];
						cache[depth * 3 + 1] = tTe[inds[i]];
						cache[depth * 3 + 2] = HAI_EMPTY;

						form[depth] = FORM_TOITSU;
						searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);
					}

		// ryanmen (shupai only)
		cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = HAI_EMPTY;

		for (i = 0; i <= n - 2; i++)
			if (tTe[inds[i]] > HAI_9S)
				break;
			else
				for (j = i + 1; (j <= i + 4) && (j < n); j++)
					if ((tTe[inds[j]] == tTe[inds[i]] + 1) && (tTe[inds[j]] / 9 == tTe[inds[i]] / 9))
						if ((depth == 0) || (FORM_RYANMEN < form[depth - 1]) || ((FORM_RYANMEN == form[depth - 1]) && (tTe[inds[i]] >= cache[(depth - 1) * 3 + 0])))
							if ((tTe[inds[j]] != cache[depth * 3 + 1]) || (tTe[inds[i]] != cache[depth * 3 + 0]))
							{
								nextN = 0;
								for (l = 0; l < n; l++)
								if ((l != i) && (l != j))
									nextInds[nextN++] = inds[l];

								cache[depth * 3 + 0] = tTe[inds[i]];
								cache[depth * 3 + 1] = tTe[inds[j]];
								cache[depth * 3 + 2] = HAI_EMPTY;

								form[depth] = FORM_RYANMEN;
								searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);
							}


		// kanchan (shupai only)
		cache[depth * 3 + 0] = cache[depth * 3 + 1] = cache[depth * 3 + 2] = HAI_EMPTY;

		for (i = 0; i <= n - 2; i++)
			if (tTe[inds[i]] > HAI_9S)
				break;
			else
				for (j = i + 1; (j <= i + 8) && (j < n); j++)
					if ((tTe[inds[j]] == tTe[inds[i]] + 2) && (tTe[inds[j]] / 9 == tTe[inds[i]] / 9))
						if ((depth == 0) || (FORM_KANCHAN < form[depth - 1]) || ((FORM_KANCHAN == form[depth - 1]) && (tTe[inds[i]] >= cache[(depth - 1) * 3 + 0])))
							if ((tTe[inds[j]] != cache[depth * 3 + 1]) || (tTe[inds[i]] != cache[depth * 3 + 0]))
							{
								nextN = 0;
								for (l = 0; l < n; l++)
								if ((l != i) && (l != j))
									nextInds[nextN++] = inds[l];

								cache[depth * 3 + 0] = tTe[inds[i]];
								cache[depth * 3 + 1] = tTe[inds[j]];
								cache[depth * 3 + 2] = HAI_EMPTY;

								form[depth] = FORM_KANCHAN;
								searchForm(tTe, nHai, nNaki, nextInds, depth + 1, form, cache, machi, hairi);
							}
	}

	t = d = toi = 0;
	for (i = 0; i < depth; i++)
	{
		switch (form[i])
		{
		case FORM_TOITSU:
			toi++;
		case FORM_RYANMEN:
		case FORM_KANCHAN:
			d++;
			break;
		case FORM_SHUNTSU:
		case FORM_KOTSU:
			t++;
			break;
		}
	}

    // toitsu present: 2(4-n-t)-min(d-1,4-n-t)-1
    // no toitsu: 2(4-n-t)-min(d,4-n-t)

    int toiPresent = (toi != 0);
    int nt = 4 - nNaki - t;

    if(d - toiPresent < nt)
        smaller = d - toiPresent;
    else
        smaller = nt;
    shanten = (nt << 1) - smaller - toiPresent;


	// this is used to weed out insignificant interpretations 
	// e.g., breaking up 34 as 3 + 4 when doing so doesn't affect shanten
	significance = (t << 1) + d;

	if (shanten < machi[IND_NSHANTEN]) // new minimum
	{
		machi[IND_NSHANTEN] = shanten;
		if (machi != NULL)
			for (i = 0; i < 34; i++)
				machi[i] = 0;
		if (hairi != NULL)
			hairi[IND_NHAIRI] = 0;
	}

	if ((machi != NULL) && (shanten == machi[IND_NSHANTEN]) && (significance >= cache[21])) // new minimum or same as previous minimum
	{
		cache[21] = significance;

		for (i = 0; i < depth; i++)
		{
			switch (form[i])
			{
            case FORM_TOITSU:
                // if (shanten == 0) && (toi != 2), this cannot be a machi (this case is subsumed by below)
                // or if (toi == 1) && d-1 >= nt, this cannot be a machi
                if ((toi != 1) || (d - 1 < nt))
					machi[cache[3 * i + 0]]++;
				break;
			case FORM_RYANMEN:
				tempHai = cache[3 * i + 0];
				if ((tempHai != HAI_1M) && (tempHai != HAI_1P) && (tempHai != HAI_1S))
					machi[tempHai - 1]++;
				tempHai = cache[3 * i + 1];
				if ((tempHai != HAI_9M) && (tempHai != HAI_9P) && (tempHai != HAI_9S))
					machi[tempHai + 1]++;
				// if (!toi) && (d-1 >= 4-n-t), ryanmen can reduce shanten by becoming toitsu
                if ((!toi) && (d - 1 >= nt)) {
					machi[cache[3 * i + 0]]++;
					machi[cache[3 * i + 1]]++;
				}
				break;
			case FORM_KANCHAN:
				machi[cache[3 * i + 0] + 1]++;
				// if (!toi) && (d-1 >= 4-n-t), kanchan can reduce shanten by becoming toitsu
                if ((!toi) && (d - 1 >= nt)) {
					machi[cache[3 * i + 0]]++;
					machi[cache[3 * i + 1]]++;
				}
				break;
			}
		}

        // tanki -> toitsu condition
		// if (toi) && (d - 1 < 4 - n - t) or
        // if (!toi) && (d <= 4 - n - t), then getting a toitsu reduces shanten (note <=)
		// i.e., if (((toi) && (d - 1 < 4 - nNaki - t)) || ((!toi) && (d <= 4 - nNaki - t)) || (4 - nNaki - t == 0))
		// can be simplified to:
        if (d <= 4 - nNaki - t)
			for (i = 0; i < n; i++)
				machi[tTe[inds[i]]]++;

		// tanki -> kanchan or tanki -> ryanmen/penchan condition
		// d - (toi > 0) < 4 - n - t
        if (d - toiPresent < nt)
		{
			for (i = 0; i < n; i++)
			{
				if (tTe[inds[i]] >= HAI_TON)
					break;

				if ((tTe[inds[i]] % 9) >= 2)
					machi[tTe[inds[i]] - 2]++;
				if ((tTe[inds[i]] % 9) >= 1)
					machi[tTe[inds[i]] - 1]++;
				if ((tTe[inds[i]] % 9) <= 7)
					machi[tTe[inds[i]] + 1]++;
				if ((tTe[inds[i]] % 9) <= 6)
					machi[tTe[inds[i]] + 2]++;
			}

		}

		if (hairi != NULL)
		{
			curPos = 0;
			for (i = 0; i < depth; i++)
			{
				switch (form[i])
				{
				case FORM_SHUNTSU:
				case FORM_KOTSU:
					k = 3;
					break;
				case FORM_TOITSU:
				case FORM_RYANMEN:
				case FORM_KANCHAN:
					k = 2;
					break;
                default: // this should not happen
                    k = 1;
                    break;
				}
				for (j = 0; j < k; j++)
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + j] = cache[3 * i + j];
				curPos += k;
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + i] = form[i];
			}
			if (depth != 6)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + depth] = FORM_TANKI;
			if (n == 0)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + 0] = HAI_EMPTY;
			for (j = 0; j < n; j++)
				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + curPos + j] = tTe[inds[j]];
			hairi[IND_NHAIRI]++;
		}
	}

	return 0;
}*/

void GameThread::shanten(const int *te, int* machi, int* hairi)
// Ignores te[IND_TSUMOHAI] and calculates shanten
// Does not assume sorted tehai (it will sort inside)
// Pass hairi = NULL if you don't need it
{
	int nHai, nNaki, i, j, cur, shanten, nSolo;
	int tTe[13];
	int depth, form[6], inds[13], cache[22]; // cache 0~17 : slot for max 6 forms, 18~20 : buffer slots, 21 : slot for previous significance
	int toiList[13], skipList[13], toitsu;
	int ycCheck[34], yc, yctoi;


	nNaki = te[IND_NAKISTATE] & MASK_TE_NNAKI;
	nHai = 13 - 3 * nNaki;
	// naki hais start at 13 - 3 * nNaki, up to right before 13

	machi[IND_NSHANTEN] = 6; // max shanten is 6 because of chitoi

	if (machi != NULL)
        for (i = 0; i < 34; i++)
            machi[i] = 0;

	if (hairi != NULL)
		hairi[IND_NHAIRI] = 0;

	for (i = 0; i < nHai; i++)
		tTe[i] = te[i] & MASK_HAI_NUMBER; // remove akadora flag, copy to a temporary te


	///////////////// Chitoi form ////////////////////
	// No naki
	// shanten = 6 - toitsu
	// toitsu cannot overlap
	//////////////////////////////////////////////////
	if (!nNaki)
	{
		int exception = 0;

		toitsu = 0;
		for (i = 0; i < 13; i++)
		{
			toiList[i] = HAI_EMPTY;
			skipList[i] = 0;
			cache[i] = 0;
		}


		for (i = 0; i < nHai - 1; i++)
		{
			if (skipList[i])
				continue;
			for (j = i + 1; j < nHai; j++)
			{
				if (tTe[i] == tTe[j])
				{
					cur = tTe[i];
					if ((cur != toiList[0]) && (cur != toiList[1]) && (cur != toiList[2]) && (cur != toiList[3]) && (cur != toiList[4]))
						toiList[toitsu++] = cur;
					skipList[j]++;
					cache[i] = cache[j] = 1;
					break;
				}
			}
		}

		shanten = 6 - toitsu;
		// exception: 6 toitsu + 1 hai that overlaps with any of the toitsu
		if (shanten == 0)
		{
			for (i = 0; i < nHai; i++)
				if (!cache[i]) {
					cur = tTe[i];
					if ((cur == toiList[0]) || (cur == toiList[1]) || (cur == toiList[2]) || (cur == toiList[3]) || (cur == toiList[4]) || (cur == toiList[5])) {
						shanten++;
						exception = 1;
						break;
					}
				}
		}

		if (shanten < machi[IND_NSHANTEN]) // new minimum
		{
			machi[IND_NSHANTEN] = shanten;
			if (machi != NULL)
			for (i = 0; i < 34; i++)
				machi[i] = 0;
			if (hairi != NULL)
				hairi[IND_NHAIRI] = 0;
		}

		if ((machi != NULL) && (shanten == machi[IND_NSHANTEN])) // new minimum or same as old minimum
		{
			if (exception) // all hais except those you have now are machi's 
			{
				for (i = HAI_1M; i <= HAI_CHUN; i++)
					if ((i != toiList[0]) && (i != toiList[1]) && (i != toiList[2]) && (i != toiList[3]) && (i != toiList[4]) && (i != toiList[5]))
						machi[i]++;
			}
			else
			{
				for (i = 0; i < nHai; i++)
				{
					cur = tTe[i];
					if ((cur != toiList[0]) && (cur != toiList[1]) && (cur != toiList[2]) && (cur != toiList[3]) && (cur != toiList[4]) && (cur != toiList[5]))
						machi[cur]++;
				}
			}
			if (hairi != NULL)
			{
				for (i = 0; i < toitsu; i++)
				{
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + (i << 1) + 0] = toiList[i];
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + (i << 1) + 1] = toiList[i];
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + i] = FORM_TOITSU;
				}
				if (toitsu != 6)
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14 + toitsu] = FORM_TANKI;
				nSolo = 0;
				for (i = 0; i < nHai; i++)
				{
					cur = tTe[i];
					//if ((cur != toiList[0]) && (cur != toiList[1]) && (cur != toiList[2]) && (cur != toiList[3]) && (cur != toiList[4]) && (cur != toiList[5]))
					if (!cache[i])
						hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + (toitsu << 1) + nSolo++] = cur;
				}
				hairi[IND_NHAIRI]++;
			}
		}

	}


	///////////////// Kokushi form ///////////////////
	// No naki
	// yc = # of kinds of yaochuhai
	// yctoi = whether there is a yaochuhai toitsu or not
	// shanten = 13 - (yc + (yctoi > 0))
	//////////////////////////////////////////////////
	if (!nNaki)
	{
		// initialize
		yc = 0;
		yctoi = 0;
		for (i = 0; i < 34; i++)
			ycCheck[i] = 0;

		for (i = 0; i < nHai; i++)
		{
			cur = tTe[i];
			if ((cur == HAI_1M) || (cur == HAI_9M) || (cur == HAI_1P) || (cur == HAI_9P) || (cur == HAI_1S) || (cur == HAI_9S) || \
				((cur >= HAI_TON) && (cur <= HAI_CHUN)) )
			{
				ycCheck[cur]++;
				yctoi += (!yctoi) && (ycCheck[cur] > 1); // only increase yctoi once
				cache[i] = 1;
			}
			else
				cache[i] = 0;
		}

		for (i = 0; i < 34; i++)
			yc += (ycCheck[i] > 0); // count # of kinds of yaochuhai

		shanten = 13 - yc - yctoi;


		if (shanten < machi[IND_NSHANTEN]) // new minimum
		{
			machi[IND_NSHANTEN] = shanten;
			if (machi != NULL)
				for (i = 0; i < 34; i++)
					machi[i] = 0;
			if (hairi != NULL)
				hairi[IND_NHAIRI] = 0;
		}

		if ((machi != NULL) && (shanten == machi[IND_NSHANTEN])) // new minimum or same as old minimum
		{
			for (i = 0; i < 34; i++)
				if ((i == HAI_1M) || (i == HAI_9M) || (i == HAI_1P) || (i == HAI_9P) || (i == HAI_1S) || (i == HAI_9S) || \
					((i >= HAI_TON) && (i <= HAI_CHUN)) )
					machi[i] += (!yctoi) || (!ycCheck[i]); // yctoi = 0 : all yaochuhai; yctoi = 1 : only those you don't have
			if (hairi != NULL)
			{
				yc = 0;
				for (i = 0; i < nHai; i++)
				if (cache[i])
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + yc++] = tTe[i];
				nSolo = 0;
				for (i = 0; i < nHai; i++)
				if (!cache[i])
					hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + yc + nSolo++] = tTe[i];

				hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14] = FORM_TANKI;
				hairi[IND_NHAIRI]++;
			}
		}
	}

	///////////////// General form ///////////////////
	// n = # of naki
	// t = shuntsu + kotsu (kantsu)
	// d = ryanmen (penchan) + kanchan + toitsu
	// shanten = 2 * (4 - n - t) - min(d, 4 - n - t)   (no toitsu)
    // shanten = 2 * (4 - n - t) - min(d - 1, 4 - n - t) - 1   (with toitsu)
	// toitsu cannot overlap
	//////////////////////////////////////////////////
	if (nNaki == 4)
	{
		machi[IND_NSHANTEN] = 0;
		if (machi != NULL)
			machi[tTe[0]]++;
		if (hairi != NULL)
		{
			hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 0] = tTe[0];
			hairi[hairi[IND_NHAIRI] * IND_HAIRISIZE + 14] = FORM_TANKI;
			hairi[IND_NHAIRI]++;
		}
	}
	else
	{
		sortHai(tTe, nHai);
		depth = 0;
		for (i = 0; i < 6; i++)
			form[i] = 0;
		for (i = 0; i < nHai; i++)
			inds[i] = i;
		
		cache[21] = 0; // last significance
		// recursive function to search general form
		searchForm(tTe, nHai, nNaki, inds, depth, form, cache, machi, hairi);
	}

    return;
}
