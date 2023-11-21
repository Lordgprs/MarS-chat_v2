# CHAT

## Сетевой чат

## Состав команды:
 - Максим Вельгач (https://github.com/Lordgprs) - тимлид. Отвечает за реализацию хранения и отправки сообщений и за общую структуру кода
 - Сергей Маркин (https://github.com/MarS-37) - отвечает за структуру классов ChatServer, ChatClient (управление), ChatUser (пользователи), за хранение списка пользователей в памяти, за обработку команд вида /command

Для хэширования паролей использован open-source класс SHA256, распространяемый по лицензии MIT: https://github.com/System-Glitch/SHA256/blob/master/LICENSE

## ОБЩЕЕ ОПИСАНИЕ:

Настоящий чат представляет из себя две самостоятельные программы: клиент и выделенный сервер. Связь между клиентом и сервером осуществляется по протоколу TCP.
Сервер поддерживает одновременное подключение множества клиентов, каждого клиента обрабатывает собственная копия родительского процесса, созданная при помощи системного вызова fork() - 
классическая схема TCP-сервера для Linux. Список пользователей и история сообщений автоматически сохраняется в файл. Пароли хранятся в файле в виде хэша SHA256. Конфигурация содержится в файлах
client.cfg и server.cfg

Для синхронизации процессов на сервере используется СУБД MySQL. Также в базе данных хранится информация о пользователях, текущих соединениях и сообщениях (история переписок).

Для синхронизации процессов на клиенте используются временные файлы, создаваемые в каталоге /tmp: /tmp/chat_server и /tmp/chat_client.

Формат конфигурационных файлов:
 - Variable = Value: строка, содержащая значение параметра конфигурации
 - \# comment: строка, содержащая комментарий

Допустимые параметры конфигурации сервера:
 - ListenPort: порт, на котором сервер принимает входящие соединения

Допустимые параметры конфигурации клиента:
 - ServerAddress: IP сервера
 - ServerPort: порт сервера

## РЕАЛИЗОВАНЫЙ ФУНКЦИОНАЛ (КЛИЕНТ):
 
Выход из программы (команда /exit или /quit). Также корректно обрабатывается сигнал SIGINT, поступаемый при нажатии Ctrl-C.

Вывод справки по работе программы (команда /help) 

Регистрация в чате (команда /signup)
 - при регистрации вводим имя логин и пароль
 - обработка исключений: логин и пароль не должен быть пустым
 - обработка исключений: попытка регистрации уже зарегистрированного пользователя
 - обработка исключений: если есть авторизованный пользователь, регистрация недоступна
 - логин пользователя может состоять только из A-Z,a-z,"-","_"
 
Авторизация в чате (команда /signin)
 - при авторизации вводим логин и пароль
 - обработка исключений: проверка регистрации логина
 - обработка исключений: проверка пароля пользователя
 - обработка исключений: если есть авторизованный пользователь, авторизация недоступна
 - одновременный вход с одной учётной записи с разных клиентов запрещён
 - при успешной авторизации вывести приветствие по имени пользователя

Выход авторизованного пользователя (команда /logout)

Удаление авторизованного пользователя (команда /remove)

## РЕАЛИЗОВАННЫЙ ФУНКЦИОНАЛ (СЕРВЕР):

Выход из программы (команда /exit, /quit или комбинация клавиш Ctrl-C

Вывод справки по работе программы (команда /help)

Список активных клиентов (команда /list)

Отключение активного клиента (команда /kick username)

Удаление неактивного пользователя (команда /remove username)

Интерфейс отправки сообщений:
 - если есть авторизованный пользователь и введен текст, текст отправляется как 
	сообщение для всех пользователей
 - если сообщение будет начинаться с @username где username - логин зарегистрированного пользователя,
	оно будет отправлено сообщение пользователю username
 - не подходящий к этим условиям введеный текст не рассматривается программой
 
## ТЕХНИЧЕСКОЕ ОПИСАНИЕ:

 Пользовательские типы:
 - ChatUser: класс для хранения информации о пользователях
 - ChatMessage: абстрактный класс, описывающий интерфейс работы с сообщениями. Объявлены чистые виртуальные функции:
 print() - печать сообщения, 
 printIfUnreadByUser() - печать сообщения только если оно ещё не прочитано активным пользователем,
 isRead() - проверка, прочитано ли сообщение активным пользователем
 - PrivateMessage: унаследованный от ChatMessage класс для работы с личными сообщениями
 - BroadcastMessage: унаследованный от ChatMessage класс для работы с широковещательными сообщениями
 - ChatServer: основной класс серверной части, содержащий метод work(), отвечающий за работу программы.
 - ChatClient: основной класс клиентской части, содержащий метод work(), отвечающий за работу программы.
 - ConfigFile: класс, отвечающий за парсинг конфигурационных файлов
 - Mysql: RAII-обёртка для API MySQL для языка Си

 Дополнительно проект содержит файлы project_lib.h и project_lib.cpp. Данные файлы содержат функцию split(), отвечающую за разбиение строки на части с использованием заданного разделителя.
 Данную функцию было решено вынести за пределы всех классов, так как она используется почти всеми классами. Функция объявлена в пространстве имён Chat.

## ПОДДЕРЖКА ОС:

 В настоящее время в связи с использованием большого количества системных вызовов Linux, поддержка Windows временно прекращена. В будущем планируется переход на средства
 стандартной библиотеки языка, в частности использование потоков STL вместо параллельных процессов, после чего поддержка Windows будет возобновлена.
 
