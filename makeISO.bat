echo off

REM STEP 1 - convert to Opera file system ISO
tools\opera\3doiso.exe -in CD -out demo.iso
echo 1 of 2: ISO file system done

REM STEP 2 - Sign it
cd tools\sign
3doEncrypt.exe genromtags ..\..\demo.iso
echo 2 of 2: signed demo.iso
echo Great Success!

pause
