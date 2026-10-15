.pragma library

// "What's new" per release line (R2-054): what the line changed for someone
// coming from the one before, in the UI language. Facts of the release only
// — every entry here is something the build does. A line without an entry
// shows nothing after an update.
//
// line: "0.8" for 0.8.0, 0.8.1, …
var notes = {
    "0.9": {
        sections: [
            { title: { en: "Move to profile", ru: "Перенос в профиль" },
              body: { en: "M → “Move to profile…”, or the selection bar; Ctrl N and quick capture take “+Work” for the profile",
                      ru: "M → «Перенести в профиль…» или панель выделения; Ctrl N и быстрый ввод понимают «+Работа»" } },
            { title: { en: "Repeats", ru: "Повторы" },
              body: { en: "“every 2 weeks”, “every mon, wed”, “every other day”; the next ones show faintly in Week and Month",
                      ru: "«каждые 2 недели», «каждый пн и ср», «через день»; следующие видны бледно в Неделе и Месяце" } },
            { title: { en: "Time zones", ru: "Часовые пояса" },
              body: { en: "a meeting keeps its own zone: pick it in the meeting, or type “15:00 EST”; your local time stays next to it",
                      ru: "у встречи свой пояс: выбрать во встрече или написать «15:00 по Москве»; рядом — ваше время" } },
            { title: { en: "Today for every profile", ru: "Сегодня по всем профилям" },
              body: { en: "Settings → Calendar → “Today shows”: this profile or all of them",
                      ru: "Настройки → Календарь → «Сегодня» показывает: этот профиль или все" } },
            { title: { en: "Hints that show once", ru: "Подсказки один раз" },
              body: { en: "when Tabless does something on its own — folds a merge request into its Jira ticket, keeps a move local, reads dates from text — it says so once, beside the thing, with where to change it",
                      ru: "когда Tabless делает что-то сам — склеивает MR с тикетом Jira, сдвигает карточку только у себя, понимает даты из текста, — он один раз говорит об этом рядом и подсказывает, где это изменить" } },
            { title: { en: "Notes as on the page", ru: "Заметки как на листе" },
              body: { en: "rich mode by default: airier lines, “/” on an empty line for blocks, task links as pills, inline code and pictures drawn in place",
                      ru: "rich-режим по умолчанию: просторные строки, «/» на пустой строке — блоки, ссылки на задачи плашками, код и картинки рисуются на месте" } },
            { title: { en: "Blue theme", ru: "Синяя тема" },
              body: { en: "Settings → Appearance → Themes: “Tabless blue”, the blue-black palette of the first designs; grey stays the default",
                      ru: "Настройки → Внешний вид → Темы: «Синяя», сине-чёрная палитра первых макетов; серая остаётся по умолчанию" } },
            { title: { en: "Fewer stumbles", ru: "Меньше заминок" },
              body: { en: "about two hundred fixes from a day-long test by twenty kinds of engineers: keys, dates in Russian, the week grid, end of day, undo",
                      ru: "около двухсот исправлений после теста «рабочий день» двадцати разных инженеров: клавиши, даты по-русски, сетка недели, конец дня, отмена" } }
        ],
        moved: { en: "“When” and “Due” are now “Target start” and “Due date” · Scale and Contrast → Settings → Appearance, under More · Docs got a key: g d · n / p step the week",
                 ru: "«Когда» и «Срок» теперь «Целевое начало» и «Срок исполнения» · Масштаб и Контраст → Настройки → Внешний вид, под «ещё» · у Доков своя клавиша: g d · n / p листают неделю" }
    },
    "0.8": {
        sections: [
            { title: { en: "New name", ru: "Новое имя" },
              body: { en: "lowkey is now called Tabless; your data and settings are where you left them",
                      ru: "lowkey теперь называется Tabless; данные и настройки на месте" } },
            { title: { en: "Today", ru: "Сегодня" },
              body: { en: "the home screen: the day by the hour, in progress, deadlines",
                      ru: "главный экран: день по часам, в работе, сроки" } },
            { title: { en: "Tasks", ru: "Задачи" },
              body: { en: "board, list and calendar — one section with one filter",
                      ru: "доска, список и календарь — один раздел с общим фильтром" } },
            { title: { en: "Task as a document", ru: "Задача — документ" },
              body: { en: "a panel on the right, saves by itself, “Done” — d",
                      ru: "панель справа, сохраняется сама, «Готово» — d" } },
            { title: { en: "One input", ru: "Один ввод" },
              body: { en: "“tomorrow 15:00 p1” is understood wherever a task is created",
                      ru: "«завтра 15:00 p1» понимается везде, где создаётся задача" } },
            { title: { en: "Two styles", ru: "Два стиля" },
              body: { en: "“Bold” by default, “Quiet” in Settings → Appearance",
                      ru: "«Насыщенный» по умолчанию, «Тихий» — в Настройки → Внешний вид" } }
        ],
        moved: { en: "“Focus” and “Saved views” → “My views” · Timeline, Week, Month → Tasks · Tweaks → Settings",
                 ru: "«Фокус» и «Сохранённые виды» → «Мои виды» · Лента, Неделя, Месяц → Задачи · Твики → Настройки" }
    }
};

function lineOf(version) {
    var m = /^(\d+)\.(\d+)/.exec(String(version || ""));
    return m ? m[1] + "." + m[2] : "";
}

function forVersion(version) {
    return notes[lineOf(version)] || null;
}
