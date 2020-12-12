# dozer
ПО контроллера рыбной кормушки
## Порядок установки инструментария для сборки
1. Устанавливаем GNU Arm Embedded Toolchain:
   - Загружаем файл gcc-arm-none-eabi-8-2018-q4-major-win32.exe со страницы: [ссылка](https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-win32.exe?revision=ac917577-a723-4619-b2f0-86a551638834?product=GNU%20Arm%20Embedded%20Toolchain,32-bit,,Windows,8-2018-q4-major)
   - Запускаем загруженный установщик и устанавливаем тулчейн в папку c:/gnuarm
2. Устанавливаем MSYS2:
   - Загружаем установщик для вашей системы (32 или 64 бит) со страницы http://www.msys2.org/
   - Запускаем загруженный установщик и устанавливаем msys2 в папку по умолчанию (C:\msys64 или C:\msys32 в зависимости от типа установщика). В конце установки MSYS2 запустится автоматически
   - В окне MSYS2 введите команду: `pacman -Suy` - происходит обновление системы. На все вопросы системы отвечайте Y. Если во время обновления произойдёт ошибка, введите команду `pacman -Suy` снова.
   - После обновления система попросит перезапустить MSYS2: закройте окно MSYS2 нажав на кнопку "X" в правом верхнем углу окна (не набирайте команду exit!!!). Во всплывающем окне предупреждения нажмите "Ok"
   - Вновь запустите MSYS2: Пуск->Мои программы->MSYS2 64bit (или MSYS2 32bit)->MSYS2 MSYS и снова введите команду `pacman -Suy`. Система продолжит обновление. На все вопросы отвечайте Y
   - После завершения обновления, установите make: `pacman -S make` После завершения установки система готова к сборке ПО Aqual
3. Устанавливаем OpenOCD:
   - распаковываем архив openocd.zip в корень диска C: - программа OpenOCD установлена
## Сборка ПО Aqual
1. В окне MSYS2 перейдите в папку с деревом исходных текстов ПО Aqual (в ту папку которая содержит этот файл README.md) набрав команду: `cd <путь к папке дерева исходных текстов>` Путь необходимо набирать по следующим правилам:
   - вместо названий дисков C:\ или D:\ следует набирать /c/ или /d/
   - вместо обратных слэшей следует набирать прямые
   - если в названии папки встречаются пробелы, перед символом пробела следует ставить обратный слэш
2. Вводим `make clean`
3. Вводим `make` - начинается сборка ПО
4. После окончания сборки, в корневом каталоге дерева будет находится файл dozer.elf - это и есть файл прошивки.
## Программирование микроконтроллера
Для программирования микроконтроллера кормушки необходим программатор-отладчик STLink v2 mini.

1. Подключаем:
   - цепь GND программатора к контакту 1 разъёма X1
   - цепь SWCLK к контакту 2 разъёма X1
   - цепь SWDIO к контакту 3 разъёма X1

2. Подключаем программатор к USB разъёму компьютера. Windows должна автоматически найти и установить драйвера для устройства STLink dongle. Если этого не произошло, воспользуйтесь программой установки драйверов C:\openocd\drivers\UsbDriverTool.exe
3. Подайте питание на кормушку.
4. Находясь в каталоге сборки (там где файлы README.md и dozer.elf) в окне MSYS2 вводим команду: `make upload` - запускается программа OpenOCD и начинается программирование микроконтроллера.
