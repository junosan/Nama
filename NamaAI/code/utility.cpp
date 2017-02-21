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

#include "namaai.h"

void countVisible(const AiData *pD, int* hai)
// hai[34] stores how many of each hai (HAI_1M ~ HAI_CHUN) is visible to this player
{
    int i, j, nNaki, nKawa, nakiType;
    const int* te = pD->te;
    const int *p;
    const int* kawa = pD->kawa;
    const int* naki = pD->naki;
    const int* dorahyouji = pD->dorahyouji;

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

    return;
}


int sortPartition(int* array, int left, int right, int pivotIndex)
// Partition function for quicksort
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

void sortHai(int* arr, int n)
// Nonrecursive quicksort (n <= 14)
{
    int pivotIndex;
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


void searchForm(const int* tTe, int nHai, int nNaki, const int* inds, int depth, int* form, int* cache, int* machi, int* hairi)
// Internal recursive function for finding machi & hairi
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

void shanten(const int *te, int* machi, int* hairi)
// Calculates shanten with (13 - 3 * nNaki) number of hais (ignores IND_TSUMOHAI)
// No need to supply sorted te; it will sort inside
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

