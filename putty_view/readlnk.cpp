#include "iostream"
#include "stdio.h"
#include "iomanip"
#include "stdlib.h"
#include "cstring"
#include "windows.h"
using namespace std;

#define MAXSIZE 256

int main(void)
{
    //将文件读取进来
    FILE* pFile = NULL;
    pFile = fopen("VMware Player.lnk", "rb");
    if(!pFile){
        cerr<<"error while open";
        exit(-1);
    }
    long iLow;
    long iHigh;
    long iPathLen;
    int iHaveSil = 0;
    //前进到14h处, 判断是否有shell item list
    fseek(pFile, 20L, SEEK_CUR);
    iLow = fgetc(pFile);
    if(iLow & 1 == 1){
        iHaveSil = 1;
    }
    //前进到4ch处, 找到sil
    fseek(pFile, 76L, SEEK_SET); 
    iLow = fgetc(pFile);
    iHigh = fgetc(pFile);
    //前进到文件位置信息的地方
    fseek(pFile, iLow, SEEK_CUR);
    int i = 0;
    for(i = 0; i < iHigh; i += 1){
        fseek(pFile, 256L, SEEK_CUR);
    }
    //得到本地文件信息的偏移
    fseek(pFile, 16L, SEEK_CUR);
    iLow = fgetc(pFile);
    //得到附加信息的偏移
    fseek(pFile, 7L, SEEK_CUR);
    iHigh = fgetc(pFile);
    //得到路径的长度
    iPathLen = iHigh - iLow;
    char PathInfo[MAXSIZE];
    //行进到本地路径信息
    fseek(pFile, -25L, SEEK_CUR);
    fseek(pFile, iLow, SEEK_CUR);
    //获得路径信息
    for(i = 0; i < iPathLen; i += 1){
        PathInfo[i] = fgetc(pFile);
        //如果取完, 就停止
        if(PathInfo[i] == 0){
         PathInfo[i] = '\0';
         break;
        }
    }
    //过滤出具体路径信息
    cout << PathInfo << endl;
    for(i = strlen(PathInfo)-1; i>=0; i -= 1){
        if(PathInfo[i] == '\\')
         break;
        else PathInfo[i] = '\0';
    }
    cout << PathInfo << endl;
    fclose(pFile);
    return EXIT_SUCCESS;
}