echo Matando processos...

taskkill /f /im trose.exe
taskkill /f /im sho_gs.exe
taskkill /f /im sho_ws.exe
taskkill /f /im sho_ls.exe

echo Fazendo deploy dos arquivos do server...
xcopy C:\Rose\irose\bin\release\SHO_LS.dll C:\Rose\irose\server\LoginServer\ /y
xcopy C:\Rose\irose\bin\release\SHO_WS.dll C:\Rose\irose\server\WorldServer\ /y
xcopy C:\Rose\irose\bin\release\SHO_GS.dll C:\Rose\irose\server\GameServer\ /y

echo Fazendo deploy dos arquivos do client...
xcopy C:\Rose\irose\bin\release\trose.exe C:\Rose\irose\game\ /y
xcopy C:\Rose\irose\bin\release\triggervfs.dll C:\Rose\irose\game\ /y
xcopy C:\Rose\irose\bin\release\znzin.dll C:\Rose\irose\game\ /y

echo Iniciando Login server
start C:\Rose\irose\server\LoginServer\sho_ls.exe
timeout /t 10

echo Iniciando World server
start C:\Rose\irose\server\WorldServer\sho_ws.exe
timeout /t 15

echo Iniciando Game server
start C:\Rose\irose\server\GameServer\sho_gs.exe
echo Iniciando o Client do Jogo
cd C:\Rose\irose\game\
TRose.exe @TRIGGER_SOFT@ _server 127.0.0.1