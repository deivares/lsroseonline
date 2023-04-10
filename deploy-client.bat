echo Matando processos...

taskkill /f /im trose.exe

echo Fazendo deploy dos arquivos do client...
xcopy C:\Rose\irose\bin\release\trose.exe C:\Rose\irose\game\ /y
xcopy C:\Rose\irose\bin\release\triggervfs.dll C:\Rose\irose\game\ /y
xcopy C:\Rose\irose\bin\release\znzin.dll C:\Rose\irose\game\ /y

echo Iniciando o Client do Jogo
cd C:\Rose\irose\game\
TRose.exe @TRIGGER_SOFT@ _server 127.0.0.1