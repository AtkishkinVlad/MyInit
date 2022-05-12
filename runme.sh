# Удаление выходных файлов
rm -f out* /tmp/myinit*
# Компиляция
make
# Первый вариант конфигурационного файла
cp config1.txt config.txt
# Запуск minit
./myinit $PWD/config.txt
sleep 1
# Демонстрация myinit и его потомков
echo "Myinit and it's children"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo
# Убиваем sleep
sleep 1
# Демонстрация myinit и его потомков
echo "Myinit and it's children:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo
# Второй вариант конфигурационного файла
cp config2.txt config.txt
# Сигнал myinit на перезапуск
pkill -SIGHUP myinit
sleep 1
# Демонстрация myinit и его потомков
echo "Myinit and it's children:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo
# Сигнал myinit на завершение работы
pkill -SIGINT myinit
sleep 1
# Проверка завершения myinit
echo "Myinit:"
pgrep -a myinit
echo
