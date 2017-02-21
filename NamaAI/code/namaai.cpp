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
#include <time.h>

#include "namaai.h"

extern "C" NAMAAI_API void initialize()
{
    init_genrand((unsigned long)time(NULL));
}

extern "C" NAMAAI_API AiAnswer askAction(AiData d)
{
    int i, nOption;
    AiAnswer ans;
    ans.action = ACTION_PASS;
    ans.optionChosen = 0;

    int firstAction = -1;
    for (int i = 0; i < IND_ACTIONAVAILSIZE; i++)
        if (d.actionAvail[i]) {
            firstAction = i;
            break;
        }

    if (firstAction == -1)
        return ans;

    // first half (tsumoho, kyushukyuhai, ankan, kakan, richi, dahai)
    if (firstAction <= ACTION_DAHAI)
    {
        SLEEP_MS(500);

        if (d.actionAvail[ACTION_TSUMOHO]) {
            ans.action = ACTION_TSUMOHO;
            return ans;
        }

        if (d.actionAvail[ACTION_KYUSHUKYUHAI]) {
            ans.action = ACTION_KYUSHUKYUHAI;
            return ans;
        }

        if (d.actionAvail[ACTION_ANKAN]) {
            ans.action = ACTION_ANKAN;
            ans.optionChosen = 0;
            return ans;
        }

        if (d.actionAvail[ACTION_KAKAN]) {
            ans.action = ACTION_KAKAN;
            ans.optionChosen = 0;
            return ans;
        }

        if (d.actionAvail[ACTION_RICHI]) {
            ans.action = ACTION_RICHI;
            ans.optionChosen = 0;
            return ans;
        }

        if (d.actionAvail[ACTION_DAHAI]) {

            if (d.te[IND_NAKISTATE] & FLAG_TE_RICHI)
            {
                // only one option will be given anyways
                ans.action = ACTION_DAHAI;
                ans.optionChosen = 0;
                return ans;
            }

            for (i = 0; i <= IND_TSUMOHAI; i++)
                if (d.dahaiInds[i] == -1)
                    break;
            nOption = i;

            ans.optionChosen = genrand_int32() % nOption;

            // (checking premitive optimal dahai)
            int hai[34], maxMachi, tMachi, maxInds[15], nMaxInds, tTe[15], machi[IND_MACHISIZE], minShanten, tShanten;

            countVisible(&d, hai);

            // try throwing away one hai from 0~IND_TSUMOHAI and see how wide machi is
            maxMachi = nMaxInds = 0;
            minShanten = 6;
            for (i = 0; i <= IND_TSUMOHAI; i++)
                tTe[i] = d.te[i] & MASK_HAI_NUMBER;
            tTe[IND_NAKISTATE] = d.te[IND_NAKISTATE];
            for (i = 0; i < nOption; i++)
            {
                tTe[d.dahaiInds[i]] = d.te[IND_TSUMOHAI] & MASK_HAI_NUMBER;

                // check machi
                shanten(tTe, machi, NULL);

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

                tTe[d.dahaiInds[i]] = d.te[d.dahaiInds[i]] & MASK_HAI_NUMBER;
            }
            if (nMaxInds)
                ans.optionChosen = maxInds[nMaxInds - 1];

            ans.action = ACTION_DAHAI;
            return ans;
        }
    }
    // second half (ron, daiminkan, pon, chi)
    else
    {
        int sutehai = d.kawa[IND_KAWASIZE * d.curDir + d.kawa[IND_KAWASIZE * d.curDir + IND_NKAWA] - 1];

        if (d.actionAvail[ACTION_RONHO]) {
            SLEEP_MS(500);
            ans.action = ACTION_RONHO;
            return ans;
        }

        if (d.actionAvail[ACTION_DAIMINKAN]) {
//            SLEEP_MS(500);
//            ans.action = ACTION_DAIMINKAN;
//            return ans;
        }

        if (d.actionAvail[ACTION_PON]) {
            // only pon yakuhais
            if (((sutehai & MASK_HAI_NUMBER) == HAI_HAKU) || ((sutehai & MASK_HAI_NUMBER) == HAI_HATSU) || ((sutehai & MASK_HAI_NUMBER) == HAI_CHUN) || \
                    ((sutehai & MASK_HAI_NUMBER) == d.chanfonpai) || ((sutehai & MASK_HAI_NUMBER) == HAI_TON + d.myDir)) {
                SLEEP_MS(500);
                ans.action = ACTION_PON;
                ans.optionChosen = 0;
                return ans;
            }
        }

        if (d.actionAvail[ACTION_CHI]) {
//            SLEEP_MS(500);
//            ans.action = ACTION_CHI;
//            return ans;
        }

    }

    return ans;
}
