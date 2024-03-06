/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_json_struct_common.h
 * Description:
 *
 */

#ifndef _CVI_JSON_STRUCT_COMM_H
#define _CVI_JSON_STRUCT_COMM_H

#include <stdio.h>
#include <float.h>
#include <linux/cvi_common.h>

#include "cvi_json.h"
#include "cvi_bin.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

// json utils
typedef struct cvi_json_object JSON;
extern char param_point[150];
#define CVI_U8_MAX 0xff
#define CVI_S8_MIN -128
#define CVI_S8_MAX 127
#define CVI_U16_MAX 0xffff
#define CVI_S16_MIN -32768
#define CVI_S16_MAX 0x7fff
#define CVI_U32_MAX 0xffffffff
#define CVI_S32_MIN -2147483648
#define CVI_S32_MAX 0x7fffffff
#define CVI_U64_MAX 0xffffffffffffffff
#define CVI_FLOAT_MIN -FLT_MAX
#define CVI_FLOAT_MAX FLT_MAX
#define R_FLAG 1
#define W_FLAG 0

cvi_json_bool cvi_json_object_object_get_ex2(struct cvi_json_object *obj, const char *key,
						struct cvi_json_object **value);
#define CVI_TRACE_JSON(level, ...)                                                                                     \
	do {                                                                                                           \
		printf(__VA_ARGS__);                                                                                   \
		printf("\n");                                                                                          \
	} while (0)
#define JSON_(r_w_flag, base, type, key, value) type##_JSON(r_w_flag, base, key, (value))
#define JSON(r_w_flag, type, value) JSON_(r_w_flag, obj, type, #value, &data->value)
#define JSON_A_(r_w_flag, base, type, key, value, length)                                   \
	do {                                                                                    \
		JSON *array;                                                                        \
		if (r_w_flag == R_FLAG) {                                                           \
			if (cvi_json_object_object_get_ex(base, key, &array)) {                         \
				cvi_json_type cvi_type = cvi_json_object_get_type(array);                   \
				if (cvi_type != cvi_json_type_array) {                                      \
					JSON_PRINT_ERR_DATA_TYPE(key);                                          \
					break;                                                                  \
				}                                                                           \
				int size = cvi_json_object_array_length(array);                             \
				size = size > length ? length : size;                                       \
				for (int i = 0; i < size; i++) {                                            \
					char buffer_temp[50] = { 0 };                                           \
					snprintf(buffer_temp, sizeof(buffer_temp) - 1, "%s[%d]", key, i);       \
					type##_JSON(r_w_flag, array, buffer_temp, (type *)(value) + i);         \
				}                                                                           \
			} else {                                                                        \
				JSON_PRINT_ERR_NOT_EXIST(key);                                              \
			}                                                                               \
		} else {                                                                            \
			array = cvi_json_object_new_array();                                            \
			for (int i = 0; i < length; i++) {                                              \
				type##_JSON(r_w_flag, array, #value, ((type *)(value) + i));                \
			}                                                                               \
			cvi_json_object_object_add(base, key, array);                                   \
		}                                                                                   \
	} while (0)
#define JSON_A(r_w_flag, type, value, length) JSON_A_(r_w_flag, obj, type, #value, data->value, (length))
#define JSON_SB(r_w_flag, type, value)                                                                                 \
	{                                                                                                              \
		JSON *obj_temp = 0;                                                                                    \
		if (r_w_flag == R_FLAG) {                                                                              \
			if (cvi_json_object_object_get_ex2(obj, #value, &obj_temp)) {                                  \
				if (strcmp(#type, "CVI_U64") == 0) {                                                   \
					unsigned long long temp;                                                       \
					temp = cvi_json_object_get_uint64(obj_temp);                                   \
					data->value = temp;                                                            \
				}                                                                                      \
			} else {                                                                                       \
				JSON_PRINT_ERR_NOT_EXIST(#value);                                                      \
			}                                                                                              \
		} else {                                                                                               \
			if (strcmp(#type, "CVI_U64") == 0) {                                                           \
				obj_temp = cvi_json_object_new_uint64(data->value);                                    \
			}                                                                                              \
			if (cvi_json_object_is_type(obj, cvi_json_type_array)) {                                       \
				cvi_json_object_array_add(obj, obj_temp);                                              \
			} else {                                                                                       \
				cvi_json_object_object_add(obj, #value, obj_temp);                                     \
			}                                                                                              \
		}                                                                                                      \
	}
#define JSON_START(r_w_flag)                                                                                           \
	JSON *obj = NULL;                                                                                              \
	{                                                                                                              \
		if (r_w_flag == R_FLAG) {                                                                              \
			if (cvi_json_object_object_get_ex2(j, key, &obj) == 0) {                                       \
				goto NotExit;                                                                          \
			}                                                                                              \
			if (strlen(param_point) + strlen(key) + 1 <= sizeof(param_point) - 1) {                        \
				strcat(param_point, key);                                                              \
				strcat(param_point, ".");                                                              \
			} else {                                                                                       \
				printf("param_point overflow key =%s \n",key);                                                       \
			}                                                                                              \
		} else {                                                                                               \
			obj = cvi_json_object_new_object();                                                            \
		}                                                                                                      \
	}
#define JSON_END(r_w_flag)                                                                                             \
	{                                                                                                              \
		if (r_w_flag == R_FLAG) {                                                                              \
			param_point[strlen(param_point) - 1] = 0;                                                      \
			char *str = strrchr(param_point, '.');                                                         \
			if (str == NULL) {                                                                             \
				str = param_point;                                                                     \
			} else {                                                                                       \
				str++;                                                                                 \
			}                                                                                              \
			if (strstr(key, "ISP_MESH_SHADING_GAIN_LUT_ATTR_S") || strstr(key, "ISP_CLUT_ATTR_S")) {\
				memset(param_point, 0, sizeof(param_point));                        \
			} else {                        \
				memset(str, 0, sizeof(param_point) - (str - param_point)); \
			}                                                   \
			return;                                                                                        \
NotExit:                                                                                               \
			JSON_PRINT_ERR_NOT_EXIST(key);                                                             \
		} else {                                                                                               \
			if (cvi_json_object_is_type(j, cvi_json_type_array)) {                                         \
				cvi_json_object_array_add(j, obj);                                                     \
			} else {                                                                                       \
				cvi_json_object_object_add(j, key, obj);                                               \
			}                                                                                              \
		}                                                                                                      \
	}

#define JSON_START_ENTRANCE(r_w_flag)                 \
	{                                                                 \
		if (r_w_flag == R_FLAG) {                                    \
			if (cvi_json_object_object_get_ex2(j, key, &obj) == 0) {      \
				idx = 0; \
				JSON_PRINT_ERR_NOT_EXIST(key);                                   \
			}                                                                          \
		} else {                                                                \
			obj = cvi_json_object_new_object();                             \
		}                                                                 \
	}
#define JSON_END_ENTRANCE(r_w_flag)                     \
	{                                                      \
		if (r_w_flag != R_FLAG) {                                                                     \
			if (cvi_json_object_is_type(j, cvi_json_type_array)) {                                         \
				cvi_json_object_array_add(j, obj);                                                     \
			} else {                                                                                       \
				cvi_json_object_object_add(j, key, obj);                                               \
			}                                                                                              \
		}                                                                                                      \
	}

#define GET_ARRAY_STRING_SIZE(key, size, max_byte)   (strlen(key) + size*(max_byte + 2) + 10)
#define GET_OBJECT_STRING_SIZE(key, max_byte)       (strlen(key) + max_byte + 10)

#define JSON_EX_START(str)        strcat(str, "{ ")
#define JSON_EX_END(str)        strcat(str, " }")

#define PARSE_INT_ARRAY_TO_JSON_STR(key, data, size, str)   {       \
	char tmpstr[64] = {0};                                                      \
	snprintf(tmpstr, sizeof(tmpstr),"\"%s\": [ %d", key, data[0]);              \
	strcat(str, tmpstr);                                                        \
	int curPos = strlen(str);                                                   \
	for(int i = 1; i < size; i++){                                              \
		snprintf(tmpstr, sizeof(tmpstr), ", %d", data[i]);                      \
		strcat(str + curPos, tmpstr);                                           \
		curPos += strlen(tmpstr);                                               \
	}                                                                           \
	strcat(str, " ]");                                                          \
}

#define PARSE_JSON_STR_TO_INT_VALUE(key, json_str, data) do{                    \
	unsigned int tmpVal;                                                        \
	char *tmp_str = strstr(json_str, #key);                                          \
	tmp_str = strstr(tmp_str, ":");                                           \
	sscanf(tmp_str + 1, "%u", &tmpVal);                                        \
	data->key = tmpVal;                                                        \
} while(0);

#define PARSE_JSON_STR_TO_INT_ARRAY(key, json_str, data, size)   do {           \
	unsigned int tmpVal;                                                        \
	char *tmp_str = json_str;													\
	char *keyPos = strstr(tmp_str, #key);                                      \
	char *endPos = strstr(keyPos, "]");                                       \
	if(keyPos && keyPos < endPos) {                                             \
		tmp_str = strstr(keyPos, "[");                                       \
		valPos = tmp_str + 1;                                                  \
		for (int i = 0; i < size; i++) {                                        \
			sscanf(valPos, "%u", &tmpVal);                                      \
			data->key[i] = tmpVal;                                              \
			if(strstr(valPos, ",") && strstr(valPos, ",") < endPos) {           \
				valPos = strstr(valPos, ",") + 1;                               \
			} else {                                                            \
				break;                                                          \
			}                                                                   \
		}                                                                       \
	} else {                                                                    \
		printf("not found keyword %s \n", #key);                             \
	}                                                                           \
} while(0);


void JSON_CHECK_RANGE(char *name, int *value, int min, int max);
void JSON_CHECK_RANGE_OF_32(char *name, long long *value, long long min, long long max);
void JSON_CHECK_RANGE_OF_U64(char *name, unsigned long long *value, unsigned long long min, unsigned long long max);
void JSON_CHECK_RANGE_OF_DOUBLE(char *name, double *value, double min, double max);
void JSON_PRINT_ERR_NOT_EXIST(char *name);
void JSON_PRINT_ERR_DATA_TYPE(char *name);
void CVI_BOOL_JSON(int r_w_flag, JSON *j, char *key, CVI_BOOL *value);
void CVI_U8_JSON(int r_w_flag, JSON *j, char *key, CVI_U8 *value);
void CVI_S8_JSON(int r_w_flag, JSON *j, char *key, CVI_S8 *value);
void CVI_U16_JSON(int r_w_flag, JSON *j, char *key, CVI_U16 *value);
void CVI_S16_JSON(int r_w_flag, JSON *j, char *key, CVI_S16 *value);
void CVI_U32_JSON(int r_w_flag, JSON *j, char *key, CVI_U32 *value);
void CVI_S32_JSON(int r_w_flag, JSON *j, char *key, CVI_S32 *value);
void CVI_U64_JSON(int r_w_flag, JSON *j, char *key, CVI_U64 *value);
void CVI_FLOAT_JSON(int r_w_flag, JSON *j, char *key, CVI_FLOAT *value);

JSON *JSON_GetNewObject(void);
JSON *JSON_TokenerParse(const char *buffer);
CVI_S32 JSON_GetJsonStrLen(JSON *json_object);
CVI_S32 JSON_ObjectPut(JSON *json_object);
const char *JSON_GetJsonStrContent(JSON *json_object);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _CVI_JSON_STRUCT_COMM_H
