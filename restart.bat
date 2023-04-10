echo Matando processos...

taskkill /f /im sho_gs.exe
taskkill /f /im sho_ws.exe
taskkill /f /im sho_ls.exe

echo Iniciando Login server
start C:\Rose\irose\server\LoginServer\sho_ls.exe
timeout /t 15

echo Iniciando World server
start C:\Rose\irose\server\WorldServer\sho_ws.exe
timeout /t 15

echo Iniciando Game server
start C:\Rose\irose\server\GameServer\sho_gs.exe