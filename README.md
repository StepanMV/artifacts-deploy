
# Зависимости
- Qt 5 или Qt 6 (тестировал на 6.7.1 и 5.15.14)
- libssh (https://www.libssh.org/), версия для Windows включена в репозиторий
- libzip  (https://libzip.org/), версия для Windows включена в репозиторий

# Сборка
Стандартная сборка cmake'ом
Необходимо указать флаг QT_VERSION_MAJOR (=5 or 6) для выбора версии Qt
На windows также необходимо добавить в Path включенные в репозиторий dll
