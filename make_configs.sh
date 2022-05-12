rm -f config1.txt
echo /usr/bin/bc -l $PWD/in01.txt $PWD/out01.txt >> config1.txt
echo /usr/bin/sleep 15 $PWD/in02.txt $PWD/out02.txt >> config1.txt
echo /usr/bin/sort $PWD/in03.txt $PWD/out03.txt >> config1.txt
rm -f config2.txt
echo /usr/bin/bc -l $PWD/in04.txt $PWD/out04.txt >> config2.txt
