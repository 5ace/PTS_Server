@echo off
REM set the correct binary exe
set BINEXE="C:\bin\md5deep-3.9.2\md5deep64.exe"
set JAVAEXE="C:\bin\cdvsHashChecker.jar"
set EXPERIMENTS="graphics\original graphics\vga graphics\vga_jpeg paintings video buildings objects"
set OUTFILE="all_md5sum.txt"
		
REM save the script dir
set SCRIPTDIR=%CD%

REM set the working home dir (drive and directory)
E:
cd "\CDVS\Dataset-28072011"
echo working directory: %CD%

REM check files
if NOT EXIST %BINEXE% goto missing1
if NOT EXIST %JAVAEXE% goto missing2
for %%x IN (graphics\original graphics\vga graphics\vga_jpeg paintings video buildings objects) do if NOT EXIST  %%x goto missing3

REM skip generation (TO BE REMOVED)
goto docheck

REM reset output file
if EXIST %OUTFILE% del %OUTFILE%
echo computing %OUTFILE%, please wait...

FOR /R graphics\original %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R graphics\vga %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R graphics\vga_jpeg %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R paintings %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R video %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R buildings %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%
FOR /R objects %%x IN (*.cdvs) DO %BINEXE% %%x >> %OUTFILE%

:docheck
java -jar %JAVAEXE% %OUTFILE%
echo done.
goto end

:missing1
echo %BINEXE% is missing;
echo please install %BINEXE% from http://md5deep.sourceforge.net/ before running this script.
goto end

:missing2
echo %JAVAEXE% is missing;
echo please install %JAVAEXE% before running this script.
goto end

:missing3
echo some experiment directory is missing: %EXPERIMENTS%
goto end

:end
pause
exit