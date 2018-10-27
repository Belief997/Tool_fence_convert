#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#include "cJSON.h"
#include "crc16.h"
#include "protocol.h"

#define LOG_INFO(fmt, ...)  printf("[INFO]:"fmt"\r\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  printf("[DEBUG]:"fmt"\r\n", ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) printf("[ERROR]:"fmt"\r\n", ##__VA_ARGS__)


typedef enum
{
    ERROR_OK = 0,
    ERROR_PARAM_ERROR = -1,
    ERROR_SOURCE_FILE_NOT_EXIT = -2,
    ERROR_SOURCE_JSON_FORMAT_ERROR = -3,
    ERROR_SOURCE_FILE_READ_ERROR = -4,
    ERROR_SOURCE_FILE_WRITE_ERROR = -5,
    ERROR_SYS_MEMORY = -6,
    ERROR_FENCE_FORMAT_ERROR = -7,
    ERROR_OUTPUT_FILE_WRITE_ERROR = -8,
}ENUM_ERROR;

static char* getErrDisc(ENUM_ERROR err)
{
    #define DESC_DEF(x) case x:\
                            return #x
    switch(err)
    {

        DESC_DEF(ERROR_OK);
        DESC_DEF(ERROR_PARAM_ERROR );
        DESC_DEF(ERROR_SOURCE_FILE_NOT_EXIT );
        DESC_DEF(ERROR_SOURCE_JSON_FORMAT_ERROR );
        DESC_DEF(ERROR_SOURCE_FILE_READ_ERROR );
        DESC_DEF(ERROR_SOURCE_FILE_WRITE_ERROR );
        DESC_DEF(ERROR_SYS_MEMORY );
        DESC_DEF(ERROR_FENCE_FORMAT_ERROR );
        DESC_DEF(ERROR_OUTPUT_FILE_WRITE_ERROR );

        default:
        {
            static char state_name[10] = {0};
            snprintf(state_name, 10, "%d", err);
            return state_name;
        }
    }
}

ENUM_ENDIAN getEndian(void)
{
    union w
    {
        int a;
        char b;
    }c;
    c.a = 1;
    return (c.b == 1 ? ENDIAN_SMALL:ENDIAN_BIG);
}

int getFileSize(char *fileName)
{
    unsigned long filesize = 0;
    FILE *fp = NULL;
    fp = fopen(fileName, "rb+");
    if(NULL == fp)
    {
        LOG_DEBUG("open %s failed:%s", fileName, strerror(errno));
        return filesize;
    }
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);
    return filesize;
}

int readFileHead(char *fileName, char *buf, int length)
{
    FILE *fp = NULL;
    int ret;

    if(!buf)
    {
        return ERROR_PARAM_ERROR;
    }

    fp = fopen(fileName, "r+");
    if(!fp)
    {
        LOG_DEBUG("open %s file error:%s", fileName, strerror(errno));
        return ERROR_SOURCE_FILE_NOT_EXIT;
    }

    ret = ERROR_OK;
    do
    {
        ret = fread(buf, sizeof(char), length, fp);
        if((ret < 0)||(ret > length))
        {
            LOG_DEBUG("read %s file error:%s", fileName, strerror(errno));
            ret =  ERROR_SOURCE_FILE_READ_ERROR;
            break;
        }

    }
    while (0);

    fclose(fp);

    return ret;
}

int writeFileCover(char *fileName, char *buf, int length)
{
    int ret;
    FILE *fp = NULL;

    fp = fopen(fileName, "wb+");
    if(!fp)
    {
        LOG_DEBUG("write and open file error:%s", strerror(errno));
        return ERROR_SOURCE_FILE_WRITE_ERROR;
    }

    ret = ERROR_OK;
    do
    {
        ret = fwrite((const void*)buf, sizeof(char), length, fp);
        if(ret < 0)
        {
            LOG_DEBUG("write setting.conf success");
            ret = ERROR_OUTPUT_FILE_WRITE_ERROR;
            break;
        }

    }while(0);

    fclose(fp);
    return ret;
}

float ed_tlr_float(float x, ENUM_ENDIAN e)
{
    union W
    {
        float a;
        int b;

    }c;

    if(ENDIAN_SMALL == e) return x;

    c.a = x;
    c.b = nettohl(c.b);
    return c.a;
}

int convert(char *outputBuf, char *inputBuf, int *inputLen, int *outputLen, ENUM_ENDIAN endian)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int outputLenTemp = sizeof(FENCE);
    cJSON *json_root = NULL;
    cJSON *json_version = NULL;
    cJSON *json_fence = NULL;

    FENCE *fenceInst = (FENCE *)outputBuf;
    FENCE_POINT *pPoints = fenceInst->points;
    //deep 0
    json_root = cJSON_Parse(inputBuf);
    if (!json_root)
    {
        LOG_DEBUG("parse json format error");
        cJSON_Delete(json_root);
        return ERROR_SOURCE_JSON_FORMAT_ERROR;
    }

    ret = ERROR_OK;
    do//depp 1
    {
        int fenceNum = 0;
        json_version = cJSON_GetObjectItem(json_root, "version");
        if(!json_version)
        {
            LOG_DEBUG("json format error, no version");
            ret = ERROR_SOURCE_JSON_FORMAT_ERROR;
            break;
        }

        json_fence = cJSON_GetObjectItem(json_root, "fence");
        if(!json_fence)
        {
            LOG_DEBUG("json format error, no fence");
            ret = ERROR_SOURCE_JSON_FORMAT_ERROR;
            break;
        }

        do//deep2
        {
            memcpy(fenceInst->signature, "XiaoanTech", 10);
            fenceInst->version = ed_tlr_32( json_version->valueint, endian);

            fenceNum = cJSON_GetArraySize(json_fence);
            if(fenceNum < 1)
            {
                LOG_DEBUG("fence format error, less len 1 fence");
                ret = ERROR_FENCE_FORMAT_ERROR;
                break;
            }

            fenceInst->fenceNum = fenceNum;

            for(i = 0; i < fenceNum; i++)
            {
                cJSON *json_fenceTemp = NULL;
                json_fenceTemp = cJSON_GetArrayItem(json_fence, i);
                if(!json_fenceTemp)
                {
                    LOG_DEBUG("get fence%d, error", i);
                    ret =  ERROR_SOURCE_JSON_FORMAT_ERROR;
                    break;
                }

                do//deep 3
                {
                    int nodeNum = 0;
                    cJSON *json_mode = NULL;
                    cJSON *json_nodeNum = NULL;
                    cJSON *json_nodes = NULL;

                    json_mode = cJSON_GetObjectItem(json_fenceTemp, "mode");
                    if(!json_mode)
                    {
                        LOG_DEBUG("get fence%d, mode error", i);
                        ret =  ERROR_SOURCE_JSON_FORMAT_ERROR;
                        break;
                    }

                    json_nodeNum = cJSON_GetObjectItem(json_fenceTemp, "nodeNum");
                    if(!json_nodeNum)
                    {
                        LOG_DEBUG("get fence%d, nodeNum error", i);
                        ret =  ERROR_SOURCE_JSON_FORMAT_ERROR;
                        break;
                    }

                    json_nodes = cJSON_GetObjectItem(json_fenceTemp, "nodes");
                    if(!json_nodes)
                    {
                        LOG_DEBUG("get fence%d, nodeNum error", i);
                        ret =  ERROR_SOURCE_JSON_FORMAT_ERROR;
                        break;
                    }

                    nodeNum = cJSON_GetArraySize(json_nodes);
                    if(nodeNum != json_nodeNum->valueint)
                    {
                        LOG_DEBUG("get fence%d, nodeNul != num", i);
                        ret = ERROR_FENCE_FORMAT_ERROR;
                        break;
                    }

                    if(nodeNum < 3)
                    {
                        LOG_DEBUG("fence%d format error, less len 3 points", i);
                        ret = ERROR_FENCE_FORMAT_ERROR;
                        break;
                    }

                    fenceInst->fencePoly[i].mode = json_mode->valueint;
                    fenceInst->fencePoly[i].nodeNum = ed_tlr_16(nodeNum, endian);

                    for(j = 0; j < nodeNum; j++)
                    {
                        cJSON *json_nodeTemp = NULL;
                        json_nodeTemp = cJSON_GetArrayItem(json_nodes, j);
                        if(!json_nodeTemp)
                        {
                            LOG_DEBUG("get fence%d, node%d error", i, j);
                            ret = ERROR_SOURCE_JSON_FORMAT_ERROR;
                            break;
                        }
                        do//deep 4
                        {
                            cJSON *json_lat = NULL;
                            cJSON *json_lon = NULL;

                            json_lat = cJSON_GetObjectItem(json_nodeTemp, "lat");
                            if(!json_lat)
                            {
                                LOG_DEBUG("get fence%d, node%d lat error", i, j);
                                ret = ERROR_SOURCE_JSON_FORMAT_ERROR;
                                break;
                            }

                            json_lon = cJSON_GetObjectItem(json_nodeTemp, "lon");
                            if(!json_lon)
                            {
                                LOG_DEBUG("get fence%d, node%d lon error", i, j);
                                ret = ERROR_SOURCE_JSON_FORMAT_ERROR;
                                break;
                            }
                            pPoints->latitude = ed_tlr_float(((float)json_lat->valuedouble), endian);
                            pPoints->longitude = ed_tlr_float(((float)json_lon->valuedouble), endian);
                            pPoints++;
                            outputLenTemp += sizeof(FENCE_POINT);
                        }while (0);//deep 4

                        if(ret < 0) break;
                    }

                }while (0);//deep 3

                if(ret < 0) break;
            }

        }while (0);//deep2

    }while (0);//depp 1

    cJSON_Delete(json_root);

    if(ERROR_OK == ret)
    {
        *outputLen = outputLenTemp;
    }

    return ret;
}

int exit_here(int err)
{
    if(err < 0) LOG_ERROR("%s happened,exit...", getErrDisc(err));
    else LOG_INFO("convert success, CRC:0x%x %u", err, err);
    exit(err);
}

//#define FILE_NAME_SOURCE "D:\\xiaoan\\proj\\fence_convert_tool\\fence_convert\\bin\\Debug\\input.js"
//#define FILE_NAME_OUTPUT "D:\\xiaoan\\proj\\fence_convert_tool\\fence_convert\\bin\\Debug\\output.conf"
static char outputBuf[1024*128] = {0};
int main(int argc, char *argv[])
{
    int ret = 0;
    int inputFileSize = 0;
    char *inputBuf = NULL;
    char FILE_NAME_SOURCE[256] = {0};
    char FILE_NAME_OUTPUT[256] = {0};

    ENUM_ENDIAN endian;

    if(argc == 3)
    {
        memcpy(FILE_NAME_SOURCE, argv[1], strlen(argv[1]));
        memcpy(FILE_NAME_OUTPUT, argv[2], strlen(argv[2]));
    }
    else
    {
        printf("example:fence_convert ./input.json ./output.conf\r\n");
        exit_here(ERROR_PARAM_ERROR);
    }

    endian = getEndian();
    LOG_DEBUG("sys endian is:%s", endian == ENDIAN_SMALL ? "SMALL":"BIG");

    inputFileSize = getFileSize(FILE_NAME_SOURCE);
    if(inputFileSize <= 0)
    {
        LOG_DEBUG("get source file size error");
        exit_here(ERROR_SOURCE_FILE_NOT_EXIT);
    }

    inputBuf = (char *)malloc(sizeof(char) * inputFileSize);
    if(!inputBuf)
    {
        LOG_DEBUG("input buffer malloc error");
        exit_here(ERROR_SYS_MEMORY);
    }

    ret = 0;
    do
    {
        int outputLen = 0;
        ret = readFileHead(FILE_NAME_SOURCE, inputBuf, inputFileSize);
        if(ret < 0) break;

        ret = convert(outputBuf, inputBuf, &inputFileSize, &outputLen, endian);
        if(ret < 0) break;
        else LOG_DEBUG("%s, fileSize:%d", FILE_NAME_OUTPUT, outputLen);

        ret = writeFileCover(FILE_NAME_OUTPUT, outputBuf, outputLen);
        if(ret < 0) break;

        ret = crc16_ccitt(outputBuf, outputLen);
    }
    while (0);

    free(inputBuf);
    exit_here(ret);

    return 0;
}
