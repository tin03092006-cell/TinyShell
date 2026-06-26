@echo off
chcp 65001 >nul
echo =======================================================
echo [1/2] KIEM TRA TU DONG CAC CHUC NANG CO BAN
echo =======================================================
echo Dang doc lenh tu file test_auto.txt vao myshell.exe...
echo.
myshell.exe < test_auto.txt
echo.
echo =======================================================
echo [2/2] KIEM TRA TUONG TAC CHUC NANG QUAN LY TIEN TRINH
echo =======================================================
echo De kiem tra cac lenh stop, resume, kill, ban can lam thu cong vi PID thay doi.
echo.
echo Ban hay go cac lenh sau vao shell sap hien ra:
echo 1. notepad.exe ^&       (De mo mot tien trinh ngam)

echo 2. list                 (De xem PID cua notepad vua mo)
echo 3. stop ^<PID^>         (Thay ^<PID^> bang so hien thi o tren. Thu go vao notepad xem co bi dong bang khong)
echo 4. resume ^<PID^>       (Notepad se hoat dong tro lai)
echo 5. kill ^<PID^>         (Notepad se bi buoc tat)
echo 6. exit                 (Thoat shell)
echo =======================================================
pause
myshell.exe
