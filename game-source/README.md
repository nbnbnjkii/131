# ЭКРОН — исходники текущей Android-игры

Отдельный самодостаточный снимок нативной игры **v0.37-suspension-pairs**.
База — `22a025201d375e6863dba1fb916185783f166bef`, с исправлениями контактов текущей ветки.
Подробности текущего этапа и ограничения: [ADVANCED_PHYSICS_RU.md](sys/android/ADVANCED_PHYSICS_RU.md).
Предыдущие исправления: [CONTACT_SYSTEM_RU.md](sys/android/CONTACT_SYSTEM_RU.md).
Не сайт и не WebView: Kotlin + C++17 + OpenGL ES 3.

## Содержимое

- `sys/android/` — Android Studio / Gradle проект, Kotlin-интерфейс, C++-движок,
  модели города и УАЗа, текстуры, встроенное интро и конфигурация.
- `tests/native/` — нативные регрессионные тесты.
- `tools/` — проверка шейдеров и генератор текстур.
- `.github/workflows/android-sandbox.yml` — сборка в GitHub Actions, если содержимое
  этой папки поместить в корень отдельного репозитория.
- `LICENSE` — лицензия исходного репозитория. Лицензии стороннего кода находятся
  также в заголовках third_party; сведения о моделях — в документации sys/android.
  Разрешение на дальнейшее распространение сторонней модели города нужно проверить
  отдельно: лицензия репозитория не заменяет лицензию модели.

## Собрать APK

Открыть **`game-source/sys/android`** в Android Studio.
Нужны JDK 17, Android SDK 34, Build Tools 34.0.0, NDK 26.1.10909125,
CMake 3.22.1 и доступ к Maven/Google для загрузки зависимостей.

Либо из этой папки:

```sh
cd sys/android
./gradlew :app:assembleDebug :app:testDebugUnitTest
```

На Windows: `gradlew.bat :app:assembleDebug :app:testDebugUnitTest`.
Путь APK: `sys/android/app/build/outputs/apk/debug/app-debug.apk`.
Это debug APK для установки и тестирования, не подписанный релиз для магазина.

## Проверить C++ без Android

Из корня **game-source**:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic tests/native/pickup_settle_delete_test.cpp -o /tmp/ekron-test
/tmp/ekron-test
```

## Проект и готовая сборка

- GitHub: https://github.com/nbnbnjkii/ekron
- APK (ZIP): https://github.com/nbnbnjkii/ekron/actions/workflows/android-sandbox.yml
  Выберите успешную сборку нужной ветки, затем артефакт `ekron-native-sandbox-debug`.
  Для скачивания может понадобиться вход в GitHub. Срок хранения ограничен.

Это снимок, а не автоматически синхронизируемая копия. Основная разработка пока
остаётся в `/sys/android` исходного репозитория; дальнейшие изменения там не
появятся в этой папке сами. В снимке сохранены ограничения собственного физического решателя;
наличие исходников не означает устранения всех проблем на телефоне.
