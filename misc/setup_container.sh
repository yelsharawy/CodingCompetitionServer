#!/bin/sh

C=$1
shift 1

for vital in bin usr lib lib32 libx32 lib64 etc dev
do
    # echo mounting /$vital/
    mkdir $C/$vital/
    # chmod -t $C/$vital/ # doesn't seem to work or do anything
    # chown root $C/$vital/
    mount --rbind -o ro /$vital/ $C/$vital/ #2> /dev/null
done

for other in proc tmp var var/tmp home
do
    mkdir $C/$other/
    # chmod -t $C/$other/
    # chown root $C/$other/
done

if [ $1 ]
then
    mount --rbind $1 $C/home
    # ^ intentionally not read-only
    shift 1
fi

cd $C

export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
hostname container
env -i PATH=$PATH HOME='/home' unshare -U --fork --pid --mount-proc -R . "${@:-bash}"

# env -i PATH=$PATH HOME='/home' unshare --fork --pid -R . "${@:-bash}"
# mount -t proc /proc proc/