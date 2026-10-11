// The example as 0.8.0 - 0.8.5 made it, kept only to recognise it (see
// SampleData::legacy in SampleData.h). Copied verbatim: a changed character
// here makes an untouched old example count as the person's work.

#include "SampleData.h"

namespace SampleData::legacy {

namespace {
Task mkTask(const char* id, const char* title, const char* desc, const char* pri, const char* status, const QDate& dl, const char* branch) {
  Task t;
  t.id = id;
  t.title = QString::fromUtf8(title);
  t.desc = QString::fromUtf8(desc);
  t.priority = pri;
  t.status = status;
  if(dl.isValid()) {
    t.scheduledAt = QDateTime(dl, QTime(0, 0));
    t.dueAt = t.scheduledAt;
  }
  t.branch = branch;
  return t;
}

// 0.8.0 shipped one starter note.
QVector<Note> notes080(Lang lang) {
  Note n;
  n.id = QStringLiteral("note-welcome");
  n.created = QDateTime::currentDateTime();
  n.updated = n.created;
  if(lang == Lang::Ru) {
    n.title = QStringLiteral("С чего начать");
    n.body = QStringLiteral(
        "# С чего начать\n\n"
        "Заметки — это markdown: **жирный**, *курсив*, `код`, списки и таблицы.\n\n"
        "- [ ] Открыть задачу APP-101 на доске\n"
        "- [ ] Перетащить задачу в календарь справа\n"
        "- [x] Прочитать эту заметку\n\n"
        "Ссылка на задачу: [[APP-101]].\n\n"
        "> Нажмите «Начать с чистого листа» на баннере, чтобы убрать демо-данные.\n");
  } else {
    n.title = QStringLiteral("Getting started");
    n.body = QStringLiteral(
        "# Getting started\n\n"
        "Notes are markdown: **bold**, *italic*, `code`, lists and tables.\n\n"
        "- [ ] Open APP-101 on the board\n"
        "- [ ] Drag a task onto the calendar on the right\n"
        "- [x] Read this note\n\n"
        "Link to a task: [[APP-101]].\n\n"
        "> Press \"Start fresh\" on the banner to clear the demo data.\n");
  }
  return {n};
}

// 0.8.1 - 0.8.5: the notes of the Knowledge sheet.
QVector<Note> notes081(Lang lang) {
  // The notes of the Knowledge sheet (H2-Knowledge, DG-074): the open one
  // first, so the example opens on it. Each says only what the app does now.
  const QDateTime now = QDateTime::currentDateTime();
  auto mk = [&now](const char* id, const QString& title, const QString& body, int minutes_ago) {
    Note n;
    n.id = QString::fromLatin1(id);
    n.title = title;
    n.body = body;
    // Made and last edited at the same moment: the list shows the newest first.
    n.created = now.addSecs(-60LL * minutes_ago);
    n.updated = n.created;
    return n;
  };
  if(lang == Lang::Ru) {
    return {
        mk("note-rate-limit",
           QStringLiteral("Дизайн rate-limit"),
           QStringLiteral("# Дизайн rate-limit\n\n"
                          "Лимитер считает попытки по ключу `login:<email>`. Баг в [[APP-101]] — ключ не нормализован.\n\n"
                          "## Что отвечаем клиенту\n\n"
                          "429 с заголовком `Retry-After`, как в "
                          "[RFC 6585](https://www.rfc-editor.org/rfc/rfc6585 \"429 Too Many Requests\"). "
                          "Тело — без подсказки, существует ли аккаунт.\n\n"
                          "## Открытые вопросы\n\n"
                          "- Окно 15 мин или скользящее? Спросить Олега.\n"
                          "- Нужна ли капча после 5 попыток — см. [[APP-108]]\n"),
           5),
        mk("note-retro-41",
           QStringLiteral("Ретро спринта 41"),
           QStringLiteral("# Ретро спринта 41\n\n"
                          "## Что получилось\n\n"
                          "- Экспорт CSV ушёл на ревью вовремя.\n\n"
                          "## Что мешало\n\n"
                          "- Лимит попыток входа: разбираем в [[Дизайн rate-limit]].\n"),
           60 * 26),
        mk("note-release-checklist",
           QStringLiteral("Чек-лист релиза"),
           QStringLiteral("# Чек-лист релиза\n\n"
                          "- [ ] Все задачи релиза в колонке «Готово»\n"
                          "- [ ] Changelog обновлён\n"
                          "- [ ] Тег поставлен, сборка зелёная\n"),
           60 * 50),
        mk("note-welcome",
           QStringLiteral("С чего начать"),
           QStringLiteral("# С чего начать\n\n"
                          "Заметки — это markdown: **жирный**, *курсив*, `код`, списки и таблицы.\n\n"
                          "- [ ] Открыть задачу APP-101 на доске\n"
                          "- [ ] Наберите / в пустой строке — чек-лист, код, ссылка на задачу\n"
                          "- [x] Прочитать эту заметку\n\n"
                          "Ссылка на задачу: [[APP-101]].\n\n"
                          "> Пример живёт в своём профиле: переключите профиль вверху боковой панели, чтобы начать своё.\n"),
           60 * 72),
    };
  }
  return {
      mk("note-rate-limit",
         QStringLiteral("Rate-limit design"),
         QStringLiteral("# Rate-limit design\n\n"
                        "The limiter counts attempts by the key `login:<email>`. The bug in [[APP-101]]: the key is not "
                        "normalised.\n\n"
                        "## What the client gets\n\n"
                        "429 with a `Retry-After` header, as in "
                        "[RFC 6585](https://www.rfc-editor.org/rfc/rfc6585 \"429 Too Many Requests\"). "
                        "The body does not hint whether the account exists.\n\n"
                        "## Open questions\n\n"
                        "- A 15 min window or a sliding one? Ask Oleg.\n"
                        "- Captcha after 5 attempts? See [[APP-108]]\n"),
         5),
      mk("note-retro-41",
         QStringLiteral("Sprint 41 retro"),
         QStringLiteral("# Sprint 41 retro\n\n"
                        "## What went well\n\n"
                        "- CSV export reached review on time.\n\n"
                        "## What got in the way\n\n"
                        "- The login attempt limit: worked out in [[Rate-limit design]].\n"),
         60 * 26),
      mk("note-release-checklist",
         QStringLiteral("Release checklist"),
         QStringLiteral("# Release checklist\n\n"
                        "- [ ] Every task of the release is in Done\n"
                        "- [ ] Changelog updated\n"
                        "- [ ] Tag pushed, build green\n"),
         60 * 50),
      mk("note-welcome",
         QStringLiteral("Getting started"),
         QStringLiteral("# Getting started\n\n"
                        "Notes are markdown: **bold**, *italic*, `code`, lists and tables.\n\n"
                        "- [ ] Open APP-101 on the board\n"
                        "- [ ] Type / on an empty line: a checklist, code, a link to a task\n"
                        "- [x] Read this note\n\n"
                        "Link to a task: [[APP-101]].\n\n"
                        "> The example lives in its own profile: switch profiles at the top of the sidebar to start "
                        "your own.\n"),
         60 * 72),
  };
}

}  // namespace

QVector<Task> tasks(Lang lang) {
  const QDate today = QDate::currentDate();
  auto d = [&](int n) {
    return today.addDays(n);
  };
  if(lang == Lang::Ru) {
    return {
        mkTask("APP-101",
               "Обход ограничения попыток входа",
               "После сброса пароля счётчик попыток иногда обнуляется — возможен перебор. Воспроизводится на мобильных клиентах.",
               "P0",
               "prog",
               d(1),
               "fix/login-rate-limit"),
        mkTask("APP-102",
               "Постраничный API: сдвиг курсора на больших списках",
               "По результатам профилирования: при удалении элементов во время прокрутки курсор пропускает страницу.",
               "P1",
               "prog",
               d(4),
               "feat/api-pagination"),
        mkTask("APP-103",
               "Дашборд: пустое состояние для новых аккаунтов",
               "Не уверен, какие метрики показывать новым и активным аккаунтам. Нужен совет от Олега.",
               "P1",
               "half",
               d(2),
               "feat/dashboard-widgets"),
        mkTask("APP-104",
               "Поиск: некорректное пустое состояние",
               "При нулевой выдаче иногда мелькает старый список перед сообщением «ничего не найдено».",
               "P2",
               "half",
               d(6),
               "fix/search-empty-state"),
        mkTask("APP-105",
               "Оформление заказа: таймаут 10с",
               "Жду трассировку от QA: воспроизводится только у одного клиента с медленной сетью.",
               "P0",
               "blocked",
               d(0),
               "fix/checkout-timeout"),
        mkTask("APP-106",
               "Экспорт в CSV: постраничная выгрузка",
               "PR #4471. Жду Андрея, ревью небольшое.",
               "P2",
               "review",
               d(3),
               "feat/csv-export"),
        mkTask("APP-107",
               "Генерация превью изображений: кэширование",
               "PR #4488. +18% скорости на сервере, −3% памяти — обсудить компромисс.",
               "P1",
               "review",
               d(2),
               "perf/image-thumbnails"),
        mkTask("APP-108",
               "Гонка при обновлении профиля",
               "Сохранение аватара иногда приходит до завершения обновления профиля — гонка на общем состоянии.",
               "P1",
               "todo",
               d(5),
               ""),
        mkTask("APP-109",
               "Рефакторинг обработчика вебхуков",
               "Разбить монолитный обработчик на 1,4 тыс. строк на проверку, маршрутизацию и доставку.",
               "P2",
               "todo",
               d(8),
               ""),
        mkTask("APP-110",
               "Настройки уведомлений: переключатели по каналам",
               "Добавить настройки для каждого канала в экран уведомлений.",
               "P2",
               "todo",
               d(9),
               ""),
        mkTask("APP-111",
               "Утечка памяти в хранилище сессий",
               "Профилировщик чист, влито в master сегодня утром.",
               "P1",
               "done",
               d(-1),
               "fix/session-memory-leak"),
        mkTask("APP-112", "Логи: структурированные поля для API-запросов", "Готово, смёржено.", "P3", "done", d(-2), ""),
        mkTask("APP-113",
               "Улучшения внутреннего логирования",
               "Медленное логирование в горячем цикле, нужен кольцевой буфер.",
               "P3",
               "backlog",
               {},
               ""),
        mkTask("APP-114",
               "Экспорт: граничный случай пустого отчёта",
               "Граничный случай: пустой отчёт и отчёт из одной строки при низкой нагрузке.",
               "P3",
               "backlog",
               {},
               ""),
        mkTask("APP-115", "Тюнинг порога кэша поиска", "Профилировать на нагрузке >200 запросов/сек.", "P3", "backlog", {}, ""),
    };
  }
  // English seed (default).
  return {
      mkTask("APP-101",
             "Login rate-limit bypass",
             "After a password reset the attempt counter sometimes resets — brute force is possible. Repros on mobile clients.",
             "P0",
             "prog",
             d(1),
             "fix/login-rate-limit"),
      mkTask("APP-102",
             "API pagination: cursor drift on large lists",
             "Profiling outcome: when items are deleted mid-scroll, the cursor skips a page.",
             "P1",
             "prog",
             d(4),
             "feat/api-pagination"),
      mkTask("APP-103",
             "Dashboard: empty state for new accounts",
             "Unsure which metrics to show for new vs active accounts. Need Oleg's input.",
             "P1",
             "half",
             d(2),
             "feat/dashboard-widgets"),
      mkTask("APP-104",
             "Search: incorrect empty state",
             "On a zero-result query the old list flashes before the 'nothing found' message.",
             "P2",
             "half",
             d(6),
             "fix/search-empty-state"),
      mkTask("APP-105",
             "Checkout: 10s timeout",
             "Waiting on QA trace: only repros for one customer on a slow network.",
             "P0",
             "blocked",
             d(0),
             "fix/checkout-timeout"),
      mkTask("APP-106",
             "CSV export: paginated download",
             "PR #4471. Waiting on Andrey, small review.",
             "P2",
             "review",
             d(3),
             "feat/csv-export"),
      mkTask("APP-107",
             "Image thumbnail generation: caching",
             "PR #4488. +18% speed on the server, -3% memory — discuss trade-off.",
             "P1",
             "review",
             d(2),
             "perf/image-thumbnails"),
      mkTask("APP-108",
             "Race condition on profile update",
             "Avatar save sometimes arrives before the profile update completes — race on shared state.",
             "P1",
             "todo",
             d(5),
             ""),
      mkTask("APP-109",
             "Refactor the webhook handler",
             "Split the 1.4k LOC monolith into validation + routing + delivery.",
             "P2",
             "todo",
             d(8),
             ""),
      mkTask("APP-110",
             "Notification settings: granular toggles",
             "Add per-channel support to the notification settings path.",
             "P2",
             "todo",
             d(9),
             ""),
      mkTask("APP-111",
             "Memory leak in session store",
             "Profiler clean, merged into master this morning.",
             "P1",
             "done",
             d(-1),
             "fix/session-memory-leak"),
      mkTask("APP-112", "Logs: structured fields for API requests", "Done, merged.", "P3", "done", d(-2), ""),
      mkTask(
          "APP-113", "Internal logging improvements", "Slow-path logs inside the hot loop — need a ring buffer.", "P3", "backlog", {}, ""),
      mkTask("APP-114",
             "Export: empty report edge case",
             "Edge case: empty report vs single-row report under low load.",
             "P3",
             "backlog",
             {},
             ""),
      mkTask("APP-115", "Search cache threshold tuning", "Profile under load >200 req/s.", "P3", "backlog", {}, ""),
  };
}

QStringList eventTitles() {
  // By title: the meetings' dates followed the day they were made on.
  return {QStringLiteral("Daily standup"),
          QStringLiteral("1:1 with Oleg"),
          QStringLiteral("Focus block"),
          QStringLiteral("Sprint planning"),
          QStringLiteral("Code review"),
          QStringLiteral("Дейли стендап"),
          QStringLiteral("1:1 с Олегом"),
          QStringLiteral("Фокус-блок"),
          QStringLiteral("Планирование спринта"),
          QStringLiteral("Ревью кода")};
}

QVector<Note> notes(Lang lang) {
  return notes080(lang) + notes081(lang);
}

}  // namespace SampleData::legacy
