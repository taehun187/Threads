#include <windows.h>
#include <cstdio>  // C 함수들을 위한 헤더

#define NUM_THREADS 10
#define SEMAPHORE_MAX_COUNT 3
#define BUFFER_SIZE 256

// 전역 세마포어 핸들
HANDLE hSemaphore;
const char* sharedFileName = "shared_resource.txt";  // 공유 파일 이름

// 파일에 쓰기
bool WriteToFile(const char* fileName, const char* data) {
    FILE* file;
    errno_t erro = fopen_s(&file, fileName, "a");  // append 모드로 파일 열기
    bool writeSuccess = false;  // 로컬 변수로 상태 관리

    if (file != NULL) {
        fprintf(file, "%s\n", data);
        writeSuccess = true;  // 기록 성공
        fclose(file);
    }
    else {
        printf("Unable to open file for writing: %s\n", fileName);
    }

    return writeSuccess;  // 기록 성공 여부 반환
}

// 스레드가 실행할 함수
DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    int threadId = *(int*)lpParam;
    DWORD waitResult;
    char buffer[BUFFER_SIZE];  // 문자열 저장을 위한 버퍼

    printf("Thread %d Launch!!!.\n", threadId);

    // 세마포어 획득 시도
    waitResult = WaitForSingleObject(hSemaphore, INFINITE);

    if (waitResult == WAIT_OBJECT_0) {
        printf("Thread %d has entered the critical section.\n", threadId);

        // 공유 파일에 쓰기
        for (int i = 0; i < 10; ++i) {
            // 데이터 문자열 생성
            snprintf(buffer, sizeof(buffer), "Thread %d writes %d", threadId, i);

            bool writeSuccess = WriteToFile(sharedFileName, buffer);

            if (writeSuccess) {
                printf("Thread %d successfully wrote to file.\n", threadId);
            }
            else {

                DWORD err = GetLastError();
                printf("Thread %d failed to write to file. err=%d.\n", threadId, err);
            }

            Sleep(1000); // 1초 대기
        }

        printf("Thread %d is leaving the critical section.\n", threadId);

        // 세마포어 해제
        if (!ReleaseSemaphore(hSemaphore, 1, NULL)) {
            printf("ReleaseSemaphore error: %d\n", GetLastError());
        }
    }
    else {
        printf("WaitForSingleObject error: %d\n", GetLastError());
    }

    return 0;
}

int main() {
    // 세마포어 초기화 (최대 SEMAPHORE_MAX_COUNT개의 스레드가 동시에 접근 가능)
    hSemaphore = CreateSemaphore(NULL, SEMAPHORE_MAX_COUNT, SEMAPHORE_MAX_COUNT, NULL);
    if (hSemaphore == NULL) {
        printf("CreateSemaphore error: %d\n", GetLastError());
        return 1;
    }

    // 스레드 ID를 저장할 배열
    int threadIds[NUM_THREADS];
    HANDLE hThreads[NUM_THREADS];

    // 스레드 생성
    for (int i = 0; i < NUM_THREADS; ++i) {
        threadIds[i] = i + 1;
        hThreads[i] = CreateThread(NULL, 0, ThreadFunction, &threadIds[i], 0, NULL);
        if (hThreads[i] == NULL) {
            printf("CreateThread error: %d\n", GetLastError());
            return 1;
        }
    }

    // 모든 스레드가 종료될 때까지 대기
    WaitForMultipleObjects(NUM_THREADS, hThreads, TRUE, INFINITE);

    // 핸들 정리
    for (int i = 0; i < NUM_THREADS; ++i) {
        CloseHandle(hThreads[i]);
    }
    CloseHandle(hSemaphore);

    return 0;
}