#!/usr/bin/env

#rm -rf ~/.wine

wineboot

export LC_ALL=en_US.utf8
export LANG=en_US.utf8
export LANGUAGE=en_US
export STAGING_WRITECOPY=1

mkdir ~/.wine/drive_c/windows/system32/drivers/etc
cp /etc/hosts ~/.wine/drive_c/windows/system32/drivers/etc/
cp /etc/networks ~/.wine/drive_c/windows/system32/drivers/etc/
cp /etc/protocols ~/.wine/drive_c/windows/system32/drivers/etc/protocol #special case
cp /etc/services ~/.wine/drive_c/windows/system32/drivers/etc/

#cd ~/.wine/drive_c/msys32/usr/bin/
