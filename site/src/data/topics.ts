// Pages for what people search for ("offline kanban board", "offline task manager"), one per
// language. Each answers one query with facts from site/PROMISES.md; the home page stays the tour.
import type { Lang } from './i18n';

export interface Topic {
  slug: string;
  /** Footer link text. */
  link: string;
  title: string;
  description: string;
  kicker: string;
  h: string;
  lede: string;
  /** Picture under the lede: a file in public/img and its alt text. */
  img: { src: string; alt: string };
  cards: [string, string][];
  moreH: string;
  more: string[];
  faqH: string;
  faq: [string, string][];
  ctaH: string;
  ctaP: string;
}

const en: Topic[] = [
  {
    slug: 'offline-kanban-board',
    link: 'Offline kanban board',
    title: 'Offline kanban board for developers — Tabless',
    description:
      'A free, open-source kanban board that works offline: your tasks and the issues assigned to you from GitHub, Jira, GitLab and 10 more trackers, in columns you drive from the keyboard. No account.',
    kicker: 'Offline kanban board',
    h: 'An offline kanban board for your own work',
    lede:
      'Tabless is a free, open-source desktop app with a kanban board that needs no network and no account. Your own tasks and the issues assigned to you from GitHub, Jira, GitLab and 10 more trackers sit in columns you drive from the keyboard.',
    img: { src: 'img/board.png', alt: 'Tabless kanban board: tasks by status in columns — Backlog, To Do, In Progress, On hold, Blocked, Review' },
    cards: [
      ['Works offline', 'Everything is one state.json file on your disk. No account, no server, no analytics. The network is used only by trackers and calendars you connect, and by an update check you can turn off.'],
      ['Columns by status', 'Backlog, To Do, In Progress, On hold, Blocked, Review and Done, with your own columns next to them. Move a card with the mouse or with one keypress.'],
      ['Tracker issues, read-only', 'Issues assigned to you land in the column of their status. Moving a card changes nothing in the tracker unless you switch status updates on for that tracker.'],
    ],
    moreH: 'More than a board',
    more: [
      'Vim-style keys that work in any keyboard layout: j and k through a column, g b to the board, / to filter, Ctrl K for the command line.',
      'The same tasks as a list, a week calendar and a month, and a Today screen with the day’s meetings by the hour.',
      'Quick capture from any app: Ctrl Shift Space, then “fix login tomorrow 14:00 p1 #auth” — date, time, priority and label are read from the words.',
      'Markdown notes next to the tasks they mention, private notes and a checklist on any tracker issue.',
      'Daily backups (the newest 20 kept) and a time machine of snapshots you can step back through.',
    ],
    faqH: 'Questions',
    faq: [
      ['Is it free?', 'Yes. Open source under the MIT licence, with no paid tier, no trial and no account.'],
      ['Does it need the internet?', 'No. The board, your tasks and notes work offline. Only tracker sync and calendar links need the network, and only if you connect them.'],
      ['Can my team share the board?', 'No, it’s a personal board. Your team’s tracker stays the shared place; Tabless shows your part of it next to your own tasks.'],
      ['Which systems does it run on?', 'Windows 10 and 11, macOS 12 and later on Apple Silicon, and Linux as an AppImage.'],
    ],
    ctaH: 'Your board, on your disk',
    ctaP: 'Free and open source. Windows, macOS and Linux.',
  },
  {
    slug: 'offline-task-manager',
    link: 'Offline task manager',
    title: 'Offline task manager with no account — Tabless',
    description:
      'A free, open-source task manager that works offline: tasks, today’s meetings and Markdown notes in one JSON file on your disk. No sign-up, no analytics. Windows, macOS, Linux.',
    kicker: 'Offline task manager',
    h: 'An offline task manager with no account',
    lede:
      'Tabless keeps your tasks, today’s meetings and your notes in one JSON file on your disk. It runs without a network, needs no sign-up and sends no analytics. Free and open source, for Windows, macOS and Linux.',
    img: { src: 'img/today.png', alt: 'Tabless Today screen: the day’s meetings and planned tasks by the hour, with what is in progress and due' },
    cards: [
      ['Nothing to sign up for', 'Download, open, write the first task. There is no account and no cloud, and nothing runs in the background once the window and tray icon are closed.'],
      ['One file you can read', 'All data is one state.json you can open in any editor or copy as a backup. Daily backups and a time machine keep earlier versions.'],
      ['Capture from any app', 'Press Ctrl Shift Space anywhere and type the task the way you’d say it. Date, time, priority and label are read from the words, in English or Russian.'],
    ],
    moreH: 'What else is inside',
    more: [
      'A Today screen with the day’s meetings and planned tasks by the hour.',
      'A kanban board, a list, a week calendar and a month view of the same tasks.',
      'Optional: tasks assigned to you from 13 trackers, read-only by default, and meetings from your calendar’s .ics link.',
      'Markdown notes where #APP-112 becomes a link to the task.',
      'A command line for the terminal: `tabless today`, `tabless add`, `tabless now`.',
    ],
    faqH: 'Questions',
    faq: [
      ['Is it free?', 'Yes. Open source under the MIT licence, with no paid tier and no account.'],
      ['What needs the network?', 'Only what you connect: tracker sync, calendar links, and an update check on start that you can turn off. Without them everything works offline.'],
      ['Where is my data?', 'In one state.json file in the app’s data folder, with backups next to it. Tracker tokens are kept out of that file.'],
      ['Can I use it on two computers?', 'By moving it: export the profile to JSON and import it on the other machine, or copy state.json while the app is closed. There’s no live sync yet.'],
    ],
    ctaH: 'Your tasks, on your disk',
    ctaP: 'Free and open source. Windows, macOS and Linux.',
  },
];

const ru: Topic[] = [
  {
    slug: 'offline-kanban-board',
    link: 'Офлайн канбан-доска',
    title: 'Офлайн канбан-доска для разработчиков — Tabless',
    description:
      'Бесплатная канбан-доска с открытым кодом, которая работает без интернета: ваши задачи и задачи из GitHub, Jira, GitLab и ещё 10 трекеров в колонках, с клавиатуры. Без аккаунта.',
    kicker: 'Офлайн канбан-доска',
    h: 'Офлайн канбан-доска для своей работы',
    lede:
      'Tabless — бесплатное десктопное приложение с открытым кодом, где есть канбан-доска без сети и без аккаунта. Ваши задачи и задачи, назначенные на вас в GitHub, Jira, GitLab и ещё 10 трекерах, лежат в колонках, которыми вы управляете с клавиатуры.',
    img: { src: 'img/board.png', alt: 'Канбан-доска Tabless: задачи по статусам в колонках' },
    cards: [
      ['Работает без сети', 'Всё хранится одним файлом state.json на вашем диске. Ни аккаунта, ни сервера, ни аналитики. Сеть нужна только трекерам и календарям, которые вы подключите, и проверке обновлений, которую можно выключить.'],
      ['Колонки по статусам', 'Backlog, To Do, In Progress, On hold, Blocked, Review и Done, а рядом ваши собственные колонки. Карточка переносится мышью или одной клавишей.'],
      ['Задачи трекера — только чтение', 'Задачи, назначенные на вас, попадают в колонку своего статуса. Перенос карточки ничего не меняет в трекере, пока вы сами не включите запись статуса для этого трекера.'],
    ],
    moreH: 'Не только доска',
    more: [
      'Клавиши в стиле Vim работают в любой раскладке: j и k по колонке, g b — доска, / — фильтр, Ctrl K — командная строка.',
      'Те же задачи списком, календарём на неделю и на месяц, и экран Today со встречами дня по часам.',
      'Быстрый ввод из любого приложения: Ctrl Shift Space и «позвонить Ане завтра в 15:00 p1» — дата, время и приоритет читаются из текста.',
      'Markdown-заметки рядом с задачами, свои заметки и чеклист на любой задаче трекера.',
      'Ежедневные бэкапы (хранятся последние 20) и машина времени со снимками.',
    ],
    faqH: 'Вопросы',
    faq: [
      ['Это бесплатно?', 'Да. Открытый код под лицензией MIT, без платного тарифа, пробного периода и аккаунта.'],
      ['Нужен ли интернет?', 'Нет. Доска, задачи и заметки работают без сети. Интернет нужен только синхронизации трекеров и ссылкам на календари, и только если вы их подключите.'],
      ['Можно ли вести доску командой?', 'Нет, это личная доска. Общим местом остаётся командный трекер, а Tabless показывает вашу часть рядом с вашими задачами.'],
      ['На каких системах работает?', 'Windows 10 и 11, macOS 12 и новее на Apple Silicon и Linux в виде AppImage.'],
    ],
    ctaH: 'Ваша доска, на вашем диске',
    ctaP: 'Бесплатно и с открытым кодом. Windows, macOS и Linux.',
  },
  {
    slug: 'offline-task-manager',
    link: 'Офлайн менеджер задач',
    title: 'Офлайн менеджер задач без аккаунта — Tabless',
    description:
      'Бесплатный менеджер задач с открытым кодом, который работает без интернета: задачи, встречи дня и Markdown-заметки в одном JSON-файле на вашем диске. Без регистрации и аналитики.',
    kicker: 'Офлайн менеджер задач',
    h: 'Менеджер задач без интернета и без аккаунта',
    lede:
      'Tabless хранит ваши задачи, встречи дня и заметки в одном JSON-файле на вашем диске. Он работает без сети, не требует регистрации и не отправляет аналитику. Бесплатно и с открытым кодом, для Windows, macOS и Linux.',
    img: { src: 'img/today.png', alt: 'Экран Today в Tabless: встречи и задачи дня по часам' },
    cards: [
      ['Регистрироваться не нужно', 'Скачали, открыли, записали первую задачу. Аккаунта и облака нет, а когда окно и иконка в трее закрыты, в фоне ничего не работает.'],
      ['Один файл, который можно прочитать', 'Все данные — один state.json, который открывается в любом редакторе и копируется как бэкап. Ежедневные бэкапы и машина времени хранят прошлые версии.'],
      ['Запись из любого приложения', 'Нажмите Ctrl Shift Space где угодно и напишите задачу, как сказали бы вслух. Дата, время, приоритет и метка читаются из текста, по-русски и по-английски.'],
    ],
    moreH: 'Что ещё внутри',
    more: [
      'Экран Today со встречами и задачами дня по часам.',
      'Канбан-доска, список, календарь на неделю и на месяц для тех же задач.',
      'По желанию: задачи, назначенные на вас, из 13 трекеров, по умолчанию только чтение, и встречи по ссылке .ics из вашего календаря.',
      'Markdown-заметки, в которых #APP-112 становится ссылкой на задачу.',
      'Командная строка для терминала: `tabless today`, `tabless add`, `tabless now`.',
    ],
    faqH: 'Вопросы',
    faq: [
      ['Это бесплатно?', 'Да. Открытый код под лицензией MIT, без платного тарифа и аккаунта.'],
      ['Чему нужна сеть?', 'Только тому, что вы подключите: синхронизации трекеров, ссылкам на календари и проверке обновлений при запуске, которую можно выключить. Без них всё работает офлайн.'],
      ['Где мои данные?', 'В одном файле state.json в папке данных приложения, рядом лежат бэкапы. Токены трекеров в этот файл не попадают.'],
      ['Можно на двух компьютерах?', 'Переносом: экспортируйте профиль в JSON и импортируйте на другой машине или скопируйте state.json, пока приложение закрыто. Синхронизации в реальном времени пока нет.'],
    ],
    ctaH: 'Ваши задачи, на вашем диске',
    ctaP: 'Бесплатно и с открытым кодом. Windows, macOS и Linux.',
  },
];

export const topics: Record<Lang, Topic[]> = { en, ru };

export function topic(lang: Lang, slug: string): Topic {
  const t = topics[lang].find((x) => x.slug === slug);
  if (!t) throw new Error(`no topic ${slug} for ${lang}`);
  return t;
}
