#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <locale.h>
#include <fcntl.h>

typedef struct
{
    DWORD serial, answerCount, rightAnswer;
    wchar_t *title, **answer;
} Q;

DWORD *clientAnswer;
COORD bufferSize;
DWORD qGlobalCount = 0;
DWORD nowPage = 0;
Q *globalQ;

void ReadQuizInput();

void ReadQsFromFile(wchar_t *path, Q *q)
{
    FILE *file = _wfopen(path, L"r");
    if (file == NULL)
    {
        printf("Error: File not found!");
        exit(1);
    }
    DWORD i = 0;
    fwscanf(file, L"%d", &qGlobalCount);
    q = (Q *)realloc(q, sizeof(Q) * qGlobalCount);
    clientAnswer = (DWORD *)calloc(qGlobalCount, sizeof(DWORD));
    for (int i = 0; i < qGlobalCount; i++)
    {
        // init title
        q[i].title = (wchar_t *)malloc(sizeof(wchar_t) * 1000);
        fwscanf(file, L"%d", &q[i].serial);
        // use fgetws to get wchar_t* values
        fgetws(q[i].title, 1000, file);
        // init serial
        fwscanf(file, L"%d %d\n", &q[i].answerCount, &q[i].rightAnswer);
        q[i].answer = (wchar_t **)malloc(sizeof(wchar_t *) * q[i].answerCount);
        for (DWORD j = 0; j < q[i].answerCount; j++)
        {
            // init answer
            q[i].answer[j] = (wchar_t *)malloc(sizeof(wchar_t) * 1000);
            fgetws(q[i].answer[j], 1000, file);
        }
    }
    globalQ = q;
    fclose(file);
}

void ClearScreen()
{
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hStdOut, pos);
    // generate space line,then print
    wchar_t *spaceLine = (wchar_t *)malloc(sizeof(wchar_t) * bufferSize.X);
    for (DWORD i = 0; i < bufferSize.X; i++)
    {
        spaceLine[i] = L' ';
    }
    for (DWORD i = 0; i < bufferSize.Y; i++)
    {
        SetConsoleCursorPosition(hStdOut, pos);
        wprintf(L"%ls", spaceLine);
        pos.Y++;
    }
    SetConsoleCursorPosition(hStdOut, (COORD){0, 0});
}

void InitializeGUI()
{
    /*
    TODO:
    get screen buffer size
    disable quick edit mode
    enable mouse input
    change colortable[0] to dark green
    full screen
    */
    _wsetlocale(LC_ALL, L"");
    _setmode(_fileno(stdout), _O_U16TEXT);
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
    csbiex.ColorTable[0] = 0x00003300;
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
    COORD size = {rect.Right + 1, rect.Bottom + 1};
    SetConsoleScreenBufferSize(Hand, size);
    bufferSize = size;
    // system("cls");
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

    answer should add ABCD prefix

    if any line is too long,it should be cut and continue at next line (line length is 3/4 of screen width,use bufferSize to calculate it)

    serial select buttons should be put 3 in one row
    */

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    COORD pos = {0, 0};
    SetConsoleCursorPosition(hStdOut, pos);
    wchar_t *spaceLine = (wchar_t *)malloc(sizeof(wchar_t) * bufferSize.X);
    for (DWORD i = 0; i < bufferSize.X; i++)
    {
        spaceLine[i] = L' ';
    }
    for (DWORD i = 0; i < bufferSize.Y; i++)
    {
        SetConsoleCursorPosition(hStdOut, pos);
        wprintf(L"%ls", spaceLine);
        pos.Y++;
    }
    pos.Y = 0;
    SetConsoleCursorPosition(hStdOut, pos);
    
    wprintf(L"%d.%ls", q->serial,q->title);
    pos.Y += 2;
    // use answerCount
    for (DWORD i = 0; i < q->answerCount; i++)
    {
        SetConsoleCursorPosition(hStdOut, pos);
        wprintf(L"%c.%ls", i + 'A', q->answer[i]);
        pos.Y++;
    }
    pos.X = bufferSize.X - 16;
    pos.Y = 0;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Progress:%d/%d", index + 1, qCount);
    // render 'select to jmp' at Y=1
    pos.Y = 1;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Select to jmp");
    pos.X = bufferSize.X - 3;
    pos.Y = 2;
    SetConsoleCursorPosition(hStdOut, pos);
    for (DWORD i = 0; i < qCount; i++)
    {
        wprintf(L"%d", i+1);
        if (i % 3 == 2)
        {
            pos.Y++;
            pos.X = bufferSize.X - 9;
            SetConsoleCursorPosition(hStdOut, pos);
        }
        else
        {
            pos.X -= 3;
            SetConsoleCursorPosition(hStdOut, pos);
        }
    }

    free(spaceLine);
    ReadQuizInput();
}

void DrawResultPage(DWORD qCount)
{
    /*
    TODO:
    draw result text at center
    alignment should be horizontal center
    draw correct count and question count at topright corner
    alignment should be horizontal right
    calculate and draw correct rate at topleft corner
    alignment should be horizontal left
    */
    //generate correctCount by comparing globalQ.answer and clientAnswer
    DWORD correctCount = 0;
    for (DWORD i = 0; i < qCount; i++)
    {
        if (globalQ[i].rightAnswer == clientAnswer[i])
        {
            correctCount++;
        }
    }
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    COORD pos = {bufferSize.X / 2 - 10, bufferSize.Y / 2 - 2};
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Result");
    pos.Y += 2;
    pos.X += 4;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Correct:%d", correctCount);
    pos.Y++;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Total:%d", qCount);
    pos.Y++;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Rate:%.2f%%", (float)correctCount / qCount * 100);
    pos.Y += 2;
    pos.X -= 4;
    SetConsoleCursorPosition(hStdOut, pos);
    wprintf(L"Press any key to exit");
}

void ReadQuizInput()
{
    /*
    TODO:
    handle mouse input at quiz page
    write client's answer to clientAnswer[] (clientAnswer is a global variable),A=1,B=2,etc...
    then render next question page if there's any question left
    or render result page if there's no question left
    nowPage is current page index,nowPage=0 means start page,nowPage=1 means quiz page(globalQ[0]),nowPage=2 means quiz page(globalQ[1]),etc...
    */
    INPUT_RECORD inRec;
    DWORD res;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE); /* 获取标准输入设备句柄*/
    LABEL1:
    do
    {
        ReadConsoleInputW(hInput, &inRec, 1, &res);
    } while (inRec.Event.MouseEvent.dwButtonState != FROM_LEFT_1ST_BUTTON_PRESSED);
    COORD pos = {0, 2};
    if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X && inRec.Event.MouseEvent.dwMousePosition.X <= bufferSize.X -9 && inRec.Event.MouseEvent.dwMousePosition.Y >= pos.Y)
    {
        clientAnswer[nowPage - 1] = inRec.Event.MouseEvent.dwMousePosition.Y - pos.Y + 1;
        nowPage++;
        if (nowPage == qGlobalCount + 1)
        {
            ClearScreen();
            DrawResultPage(qGlobalCount);
            return;
        }
        else
        {
            ClearScreen();
            DrawQuestion(&globalQ[nowPage - 1], nowPage - 1, qGlobalCount);
            return;
        }
    }
    //implentation of 'select to jmp'
    pos.X = bufferSize.X - 9;
    pos.Y = 2;
    if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X && inRec.Event.MouseEvent.dwMousePosition.X <= pos.X + 2 && inRec.Event.MouseEvent.dwMousePosition.Y >= pos.Y && inRec.Event.MouseEvent.dwMousePosition.Y <= pos.Y + 2)
    {
        clientAnswer[nowPage - 1] = inRec.Event.MouseEvent.dwMousePosition.Y - pos.Y + 1;
        nowPage = inRec.Event.MouseEvent.dwMousePosition.Y - pos.Y + 1;
        ClearScreen();
        DrawQuestion(&globalQ[nowPage - 1], nowPage - 1, qGlobalCount);
        return;
    }
    goto LABEL1;
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
    LABEL2:
    do
    {
        ReadConsoleInputW(hInput, &inRec, 1, &res);
    } while (inRec.Event.MouseEvent.dwButtonState != FROM_LEFT_1ST_BUTTON_PRESSED);
    COORD pos = {bufferSize.X / 2 - 10, bufferSize.Y / 2 - 2};
    if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X + 4 && inRec.Event.MouseEvent.dwMousePosition.X <= pos.X + 9 && inRec.Event.MouseEvent.dwMousePosition.Y == pos.Y + 2)
    {
        ClearScreen();
        nowPage += 1;
        DrawQuestion(&globalQ[0], 0, qGlobalCount);
        return;
    }
    else if (inRec.Event.MouseEvent.dwMousePosition.X >= pos.X + 14 && inRec.Event.MouseEvent.dwMousePosition.X <= pos.X + 18 && inRec.Event.MouseEvent.dwMousePosition.Y == pos.Y + 2)
    {
        exit(0);
    }
    goto LABEL2;
}

int main()
{
    ReadQsFromFile(L"question.txt", globalQ);
    InitializeGUI();
    DrawStartPage();
    ReadMouseInput();
    _getwch();
}