rm -f out* /tmp/myinit*

make

cp config1.txt config.txt

./myinit $PWD/config.txt
sleep 1

echo "Myinit and it's children"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

sleep 1

echo "Myinit and it's children:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

cp config2.txt config.txt

pkill -SIGHUP myinit
sleep 1

echo "Myinit and it's children:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

pkill -SIGINT myinit
sleep 1

echo "Myinit:"
pgrep -a myinit
echo
