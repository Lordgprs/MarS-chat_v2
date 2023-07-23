# chat_v2
for skillfactory

версия консольного чата

ПЛАНИРУЕТСЯ РЕАЛИЗОВАТЬ:
Выход из программы (команда /exit)
Вывод справки по работе программы (команда /help) 

Регистрация в чате (команда /signup)
 - при регистрации вводим имя логин и пароль
 - обработка исключений: логин и пароль не должен быть пустым
 - обработка исключений: попытка регистрации уже зарегистрированного пользователя
 - обработка исключений: если есть авторизованный пользователь, регистрация недоступна
 
Авторизация в чате (команда /signin)
 - при авторизации вводим логин и пароль
 - обработка исключений: проверка регистрации логина
 - обработка исключений: проверка пароля пользователя
 - обработка исключений: если есть авторизованный пользователь, авторизация недоступна
 - при успешной авторизации вывести приветствие по имени пользователя

Выход авторизованного пользователя (команда /logout)

РЕАЛИЗОВАНО:
 - закрытие программы командой /exit
 - вывод справки по использованию программы командой /help
 - при регистрации проверяем ввод пустого логина или пароля
 - при регистрации проверяем не используется ли логин уже зарегистрированным пользователем
 - при авторизации проверяем правильность пароля зарегистрированного пользователя
 - при успешной авторизации вывести приветствие по имени пользователя
 - при удачной авторизации в строку ввода выводится логин авторизованного пользователя
 - при авторизованом пользователе не доступна регистрация
 - выход авторизованного пользователя