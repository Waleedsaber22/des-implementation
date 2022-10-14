#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>

#ifdef __linux__

#include <time.h>

double time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#else
#include <Windows.h>

double time_sec() {
    LARGE_INTEGER li = {0};
    QueryPerformanceFrequency(&li);
    long long t = li.QuadPart;
    QueryPerformanceCounter(&li);
    return (double) li.QuadPart / t;
}

#endif

typedef unsigned long long u_64;

/*============================================*/
u_64 permutation(int perm[], u_64 target, int actualSize, int targetSize) {
    u_64 out = 0;
    for (int i = 0; i < targetSize; ++i)
        out |= (target >> actualSize - perm[targetSize - 1 - i] & 1) << i;
    return out;
}

/*============================================*/
u_64 sBox(int sBox[8][64], u_64 target, int round) {
    u_64 res = 0;
    for (int i = 0; i < 8; i++) {
        u_64 target6 = target >> i * 6 & 0x3F;            // get each 1 byte
        int index = target6 >> 1 & 0x0F | (target6 & 1) << 4 | target6 & 0x20; // rows * cols = index
        res |= sBox[7 - i][index] << i * 4;
    }
    return res;
}
/*============================================*/
/* convert binary to binary text*/
/*============================================*/
char *binaryToTbinary(u_64 c, int nBits) {
    char *binaryString = malloc(nBits);
    binaryString[0] = '\0';
    for (int j = nBits - 1; j >= 0; j--) {
        if (c & ((u_64) 1 << j)) strcat(binaryString, "1");
        else strcat(binaryString, "0");
    }
    return binaryString;
}
/*===========================================*/
/* to convert hex text to binary*/
/*===========================================*/
char* numberToText(u_64 num){
char* str = malloc(8*8+1);
str[8] = '\0'; 
for(int i = 0 ; i < 8; i ++ ){
str[7-i] = num >> i*8 & 0xFF;
}
return str;
}

/*===========================================*/
/*===========================================*/
/* to convert hex text to binary*/
/*===========================================*/
u_64 hexToInt(char c) {
    return c < 65 ? c - 48 : c - 55;
}

u_64 hexToBinary(char *str, int numHex) {
    u_64 target = 0;
    for (int i = 0; i < numHex; i++) {
        target |= hexToInt(str[numHex - 1 - i]) << 4 * i;
    }
    return target;
}

/*===========================================*/
u_64 *splitText(u_64 target) {
    u_64 *splittedText = malloc(2 * 64);
    splittedText[0] = target >> 32;
    splittedText[1] = target & 0xFFFFFFFF;
    return splittedText;
}

u_64 *splitKey(u_64 target) {
    u_64 *splittedKey = malloc(2 * 64);
    splittedKey[0] = target >> 28;
    splittedKey[1] = target & 0xFFFFFFF;
    return splittedKey;
}
/*================ Left Circular Shift =======================*/
/*===========================================*/
u_64 LCS(u_64 target, int bitsNum, int shiftNum) {
    u_64 res = 0;
    res = target << shiftNum | target >> bitsNum - shiftNum;
    return res;
}

u_64 concat(u_64 target1, u_64 target2) {
    return target1 << 28 | target2;
}

u_64 stringToBinary(char *pt, int nChar) {
    u_64 res = 0;
    for (int i = 0; i < nChar; i++) {
        res |= (u_64) pt[nChar - 1 - i] << 8 * i;
    }
    return res;
}
/*generate sub keys*/
/*===========================================*/
u_64 *subKeys(char *tkey, int mode) {
    /* convert key to binary*/
    u_64 key = 0;
    if (mode == 1) key = hexToBinary(tkey, 16); else key = stringToBinary(tkey, 8);
/* applying permutation choice 1 on key to discard 8 bits from 64bit */
    int choicePerm1[] = {
            57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34,
            26, 18, 10, 2, 59, 51, 43, 35, 27, 19, 11, 3,
            60, 52, 44, 36, 63, 55, 47, 39, 31, 23, 15, 7,
            62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37,
            29, 21, 13, 5, 28, 20, 12, 4
    };
    u_64 choiceKey = permutation(choicePerm1, key, 64, 56);
/* splitting key into halves */
    u_64 leftKey = splitKey(choiceKey)[0];
    u_64 rightKey = splitKey(choiceKey)[1];
    u_64 *roundKeys = malloc(64 * 16);
    int round = 1;
    while (round <= 16) {
/* then passing it to LCS */
        // Number of bit shifts
        int shift_table[16] = {
                1, 1, 2, 2, 2, 2, 2, 2,
                1, 2, 2, 2, 2, 2, 2, 1
        };
        leftKey = LCS(leftKey, 28, shift_table[round - 1]);
        rightKey = LCS(rightKey, 28, shift_table[round - 1]);
/* then generate subkey 48-bit by passing it to concatination permutation (choice 2) */
        u_64 totalKey = concat(leftKey, rightKey);
        int choicePerm2[] = {
                14, 17, 11, 24, 1, 5, 3, 28,
                15, 6, 21, 10, 23, 19, 12, 4,
                26, 8, 16, 7, 27, 20, 13, 2,
                41, 52, 31, 37, 47, 55, 30, 40,
                51, 45, 33, 48, 44, 49, 39, 56,
                34, 53, 46, 42, 50, 36, 29, 32
        };
        u_64 subKey = permutation(choicePerm2, totalKey, 56, 48);
        roundKeys[round - 1] = subKey;
        round++;
    }
    return roundKeys;
}

u_64 swap32(u_64 tLeft, u_64 tRight) {
    return tRight << 32 | tLeft;
}

u_64 XOR(u_64 t1, u_64 t2) {
    return t1 ^ t2;
}

/*===========================================*/
u_64 encrypt(u_64 text, u_64 *sKeys) {
    /* =====================================*/
    //           text plain side
    /* =====================================*/
/* first applying intial permutation on text */
    int initPerm[] = {
            58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
            62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
            57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
            61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7
    };
    u_64 iText = permutation(initPerm, text, 64, 64);
/*========================= Round 1 =============================

/* splitting 64bit to halves */
    u_64 leftText = splitText(iText)[0];
    u_64 rightText = splitText(iText)[1];

/* =====================================*/
    //           key plain side
/* =================== init data ==================*/
    int expPerm[48] = {
            32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
            8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
            16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
            24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1
    };
    int s[8][64] = {
            {14, 4,  13, 1,  2,  15, 11, 8,  3,  10, 6, 12, 5,
                                                                9,  0,  7,  0,  15, 7,  4,  14, 2,  13, 1,  10, 6,
                                                                                                                   12, 11, 9,  5,  3,  8, 4,  1,  14, 8,  13, 6,  2,
                                                                                                                                                                     11, 15, 12, 9,  7,  3,  10, 5,  0,  15, 12, 8,  2,
                                                                                                                                                                                                                        4,  9,  1,  7,  5,  11, 3,  14, 10, 0, 6,  13},
            {15, 1,  8,  14, 6,  11, 3,  4,  9,  7,  2, 13, 12,
                                                                0,  5,  10, 3,  13, 4,  7,  15, 2,  8,  14, 12, 0,
                                                                                                                   1,  10, 6,  9,  11, 5, 0,  14, 7,  11, 10, 4,  13,
                                                                                                                                                                     1,  5,  8,  12, 6,  9,  3,  2,  15, 13, 8,  10, 1,
                                                                                                                                                                                                                        3,  15, 4,  2,  11, 6,  7,  12, 0,  5, 14, 9},

            {10, 0,  9,  14, 6,  3,  15, 5,  1,  13, 12,
                                                        7,  11, 4,  2,  8,  13, 7,  0,  9,  3,  4,
                                                                                                    6,  10, 2,  8, 5,  14, 12, 11, 15, 1, 13,
                                                                                                                                              6,  4,  9,  8,  15, 3, 0,  11, 1,  2,  12,
                                                                                                                                                                                         5,  10, 14, 7,  1,  10, 13, 0, 6,  9,  8,
                                                                                                                                                                                                                                    7,  4,  15, 14, 3,  11, 5, 2,  12},
            {7,  13, 14, 3,  0,  6,  9,  10, 1,  2,  8, 5,  11,
                                                                12, 4,  15, 13, 8,  11, 5,  6,  15, 0,  3,  4,  7,
                                                                                                                   2,  12, 1,  10, 14, 9, 10, 6,  9,  0,  12, 11, 7,
                                                                                                                                                                     13, 15, 1,  3,  14, 5,  2,  8,  4,  3,  15, 0,  6,
                                                                                                                                                                                                                        10, 1,  13, 8,  9,  4,  5,  11, 12, 7, 2,  14},
            {2,  12, 4,  1,  7,  10, 11, 6,  8,  5,  3, 15, 13,
                                                                0,  14, 9,  14, 11, 2,  12, 4,  7,  13, 1,  5,  0,
                                                                                                                   15, 10, 3,  9,  8,  6, 4,  2,  1,  11, 10, 13, 7,
                                                                                                                                                                     8,  15, 9,  12, 5,  6,  3,  0,  14, 11, 8,  12, 7,
                                                                                                                                                                                                                        1,  14, 2,  13, 6,  15, 0,  9,  10, 4, 5,  3},
            {12, 1,  10, 15, 9,  2,  6,  8,  0,  13, 3, 4,  14,
                                                                7,  5,  11, 10, 15, 4,  2,  7,  12, 9,  5,  6,  1,
                                                                                                                   13, 14, 0,  11, 3,  8, 9,  14, 15, 5,  2,  8,  12,
                                                                                                                                                                     3,  7,  0,  4,  10, 1,  13, 11, 6,  4,  3,  2,  12,
                                                                                                                                                                                                                        9,  5,  15, 10, 11, 14, 1,  7,  6,  0, 8,  13},
            {4,  11, 2,  14, 15, 0,  8,  13, 3,  12, 9, 7,  5,
                                                                10, 6,  1,  13, 0,  11, 7,  4,  9,  1,  10, 14, 3,
                                                                                                                   5,  12, 2,  15, 8,  6, 1,  4,  11, 13, 12, 3,  7,
                                                                                                                                                                     14, 10, 15, 6,  8,  0,  5,  9,  2,  6,  11, 13, 8,
                                                                                                                                                                                                                        1,  4,  10, 7,  9,  5,  0,  15, 14, 2, 3,  12},
            {13, 2,  8,  4,  6,  15, 11, 1,  10, 9,  3, 14, 5,
                                                                0,  12, 7,  1,  15, 13, 8,  10, 3,  7,  4,  12, 5,
                                                                                                                   6,  11, 0,  14, 9,  2, 7,  11, 4,  1,  9,  12, 14,
                                                                                                                                                                     2,  0,  6,  10, 13, 15, 3,  5,  8,  2,  1,  14, 7,
                                                                                                                                                                                                                        4,  10, 8,  13, 15, 12, 9,  0,  3,  5, 6,  11}
    };
    int stPerm[32]
            = {16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23,
               26, 5, 18, 31, 10, 2, 8, 24, 14, 32, 27,
               3, 9, 19, 13, 30, 6, 22, 11, 4, 25};
    int finalPerm[64]
            = {40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47,
               15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22,
               62, 30, 37, 5, 45, 13, 53, 21, 61, 29, 36,
               4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11,
               51, 19, 59, 27, 34, 2, 42, 10, 50, 18, 58,
               26, 33, 1, 41, 9, 49, 17, 57, 25};
/*===========================================================*/
    int round = 1;
    while (round <= 16) {
/*===============================================*/
/*===============================================*/
/*===============================================*/
        u_64 prevRightText = rightText;
/* applying expanding permutation to get 48bits in right text split*/
        u_64 expansionText = permutation(expPerm, rightText, 32, 48);
/* applying XOR to subkey1 and output of expansion permutation in round 1 */
        u_64 output48 = XOR(expansionText, sKeys[round - 1]);
        u_64 output32 = sBox(s, output48, round);
        /* again apply it to straight permuntation */
        output32 = permutation(stPerm, output32, 32, 32);
/* applying XOR to both output32 and leftText1 */
        rightText = XOR(leftText, output32);
/* so we done  from first round and now we have inputs to start new round till 16*/
        leftText = prevRightText;
        /* display roundNumber, leftText, rightText, subKey*/
    // if(round < 16){
    // printf("Round %d | %X | %X | %p\n",round,leftText,rightText,sKeys[round-1]);
    // }else{
    // printf("Round %d | %X | %X | %p\n",round,rightText,leftText,sKeys[round-1]);
    // }
        round++;
    }
/* it's final permuntation and before it swap 32 bit left32 (16) right32 (16)*/
    u_64 swappedTarget = swap32(leftText, rightText);
    u_64 cypherText = permutation(finalPerm, swappedTarget, 64, 64);
    return cypherText;
}

u_64 *reverse(u_64 *keys) {
    u_64 *rKeys = malloc(64 * 16);
    for (int i = 0; i < 16; i++) {
        rKeys[i] = keys[15 - i];
    }
    return rKeys;
}

u_64 encryptText(char *ptext, u_64 *sKeys, int mode, int charNum) {
    u_64 text = 0;
    if (mode == 1) text = hexToBinary(ptext, charNum); else text = stringToBinary(ptext, charNum);
    encrypt(text, sKeys);
}

u_64 decryptText(u_64 cypher, u_64 *rKeys) {
    u_64 pText = encrypt(cypher, rKeys);
    return pText;
}

void handleMode(int *mode, int *chMode) {
    printf("\n[1] press 1 to encrypt hex text file\n");
    printf("[2] press 2 to encrypt text plain file\n");
    *chMode = 1;
    scanf("%d", mode);
}
int main(int argc, char *argv[]) {
    int k[] = {3, 4, 1, 8};
    char* key = malloc(16*8+8); // max key length
    key = "0E329232EA6D0D73";
    FILE* pFile = fopen("key.txt","w");
    fprintf(pFile,"%s",key);
    fclose(pFile);
    u_64 *sKeys = subKeys(key, 1); // mode 1 -->(hex) else (plain text)
    u_64 *rKeys = reverse(sKeys);
    FILE *ptrFile = fopen(argv[1], "r");
    if (ptrFile == NULL) {
        char *absPath = malloc(200 * 8);
        printf("write path to your file you want to encrypt: ");
        fgets(absPath, 100, stdin);
        absPath[strlen(absPath) - 1] = '\0';
        ptrFile = fopen(absPath, "r");
        if (ptrFile == NULL) {
            printf("cannot find file please make sure to add file path");
            return 0;
        }
    }
    int mode;
    int chMode = 1;
    handleMode(&mode, &chMode);
    if (mode != 1 && mode != 2) return 0;
    u_64 *buffer = malloc((u_64)268435456 * 64); // size : up to 2GB
    int dataNum = 0;
    int bitsPerC;
    int charNums;
    char *fileName = malloc(100 * 8);
    while (1) {
        printf("\n*******************************************************");
        printf("\n[1] press 1 to encrypt file\n");
        printf("[2] press 2 to decrypt file\n");
        printf("[3] press 3 to convert file type\n");
        printf("[4] press 4 to exit\n");
        int res;
        scanf("%d", &res);
        printf("*******************************************************\n");
        int charFile;
        double time1 = time_sec();
        double time2;
        switch (res) {
            case 1: // encrypt text
                if (res == 1) {
                    if(mode == 1) fileName =  "encrypted_hex.dat";
                    else fileName = "encrypted_text.dat";
                    FILE* pFile = fopen(fileName,"w");
                    int charNum = 0;
                    dataNum = 0;
                    chMode = 0;
                    charNums = mode == 1 ? 16 : 8; // char needed to full 64bit
                    bitsPerC = mode == 1 ? 4 : 8; // bits for each char
                    char *data = malloc(charNums + 1);
                    data[charNums] = '\0';
                    rewind(ptrFile);
                    while (1) {
                        charFile = fgetc(ptrFile);
                        if (charFile == EOF) {
                            if (charNum != 0) { // last bytes in text
                                u_64 encText = encryptText(data, sKeys, mode, charNum);
                                buffer[dataNum] = encText;
                                dataNum++;
                                printf("%p", encText);
                                fprintf(pFile,"%p",encText);
                                 /*to show cypher text as text or hex*/
                                //  if(mode == 1){
                                // fprintf(pFile,"%p",encText);
                                // printf("%p", encText);
                                // }
                                // else{
                                // char* text = numberToText(encText);
                                // fprintf(pFile,"%s",text);
                                // printf("%s", text);
                                // }  
                                // printf("cypher: %p", encText);
                            }
                            break;
                        }
                        if (mode == 1 && charFile == '\n') continue;
                        charNum++;
                        data[charNum - 1] = charFile;
                        if (charNum % charNums == 0) {
                            u_64 encText = encryptText(data, sKeys, mode, charNum); // encrypt each 8 byte in text
                            buffer[dataNum] = encText;
                            printf("%p", encText);
                            fprintf(pFile,"%p",encText);
                            /*to show cypher text as text or hex*/
                                //  if(mode == 1){
                                // fprintf(pFile,"%p",encText);
                                // printf("%p", encText);
                                // }
                                // else{
                                // char* text = numberToText(encText);
                                // fprintf(pFile,"%s",text);
                                // printf("%s", text);
                                // }  
                            // printf("cypher: %p", encText);
                            dataNum++;
                            charNum = 0;
                        }
                    }
                 fclose(pFile);
                 fopen(fileName,"r");
                 printf("\n Out: %s", fileName);
                }
                time2 = time_sec();
                printf("\n Time = %lfs", time2 - time1);
                break;
            case 2:
                if(mode == 1) fileName =  "decrypted_hex.txt";
                    else fileName = "decrypted_text.txt";
                    FILE* pFile = fopen(fileName,"w");
                if (dataNum == 0 || chMode == 1) {
                    printf("please encrypt data first");
                } else {
                    for (int i = 0; i < dataNum; i++) {
                        u_64 decryptedData = decryptText(buffer[i], rKeys); // decrypt each 8 byte in text
                        if(mode == 1){
                        fprintf(pFile,"%p",decryptedData);
                        printf("%p", decryptedData);
                        }
                        else{
                        char* text = numberToText(decryptedData);
                        fprintf(pFile,"%s",text);
                        printf("%s", text);
                        }  
                    }
                fclose(pFile);
                fopen(fileName,"r");
                printf("\n Out: %s", fileName);
                time2 = time_sec();
                printf("\n Time = %lfs", time2 - time1);
            }
                break;
            case 3:
                handleMode(&mode, &chMode);
                if (mode != 1 && mode != 2) return 0;
                time2 = time_sec();
                printf("\n Time = %lfs", time2 - time1);
                break;
            case 4:
                return 0;
        }
    }
    return 0;
}
