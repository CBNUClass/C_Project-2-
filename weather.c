#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

struct string {
    char* ptr;
    size_t len;
};

typedef struct {
    char name[20];
    char nx[4];
    char ny[4];
} Location;

void init_string(struct string* s) {
    s->len = 0;
    s->ptr = malloc(1);
    if (s->ptr == NULL) {
        fprintf(stderr, "메모리 할당 실패\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void* ptr, size_t size, size_t nmemb, struct string* s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "메모리 재할당 실패\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size * nmemb;
}

int main(void) {
    CURL* curl;
    CURLcode res;
    struct string response;
    init_string(&response);

    // 좌표 미리 등록된 도시 목록
    Location locations[] = {
        {"서울", "60", "127"},
        {"부산", "98", "76"},
        {"대구", "89", "90"},
        {"인천", "55", "124"},
        {"광주", "58", "74"},
        {"대전", "67", "100"},
        {"울산", "102", "84"},
        {"세종", "66", "103"},
        {"청주", "69", "106"},
        {"제주", "52", "38"}
    };

    int locationCount = sizeof(locations) / sizeof(locations[0]);
    printf("사용할 지역을 선택하세요:\n");
    for (int i = 0; i < locationCount; ++i) {
        printf("  %2d. %s (X: %s, Y: %s)\n", i + 1, locations[i].name, locations[i].nx, locations[i].ny);
    }

    int choice;
    printf("번호 입력: ");
    scanf("%d", &choice);
    if (choice < 1 || choice > locationCount) {
        printf("잘못된 선택입니다.\n");
        return 1;
    }

    char base_date[9], base_time[5];
    printf("날짜 입력 (yyyyMMdd): ");
    scanf("%8s", base_date);
    printf("시간 입력 (HHMM): ");
    scanf("%4s", base_time);

    const char* serviceKey = "fHhnNwA7fGBGdq%2FTX99FNNLQJh6pa3CQTHUPpKpk%2FyNHVqEzIDueYm2EKXOq7%2BfjY4fS4KpjCEQBoG3oQ0tTaQ%3D%3D";
    char url[1024];
    snprintf(url, sizeof(url),
             "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/"
             "getUltraSrtNcst?serviceKey=%s&pageNo=1&numOfRows=100&dataType=JSON"
             "&base_date=%s&base_time=%s&nx=%s&ny=%s",
             serviceKey, base_date, base_time, locations[choice - 1].nx, locations[choice - 1].ny);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "요청 실패: %s\n", curl_easy_strerror(res));
        } else {
            printf("\n==== 응답 원문 ====\n%s\n", response.ptr);

            cJSON* root = cJSON_Parse(response.ptr);
            if (root) {
                cJSON* response_obj = cJSON_GetObjectItem(root, "response");
                cJSON* body = cJSON_GetObjectItem(response_obj, "body");
                cJSON* items = cJSON_GetObjectItem(body, "items");
                cJSON* itemArray = cJSON_GetObjectItem(items, "item");

                int arraySize = cJSON_GetArraySize(itemArray);
                for (int i = 0; i < arraySize; ++i) {
                    cJSON* item = cJSON_GetArrayItem(itemArray, i);
                    const char* category = cJSON_GetObjectItem(item, "category")->valuestring;
                    const char* obsrValue = cJSON_GetObjectItem(item, "obsrValue")->valuestring;

                    if (strcmp(category, "T1H") == 0) {
                        printf("현재 기온: %s°C\n", obsrValue);
                    } else if (strcmp(category, "RN1") == 0) {
                        printf("강수량: %smm\n", obsrValue);
                    } else if (strcmp(category, "WSD") == 0) {
                        printf("풍속: %sm/s\n", obsrValue);
                    }
                }
                cJSON_Delete(root);
            } else {
                fprintf(stderr, "JSON 파싱 실패\n");
            }
        }
        curl_easy_cleanup(curl);
    }

    free(response.ptr);
    curl_global_cleanup();

    return 0;
}
