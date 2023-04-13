#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

typedef struct
{
    DWORD serial, answerCount, rightAnswer;
    wchar_t *title, **answer;
} Q;

DWORD qGlobalCount = 0;
Q *globalQ;

void ReadQsFromFile(char *path, Q *q)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        printf("Error: File not found!");
        exit(1);
    }
    DWORD i = 0;
    while (!feof(file))
    {
        i++;
        q = (Q *)realloc(q, sizeof(Q) * i);

        i--;
        // init title
        q[i].title = (wchar_t *)malloc(sizeof(wchar_t) * 100);
        fwscanf(file, L"%d %ls", &q[i].serial, q[i].title);
        fwscanf(file, L"%d %d", &q[i].answerCount, &q[i].rightAnswer);
        q[i].answer = (wchar_t **)malloc(sizeof(wchar_t *) * q[i].answerCount);
        for (DWORD j = 0; j < q[i].answerCount; j++)
        {
            // init answer
            q[i].answer[j] = (wchar_t *)malloc(sizeof(wchar_t) * 100);
            fwscanf(file, L"%ls", q[i].answer[j]);
        }
        i++;
    }
    qGlobalCount = i;
    fclose(file);
}

COORD bufferSize;

void InitializeGUI()
{
    /*
    TODO:
    get screen buffer size
    disable quick edit mode
    enable mouse input
    change colortable[0] to dark magenta
    full screen
    */

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    bufferSize = csbi.dwSize;

    DWORD mode;

    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdIn, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_INSERT_MODE;
    SetConsoleMode(hStdIn, mode);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFOEX csbiex;
    csbiex.cbSize = sizeof(csbiex);
    GetConsoleScreenBufferInfoEx(hConsole, &csbiex);
    csbiex.ColorTable[0] = RGB(139, 0, 139);
    SetConsoleScreenBufferInfoEx(hConsole, &csbiex);

    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdOut, &cursorInfo);

    SetConsoleTitleW(L"Quiz");

    SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(fontInfo);
    GetCurrentConsoleFontEx(hStdOut, FALSE, &fontInfo);
    fontInfo.dwFontSize.X = 8;
    fontInfo.dwFontSize.Y = 16;
    SetCurrentConsoleFontEx(hStdOut, FALSE, &fontInfo);

    // full screen without border
    HWND hwnd = GetConsoleWindow();
    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);
    LONG l_WinStyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, (l_WinStyle | WS_POPUP | WS_MAXIMIZE) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_BORDER); // 设置窗口样式
    SetWindowPos(hwnd, HWND_TOP, 0, 0, cx, cy, 0);                                                                     // 设置窗口位置和大小
    HANDLE Hand;
    CONSOLE_SCREEN_BUFFER_INFO Info;
    Hand = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(Hand, &Info);
    SMALL_RECT rect = Info.srWindow;
    COORD size = {rect.Right + 1, rect.Bottom + 1}; // 定义缓冲区大小，保持缓冲区大小和屏幕大小一致即可取消边框
    SetConsoleScreenBufferSize(Hand, size);
    bufferSize = size;
    //system("cls");
}

void DrawStartPage()
{
    /*
    TODO:
    draw welcome text,start button and exit button at center. text is 'welcome to quiz system'
    alignment should be horizontal center
    */

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    COORD pos = {bufferSize.X / 2 - 10, bufferSize.Y / 2 - 2};
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Welcome to quiz system");
    pos.Y += 2;
    pos.X += 4;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Start");
    pos.X += 9;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Exit");
}

void DrawQuestion(Q *q, DWORD index, DWORD qCount)
{
    /*
    TODO:
    draw question title and answer at topleft corner
    alignment should be horizontal left
    draw question serial number at topright corner
    alignment should be horizontal right
    draw serial select buttons at centerright
    serial select buttons should as much as Q count
    */

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"%ls", q[index].title);
    pos.Y += 2;
    SetConsoleCursorPosition(hStdOut, pos);
    for (DWORD i = 0; i < q[index].answerCount; i++)
    {
        //print abcd
        wprintf(L"%c.", 'A' + i);
        wprintf(L"%ls", q[index].answer[i]);
        pos.Y++;
        SetConsoleCursorPosition(hStdOut, pos);
    }
    pos.X = bufferSize.X - 2;
    pos.Y = 0;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"%d/%d", index + 1, qCount);
    pos.X = bufferSize.X - 2;
    pos.Y = 2;
    SetConsoleCursorPosition(hStdOut, pos);
    /*for (DWORD i = 0; i < qCount; i++)
    {
        printf("%d", i + 1);
        pos.Y++;
        SetConsoleCursorPosition(hStdOut, pos);
    }*/
}

void ReadMouseInput()
{
    /*
    TODO:
    handle mouse input at start page
    */
    INPUT_RECORD inRec;
    DWORD res;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE); /* 获取标准输入设备句柄*/
    do
    {
        ReadConsoleInputW(hInput, &inRec, 1, &res);
    } while (inRec.Event.MouseEvent.dwButtonState != FROM_LEFT_1ST_BUTTON_PRESSED);
    COORD pos = {bufferSize.X / 2 - 10, bufferSize.Y / 2 - 2};
    if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X+4 && inRec.Event.MouseEvent.dwMousePosition.X <= pos.X + 9 && inRec.Event.MouseEvent.dwMousePosition.Y == pos.Y+2)
    {
        DrawQuestion(globalQ, 0, qGlobalCount);
    }
    else if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X + 14 && inRec.Event.MouseEvent.dwMousePosition.X <= pos.X + 18 && inRec.Event.MouseEvent.dwMousePosition.Y == pos.Y+2)
    {
        exit(0);
    }
}

int main()
{
    ReadQsFromFile("question.txt", globalQ);
    InitializeGUI();
    DrawStartPage();
    ReadMouseInput();
    _getwch();
}