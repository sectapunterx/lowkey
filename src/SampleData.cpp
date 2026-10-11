#include "SampleData.h"

// The example (APP-271): one believable day of a developer on a small web
// team, built so that Today, Tasks and Notes each show what the README says
// within a glance. Content rules: calm and factual, no exclamation marks, no
// emoji; fictional people and product; nothing on a card depends on showing
// a person; nothing suggests the app plans or decides for the user.

namespace {

using SampleData::Lang;

// One sample task. The fields exampleChanges() compares (title, desc,
// status, priority) never hold a date, so the comparison holds on any day.
struct T {
  Task t;

  T(const char* id, const QString& title, const QString& desc, const char* pri, const char* status) {
    t.id = QString::fromLatin1(id);
    t.title = title;
    t.desc = desc;
    t.priority = QString::fromLatin1(pri);
    t.status = QString::fromLatin1(status);
  }

  // Planned for a day, with a time when `h` >= 0.
  T& scheduled(const QDate& day, int h = -1, int m = 0) {
    t.scheduledAt = QDateTime(day, QTime(h >= 0 ? h : 0, h >= 0 ? m : 0));
    t.scheduledHasTime = h >= 0;
    return *this;
  }

  // Owed by a day, with a time when `h` >= 0.
  T& due(const QDate& day, int h = -1, int m = 0) {
    t.dueAt = QDateTime(day, QTime(h >= 0 ? h : 0, h >= 0 ? m : 0));
    t.dueHasTime = h >= 0;
    return *this;
  }

  T& label(const char* l) {
    t.labels.append(Label{.id = QString::fromLatin1(l), .color = QString()});
    return *this;
  }

  T& estimate(int minutes) {
    t.estimateMinutes = minutes;
    return *this;
  }

  T& branch(const char* b) {
    t.branch = QString::fromLatin1(b);
    return *this;
  }

  T& blocks(const char* id) {
    t.links.append(TaskLink{.type = QStringLiteral("blocks"), .targetId = QString::fromLatin1(id)});
    return *this;
  }

  T& check(const QString& text, bool done) {
    LocalCheckItem c;
    c.id = t.id.toLower() + QStringLiteral("-ck") + QString::number(t.local.checklist.size() + 1);
    c.text = text;
    c.done = done;
    t.local.checklist.append(c);
    return *this;
  }

  T& movedAt(const QDateTime& when) {
    t.statusChangedAt = when;
    return *this;
  }

  // A stretch the timer ran, kept in step with trackedSeconds.
  T& tracked(const QDateTime& start, int minutes) {
    TimerSession s;
    s.id = t.id.toLower() + QStringLiteral("-s") + QString::number(t.local.sessions.size() + 1);
    s.start = start;
    s.end = start.addSecs(60LL * minutes);
    t.local.sessions.append(s);
    t.trackedSeconds += 60 * minutes;
    return *this;
  }

  operator Task() const {
    return t;
  }
};

QString pick(Lang lang, const char* en, const char* ru) {
  return QString::fromUtf8(lang == Lang::Ru ? ru : en);
}

}  // namespace

namespace SampleData {

QVector<QVariantMap> statuses(Lang lang) {
  // { id, name, color }
  if(lang == Lang::Ru) {
    return {
        {{"id", "backlog"}, {"name", "Бэклог"}, {"color", QColor("#8a8e98")}, {"category", "backlog"}},
        {{"id", "todo"}, {"name", "К выполнению"}, {"color", QColor("#9aa3b4")}, {"category", "todo"}},
        {{"id", "prog"}, {"name", "В работе"}, {"color", QColor("#5aa9e6")}, {"category", "prog"}},
        {{"id", "half"}, {"name", "На паузе"}, {"color", QColor("#dcb86b")}, {"category", "half"}},
        {{"id", "blocked"}, {"name", "Заблокировано"}, {"color", QColor("#e6624c")}, {"category", "blocked"}},
        {{"id", "review"}, {"name", "Ревью"}, {"color", QColor("#a4a4d6")}, {"category", "review"}},
        {{"id", "done"}, {"name", "Готово"}, {"color", QColor("#6ec18a")}, {"category", "done"}},
    };
  }
  return {
      {{"id", "backlog"}, {"name", "Backlog"}, {"color", QColor("#8a8e98")}, {"category", "backlog"}},
      {{"id", "todo"}, {"name", "To Do"}, {"color", QColor("#9aa3b4")}, {"category", "todo"}},
      {{"id", "prog"}, {"name", "In Progress"}, {"color", QColor("#5aa9e6")}, {"category", "prog"}},
      {{"id", "half"}, {"name", "On hold"}, {"color", QColor("#dcb86b")}, {"category", "half"}},
      {{"id", "blocked"}, {"name", "Blocked"}, {"color", QColor("#e6624c")}, {"category", "blocked"}},
      {{"id", "review"}, {"name", "Review"}, {"color", QColor("#a4a4d6")}, {"category", "review"}},
      {{"id", "done"}, {"name", "Done"}, {"color", QColor("#6ec18a")}, {"category", "done"}},
  };
}

QVector<Task> tasks(Lang lang) {
  const QDate today = QDate::currentDate();
  const QDateTime now = QDateTime::currentDateTime();
  const auto d = [&today](int n) {
    return today.addDays(n);
  };
  const auto tr = [lang](const char* en, const char* ru) {
    return pick(lang, en, ru);
  };

  // Today, around the meetings (standup 9:45, design review 11:30, 1:1 14:00,
  // planning 16:30): APP-112 10:00, APP-121 13:30, APP-119 15:00.
  return {
      // In progress, planned this morning, with a branch named after it.
      T("APP-112",
        tr("Flaky sync test on CI", "Нестабильный тест синхронизации в CI"),
        tr("`SyncTest.resumesAfterTimeout` fails about one run in twenty on the Linux runner and passes locally. "
           "The test waits a fixed 200 ms for the retry.",
           "`SyncTest.resumesAfterTimeout` падает примерно в одном прогоне из двадцати на Linux-раннере, "
           "локально проходит. Тест ждёт повтор фиксированные 200 мс."),
        "P1",
        "prog")
          .label("ci")
          .scheduled(d(0), 10, 0)
          .estimate(90)
          .due(d(1))
          .branch("APP-112-flaky-sync-test")
          .check(tr("Reproduce with --gtest_repeat=200", "Воспроизвести с --gtest_repeat=200"), true)
          .check(tr("Find the fixed wait", "Найти фиксированное ожидание"), true)
          .check(tr("Wait on the signal instead of the clock", "Ждать сигнала, а не времени"), false)
          .check(tr("500 green runs on CI", "500 зелёных прогонов в CI"), false)
          .tracked(QDateTime(d(-1), QTime(16, 0)), 50)
          .movedAt(QDateTime(d(-1), QTime(15, 55))),
      // Blocked, due today, waiting on QA (SampleData::waitingOn).
      T("APP-115",
        tr("Checkout: 502 on the second payment attempt", "Оплата: 502 на второй попытке"),
        tr("After a timeout the retry gets a 502 from the payment gateway. Reported in web#418. "
           "Needs the staging request log; #APP-119 makes it findable.",
           "После таймаута повторный запрос получает 502 от платёжного шлюза. Пришло в web#418. "
           "Нужен лог запросов со стенда; найти его поможет #APP-119."),
        "P0",
        "blocked")
          .label("payments")
          .due(d(0), 17, 0)
          .movedAt(QDateTime(d(-1), QTime(11, 20))),
      // What unblocks it, planned for the afternoon.
      T("APP-119",
        tr("Gateway logs: add a request id", "Логи шлюза: добавить id запроса"),
        tr("Without a request id a staging log line cannot be matched to a checkout attempt.",
           "Без id запроса строку лога со стенда не связать с конкретной попыткой оплаты."),
        "P1",
        "todo")
          .label("payments")
          .scheduled(d(0), 15, 0)
          .estimate(60)
          .blocks("APP-115"),
      T("APP-121",
        tr("Review web#431: dark theme tokens", "Ревью web#431: токены тёмной темы"),
        tr("Lena's pull request. Check the muted text contrast in both themes.",
           "Пулреквест Лены. Проверить контраст приглушённого текста в обеих темах."),
        "P2",
        "review")
          .label("ui")
          .scheduled(d(0), 13, 30)
          .estimate(30),
      // Overdue since yesterday.
      T("APP-108",
        tr("Password reset link expires after 15 minutes", "Ссылка сброса пароля живёт 15 минут"),
        tr("The email says 60 minutes. Reported in web#402.", "В письме написано 60 минут. Пришло в web#402."),
        "P1",
        "todo")
          .label("auth")
          .due(d(-1)),
      // Planned for today, no time.
      T("APP-117",
        tr("Release 2.4 notes", "Заметки к релизу 2.4"),
        tr("Everything merged since 2.3. What is left is in [[Release 2.4 checklist]].",
           "Всё, что влито после 2.3. Что осталось — в [[Чек-лист релиза 2.4]]."),
        "P2",
        "todo")
          .label("docs")
          .scheduled(d(0)),
      // Due tomorrow.
      T("APP-126",
        tr("Onboarding: empty state for new accounts", "Онбординг: пустое состояние для новых аккаунтов"),
        tr("Pull request web#433, waiting for review.", "Пулреквест web#433 ждёт ревью."),
        "P1",
        "review")
          .label("ui")
          .due(d(1))
          .branch("feat/onboarding-empty-state")
          .movedAt(QDateTime(d(-1), QTime(17, 40))),
      // Planned and due on different days.
      T("APP-123",
        tr("Move rate limits into config", "Вынести лимиты запросов в конфиг"),
        tr("Today they are constants in three services. One config key per route.",
           "Сейчас это константы в трёх сервисах. Один ключ конфига на маршрут."),
        "P2",
        "todo")
          .label("api")
          .scheduled(d(1))
          .due(d(3))
          .blocks("APP-127"),
      T("APP-129",
        tr("Swap on-call week with Maya", "Поменяться дежурством с Ольгой"),
        tr("Ask in the 1:1.", "Спросить на 1:1."),
        "P3",
        "todo")
          .due(d(2)),
      T("APP-109",
        tr("Search: old results flash before the empty state", "Поиск: старые результаты мелькают перед пустым состоянием"),
        tr("On hold until the new search endpoint is merged in the api repo.",
           "На паузе, пока новый эндпоинт поиска не влит в репозиторий api."),
        "P2",
        "half")
          .label("ui"),
      T("APP-127",
        tr("Webhooks: retry with backoff", "Вебхуки: повтор с нарастающей задержкой"),
        tr("Today a failed delivery is dropped after one try.", "Сейчас неудачная доставка теряется после первой попытки."),
        "P2",
        "backlog")
          .label("api"),
      T("APP-110", tr("CI image: move to Ubuntu 24.04", "Образ CI: перейти на Ubuntu 24.04"), QString(), "P3", "backlog").label("ci"),
      T("APP-128",
        tr("CSV export: stream rows instead of building the file in memory",
           "Экспорт в CSV: отдавать строки потоком, а не собирать файл в памяти"),
        tr("Large accounts hit the 512 MB limit of the worker.", "Большие аккаунты упираются в лимит воркера 512 МБ."),
        "P3",
        "backlog")
          .label("api"),
      T("APP-113",
        tr("Session store keeps memory after logout", "Хранилище сессий не освобождает память после выхода"),
        tr("Fixed in web#425, merged.", "Исправлено в web#425, влито."),
        "P1",
        "done")
          .label("api")
          .movedAt(now.addDays(-1)),
      T("APP-114",
        tr("Structured fields in API request logs", "Структурированные поля в логах API-запросов"),
        tr("Merged.", "Влито."),
        "P3",
        "done")
          .label("api")
          .movedAt(now.addDays(-2)),
  };
}

QVector<WaitingOn> waitingOn() {
  // Asked yesterday. Marked as already reminded: the example never sends a
  // reminder of its own.
  const QDateTime since = QDateTime(QDate::currentDate().addDays(-1), QTime(16, 10));
  return {WaitingOn{.taskId = QStringLiteral("APP-115"), .personId = QStringLiteral("qa"), .since = since, .remindedAt = since}};
}

QVector<CalEvent> events(const QDate& today, Lang lang) {
  auto mk = [&](const char* id, const QString& title, const char* type, double s, double e, const QString& att, int off) {
    CalEvent ev;
    ev.id = QString::fromLatin1(id);
    ev.title = title;
    ev.type = QString::fromLatin1(type);
    ev.start = s;
    ev.end = e;
    ev.attendees = att;
    ev.date = today.addDays(off);
    return ev;
  };
  const auto tr = [lang](const char* en, const char* ru) {
    return pick(lang, en, ru);
  };
  const QString standup = tr("Daily standup", "Стендап");
  const QString team = tr("Web team", "Веб-команда");
  CalEvent review = mk("ev-review",
                       tr("Design review: checkout errors", "Дизайн-ревью: ошибки оплаты"),
                       "sync",
                       11.5,
                       12.25,
                       tr("Maya, Tom, Ines", "Ольга, Тимур, Ирина"),
                       0);
  review.taskId = QStringLiteral("APP-115");
  return {
      // Today: four meetings.
      mk("ev-standup-0", standup, "standup", 9.75, 10.0, team, 0),
      review,
      mk("ev-1on1", tr("1:1 with Maya", "1:1 с Ольгой"), "oneone", 14.0, 14.5, tr("Maya R.", "Ольга Р."), 0),
      mk("ev-planning", tr("Sprint 42 planning", "Планирование спринта 42"), "sync", 16.5, 17.25, team, 0),
      // The rest of the week, so Week and Month are not empty.
      mk("ev-standup-1", standup, "standup", 9.75, 10.0, team, 1),
      mk("ev-gonogo", tr("Release 2.4 go/no-go", "Релиз 2.4: выпускаем или нет"), "sync", 15.0, 15.5, team, 1),
      mk("ev-standup-2", standup, "standup", 9.75, 10.0, team, 2),
      mk("ev-focus", tr("Focus: rate limits", "Фокус: лимиты запросов"), "focus", 13.0, 15.0, QString(), 2),
      mk("ev-demo", tr("Release 2.4 demo", "Демо релиза 2.4"), "sync", 11.0, 11.5, team, 3),
  };
}

QVector<Person> people(Lang lang) {
  // Ids are the same in both languages and are not the legacy "p1" form, so
  // they stay as they are; waitingOn() names them.
  auto mk = [lang](const char* id,
                   const char* en,
                   const char* ru,
                   const char* role,
                   const char* q_en,
                   const char* q_ru,
                   const char* st,
                   const QColor& c) {
    Person p;
    p.id = QString::fromLatin1(id);
    p.name = pick(lang, en, ru);
    p.role = QString::fromLatin1(role);
    p.question = pick(lang, q_en, q_ru);
    p.state = QString::fromLatin1(st);
    p.color = c;
    if(p.state != QLatin1String("todo")) {
      p.stateAt = QDateTime::currentDateTime().addDays(-1);
    }
    return p;
  };
  return {
      mk("lead",
         "Maya R.",
         "Ольга Р.",
         "Tech lead",
         "Swap on-call weeks in November?",
         "Поменяться неделями дежурства в ноябре?",
         "todo",
         QColor("#d97a6c")),
      mk("qa",
         "Sam O.",
         "Саша О.",
         "QA",
         "Staging request log for the 502 on checkout (APP-115)",
         "Лог запросов со стенда по 502 на оплате (APP-115)",
         "pinged",
         QColor("#c87fc7")),
      mk("backend",
         "Lena K.",
         "Лена К.",
         "Backend",
         "Can you review web#433 today?",
         "Посмотришь web#433 сегодня?",
         "todo",
         QColor("#6cc4b8")),
      mk("design",
         "Tom B.",
         "Тимур Б.",
         "Design",
         "Final copy for the search empty state (APP-109)",
         "Финальный текст для пустого поиска (APP-109)",
         "replied",
         QColor("#7da8d9")),
      mk("pm",
         "Ines P.",
         "Ирина П.",
         "PM",
         "Does APP-123 go into 2.4 or can it wait?",
         "APP-123 идёт в 2.4 или может подождать?",
         "todo",
         QColor("#dcc06a")),
  };
}

QStringList captureExamples(Lang lang) {
  if(lang == Lang::Ru) {
    return {QStringLiteral("починить вход 15:00 p1 #auth"),
            QStringLiteral("посмотреть MR поиска завтра 11:00 ~30м"),
            QStringLiteral("разослать заметки о релизе до пт p2")};
  }
  return {QStringLiteral("fix login 15:00 p1 #auth"),
          QStringLiteral("review the search MR tomorrow 11:00 ~30m"),
          QStringLiteral("send the release notes by fri p2")};
}

QVector<Note> notes(Lang lang) {
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
  const QString capture = captureExamples(lang).join(QLatin1Char('\n'));

  if(lang == Lang::Ru) {
    Note guide = mk("note-example-guide",
                    QStringLiteral("Как устроен этот пример"),
                    QStringLiteral("# Как устроен этот пример\n\n"
                                   "Это выдуманный рабочий день в небольшой веб-команде. Пример не подключён ни к одному "
                                   "трекеру: APP-112 только похож на ключ Jira, а web#431 — на пулреквест в GitHub. "
                                   "Меняйте, двигайте и удаляйте что угодно: ваш собственный профиль это не затронет.\n\n"
                                   "## Куда смотреть\n\n"
                                   "- **Сегодня** — четыре встречи, задачи между ними, сроки и те, чьего ответа вы ждёте.\n"
                                   "- **Задачи** — та же работа на доске, от бэклога до «Готово»: одна задача в работе, "
                                   "одна заблокирована, у одной вышел срок.\n"
                                   "- **Заметки** — Markdown со ссылками на задачи: `#APP-112` в тексте становится "
                                   "ссылкой с названием задачи, вот так: #APP-112.\n\n"
                                   "## Клавиши\n\n"
                                   "| Клавиша | Что делает |\n"
                                   "| --- | --- |\n"
                                   "| `?` | все клавиши, с поиском |\n"
                                   "| `Ctrl+K` | командная строка: задачи, заметки, команды |\n"
                                   "| `g b` | доска |\n"
                                   "| `j` / `k` | следующая / предыдущая |\n"
                                   "| `d` | готово; на готовой задаче — вернуть обратно |\n\n"
                                   "Это клавиши по умолчанию. Любую можно сменить в «Настройки → Клавиши».\n\n"
                                   "## Быстрый ввод\n\n"
                                   "Быстрый ввод задачи работает из любого приложения и читает одну строку:\n\n"
                                   "```\n") +
                        capture +
                        QStringLiteral("\n```\n\n"
                                       "Дата сама по себе — когда вы собираетесь этим заняться. После «до» — это срок. "
                                       "`p0`–`p3` — приоритет, `#слово` — метка, `~30м` — оценка.\n\n"
                                       "## План и срок\n\n"
                                       "У задачи может быть и то и другое, как у #APP-123: в плане на завтра, срок — "
                                       "через два дня после этого. «Сегодня» показывает план и сроки отдельно.\n\n"
                                       "## Когда насмотритесь\n\n"
                                       "В меню профиля вверху боковой панели есть «Убрать пример». Если вы здесь что-то "
                                       "меняли, сначала будет вопрос.\n"),
                    1);
    guide.pinned = true;
    Note runbook = mk("note-example-runbook",
                      QStringLiteral("Ранбук: сброс стенда"),
                      QStringLiteral("# Ранбук: сброс стенда\n\n"
                                     "Когда данные на стенде разошлись с сидом, например после неудачной миграции.\n\n"
                                     "1. Предупредить в чате команды, что стенд уходит.\n"
                                     "2. Сбросить и засеять:\n\n"
                                     "```sh\n"
                                     "make staging-down\n"
                                     "make staging-seed SEED=demo\n"
                                     "make staging-up\n"
                                     "```\n\n"
                                     "3. Проверить страницу health, затем оплату тестовой картой.\n\n"
                                     "| Шаг | Обычно занимает |\n"
                                     "| --- | --- |\n"
                                     "| Сид | 4 мин |\n"
                                     "| Прогрев | 2 мин |\n\n"
                                     "> Скрипты читают `STAGE` и останавливаются, если там не `staging`.\n\n"
                                     "Последний раз нужен был для #APP-115.\n"),
                      60 * 30);
    runbook.folder = QStringLiteral("Ранбуки");
    return {
        guide,
        mk("note-example-standup",
           QStringLiteral("Заметки к стендапу"),
           QStringLiteral("# Заметки к стендапу\n\n"
                          "## Вчера\n\n"
                          "- #APP-113: влито.\n"
                          "- #APP-112: причина найдена, тест ждёт повтор фиксированные 200 мс.\n\n"
                          "## Сегодня\n\n"
                          "- #APP-112: ждать сигнала, а не времени, потом 500 прогонов в CI.\n"
                          "- #APP-119 — без неё не сдвинуть #APP-115.\n"
                          "- Ревью web#431.\n\n"
                          "## Блокеры\n\n"
                          "- #APP-115: ждёт лог запросов со стенда от Саши.\n"),
           20),
        mk("note-example-release",
           QStringLiteral("Чек-лист релиза 2.4"),
           QStringLiteral("# Чек-лист релиза 2.4\n\n"
                          "- [x] Заморозка фич\n"
                          "- [x] #APP-113 влита\n"
                          "- [ ] #APP-112 зелёный 500 раз подряд\n"
                          "- [ ] #APP-115 исправлена или перенесена в 2.5\n"
                          "- [ ] Заметки к релизу, #APP-117\n"
                          "- [ ] Тег поставлен, сборка зелёная\n\n"
                          "Решение «выпускаем или нет» — в календаре.\n"),
           60 * 5),
        runbook,
    };
  }

  Note guide = mk("note-example-guide",
                  QStringLiteral("How this example works"),
                  QStringLiteral("# How this example works\n\n"
                                 "This profile is a made-up workday on a small web team. Nothing in it is connected to a "
                                 "tracker: APP-112 only looks like a Jira key, and web#431 like a GitHub pull request. "
                                 "Edit, move or delete anything. Your own profile is not touched.\n\n"
                                 "## Where to look\n\n"
                                 "- **Today**: four meetings, the tasks planned between them, what is due and who you are "
                                 "waiting on.\n"
                                 "- **Tasks**: the same work as a board, from the backlog to done. One task is in "
                                 "progress, one is blocked, one is overdue.\n"
                                 "- **Notes**: Markdown, linked to tasks. `#APP-112` in a note becomes a link with the "
                                 "task's title, like this: #APP-112.\n\n"
                                 "## Keys\n\n"
                                 "| Key | What it does |\n"
                                 "| --- | --- |\n"
                                 "| `?` | every key, with a search |\n"
                                 "| `Ctrl+K` | command line: tasks, notes, commands |\n"
                                 "| `g b` | the board |\n"
                                 "| `j` / `k` | next / previous |\n"
                                 "| `d` | done; on a done task, back |\n\n"
                                 "These are the defaults. Settings → Keys changes any of them.\n\n"
                                 "## Quick capture\n\n"
                                 "Quick capture works from any app and reads one line:\n\n"
                                 "```\n") +
                      capture +
                      QStringLiteral("\n```\n\n"
                                     "A date on its own is when you plan to do it. After `by` it is the deadline. "
                                     "`p0`–`p3` is the priority, `#word` a label, `~30m` an estimate.\n\n"
                                     "## Planned and due\n\n"
                                     "A task can have both, like #APP-123: planned for tomorrow, due two days after. "
                                     "Today shows the plan and the deadlines apart.\n\n"
                                     "## When you are done looking\n\n"
                                     "The profile menu at the top of the sidebar has “Remove the example”. If you "
                                     "changed anything here, it asks first.\n"),
                  1);
  guide.pinned = true;
  Note runbook = mk("note-example-runbook",
                    QStringLiteral("Runbook: reset staging"),
                    QStringLiteral("# Runbook: reset staging\n\n"
                                   "For when staging data no longer matches the seed, for example after a failed "
                                   "migration.\n\n"
                                   "1. Say in the team chat that staging is going down.\n"
                                   "2. Reset and seed:\n\n"
                                   "```sh\n"
                                   "make staging-down\n"
                                   "make staging-seed SEED=demo\n"
                                   "make staging-up\n"
                                   "```\n\n"
                                   "3. Check the health page, then a checkout with a test card.\n\n"
                                   "| Step | Usually takes |\n"
                                   "| --- | --- |\n"
                                   "| Seed | 4 min |\n"
                                   "| Warm-up | 2 min |\n\n"
                                   "> The scripts read `STAGE` and stop unless it is `staging`.\n\n"
                                   "Last needed for #APP-115.\n"),
                    60 * 30);
  runbook.folder = QStringLiteral("Runbooks");
  return {
      guide,
      mk("note-example-standup",
         QStringLiteral("Standup notes"),
         QStringLiteral("# Standup notes\n\n"
                        "## Yesterday\n\n"
                        "- #APP-113: merged.\n"
                        "- #APP-112: found the cause, the test waits a fixed 200 ms for the retry.\n\n"
                        "## Today\n\n"
                        "- #APP-112: wait on the signal instead of the clock, then 500 runs on CI.\n"
                        "- #APP-119 — it unblocks #APP-115.\n"
                        "- Review web#431.\n\n"
                        "## Blocked\n\n"
                        "- #APP-115: waiting on the staging request log from Sam.\n"),
         20),
      mk("note-example-release",
         QStringLiteral("Release 2.4 checklist"),
         QStringLiteral("# Release 2.4 checklist\n\n"
                        "- [x] Feature freeze\n"
                        "- [x] #APP-113 merged\n"
                        "- [ ] #APP-112 green 500 times in a row\n"
                        "- [ ] #APP-115 fixed or moved to 2.5\n"
                        "- [ ] Release notes, #APP-117\n"
                        "- [ ] Tag pushed, build green\n\n"
                        "The go/no-go is in the calendar.\n"),
         60 * 5),
      runbook,
  };
}

}  // namespace SampleData
