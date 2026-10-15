# Design gaps 0.9.4 — app vs New heap design (2026-10-15)

Two auditors, 78 sheets (both styles), offscreen captures of the real Main.qml at 1440×900, 100 % scale. Only Qt limitation found: native OS notifications — decided in DESIGN-DECISIONS (0.9.4). Everything below is fixable in QML/C++; not fixed yet. **Owner review 2026-10-15:** 25 rows are not gaps (deliberate, never visible, or owner: keep) — listed at the end; b-11 counts only scale/contrast at rest (font row is APP-314). 37 remain, plus the binding loops b-01/b-23 (log only, fixed anyway on the owner's call). Decisions for a-07, a-23, b-09, b-21 below.


Counts: QT 1 (B) · BUG 37 (2 A, 15 B, 20 C) · HARNESS 7 · DECIDED ~25.
Rig: a\tst_Cap.qml, a\run2\tst_Cap2.qml; 1440×900, ru, scale forced 100 %; grabImage(win.contentItem).

## QT
- a-01 N/X-Ntf-OS — native OS toasts (NotificationCenter_win.cpp): layout/font/buttons owned by the OS. Alternative: keep native, match title/body/button labels+order (done R3-025/026); in-app Toast shows block-start buttons.

## BUG
| id | sheet | sev | sheet | app | files |
|---|---|---|---|---|---|
| a-02 | Knowledge | A | body line height 1.7 | ~1.2: default "rich" TextArea has no leading; 1.7 only in MdView (live, tests only) | NotesView.qml:24,1151; MdView.qml:103,361 |
| a-03 | Oth-Knowledge | A | "/" on empty line → insert menu | rich editor: "/" only expands date words; menu only in MdBlockEditor live | NotesView.qml:1189-1240; MdBlockEditor.qml:460,903 |
| a-04 | Knowledge | B | bullets hanging indent | "•" over source; wrap under bullet | MdLiveDecorations.qml:89-100 |
| a-05 | Knowledge | B | task link pill (bold) / ring+underline (quiet) | underlined title, no ring/pill | MdLiveDecorations.qml:197-217 |
| a-06 | Knowledge | B | inline code padded pill | lavender mono, no fill/padding | MdHighlighter.cpp |
| a-07 | Knowledge | B | H1 30/600, H2 18/600 | weight 700, H2 ~22 | MdHighlighter.cpp:922,1365 |
| a-08 | Knowledge | B | title plain | caret on title on open → "# " visible | NotesView.qml |
| a-09 | Oth-Knowledge | B | image block + caption | `![]()` as source | MdLiveDecorations.qml:76-86 |
| a-10 | Oth-Knowledge | C | code header lang left, "копировать", hairline | no rule, capitalised | MdLiveDecorations.qml:269-311 |
| a-11 | Oth-Knowledge | B | plain 32px working-on line | lavender pill, icon, badges, "открыть" | TopBar.qml:384-520 |
| a-12 | H2-First | B | "Мои виды" + "Появятся, когда сохраните фильтр" | hidden on first run | Sidebar.qml:454,475 |
| a-13 | H2-First | B | sheet copy (subtitle/footer/button) | different copy | I18n.qml:4409,4416,4417; FirstRunHero.qml |
| a-14 | H2-First | C | field then key cards | extra "Вне поля (Esc…)" line | FirstRunHero.qml:216-224 |
| a-15 | H2-First | C | 1.5px bright border r12, title 22 | dim border, ~20 | FirstRunHero.qml:114-122 |
| a-16 | Q-First | B | title/field/2 hints/links | extra subtitle + Трекеры line | FirstRunHero.qml |
| a-17 | Q-First | C | "? — все клавиши" | "Ctrl / —" | FirstRunHero.qml:203 |
| a-18 | H2-Today | B | now-working: id·PR·CI·timer | branch name when no PR | TodayView.qml:712-718 |
| a-19 | H2-Today | C | dashed free-time rule | solid | TodayView.qml:1444 |
| a-20 | H2-Today | C | people rows only | "+ Добавить человека" | TodayView.qml |
| a-21 | bold keys | C | plain mono 11 key text | framed keycaps | KeyHint.qml:17-35 |
| a-22 | Today/DayMonth/Small | C | times in JetBrains Mono 11 | sans tabular | TodayView.qml:1284; WeekView hours |
| a-23 | H2-Today sidebar | C | first row y=120, pitch 32 | y=107, pitch 34 | Sidebar.qml |
| a-24 | H2-Today-Calm | C | first row y≈147 | y≈124 | TodayView.qml |
| a-25 | Oth-Small | C | compact layout at 1280 | minimumWidth 1100 = threshold → never compact at 100 % | Main.qml:20; Theme.qml:381-385 |
| a-26 | Oth-Small | C | sun/list/doc icons | calendar/list/pencil/doc | Sidebar.qml |
| a-27 | Ntf-Toasts | B | one toast "Готово: 3 задачи +2 события Журнал" | burst replaces, no count/Журнал | Toast.qml:107-133,455-458 |
| a-28 | Ntf-OS, Err-Storage | B | bold primary filled | outlined | WhatsNewDialog.qml:134; ReportIssueDialog.qml:141 |
| a-29 | X-Err-Empty | B | quiet outlined | filled always | DoneColumnDialog.qml:29 |
| a-30 | Oth-Select-Drag | B | drop target caption "только здесь" for write-off tracker | none | KanbanBoard.qml:985,1835 |
| a-31 | Command | C | — | event date "15 окт. 2026" + group name | AppController.cpp:15333; I18n.qml:3852 |
| a-32 | Oth-Capture | C | three footer hints | + "Ctrl M встреча · Tab документ" | QuickCapturePopup.qml:1022 |
| a-33 | Keys | C | one dim footer | accent footer + extra | KeyCheatSheet.qml |
| a-34 | Ntf-Focus | C | crash copy "Данные целы…" | longer copy | SplashScreen.qml / I18n |
| a-35 | Ntf-Focus | C | checkbox 13, timer 15 | ~10/13 | ImmersionView.qml |
| a-36 | Oth-DayMonth | C | "+2 ещё" accent, 11px | "+6 ещё · 3 встречи" dim 12 | MonthView.qml |
| a-37 | Err-Empty | C | empty board card | folded columns + "+ колонка" tile | KanbanBoard.qml:1895-1913 |
| a-38 | Oth-Archive-People | C | sheet copy | different copy | PeopleDialog.qml |

Harness notes: base rig black bg/missed popups (fixed); 110 % scale (re-shot 100 %); fake sync data; storageDamagedAt read-only; Example data ≠ sheets.
Captures: a\out\{bold,quiet}-*.png; crops a\tools\.

Counts: BUG 25 (A0 B9 C16) · QT 0 · HARNESS 9 · DECIDED ~30. Rig b\tst_Cap.qml at 100 % scale, grabImage(win.contentItem).
QT: none (inline code pill in live editor and link underline colour/offset are doable via InlineCodeFrames-style overlays).
BUG B: b-01 WeekView.qml:1565/1587/1616 binding loop in day view (width=min(implicitWidth,…) on elided Text → TextMetrics); b-02 live-editor inline code pill (MdLivePreview.qml:147, MdHighlighter.cpp:933; reuse InlineCodeFrames); b-03 list date column not fixed width (TaskListView.qml:557); b-04 folded "Гото… 2" (KanbanBoard.qml:266); b-05 column menu "Новая колонка…" vs DG-022 (KanbanBoard.qml:1477); b-07 "Удалить серию…" not danger (WeekView.qml:242); b-08 bold primary not solid in TimeMachine/Standup/TrackerPushConfirm; b-09 recap bars not blue in bold (WeeklyRecapDialog.qml:291); b-11 appearance rows at rest (SettingsView.qml:1240).
BUG C: b-06 auto-archive back in column menu vs DG-025; b-10 recap 3rd button; b-12 extra settings rows at rest; b-13 settings hint copy; b-14 event editor tz row/link/sound; b-15 task menu header lacks provider; b-16 profile-move row; b-17 chevron on Длительность; b-18 time range clipped in narrow lane; b-19 estimate sum in header; b-20 card date elides mid-token; b-21 log source col 80px; b-22 conflict shows raw md; b-23 more binding loops KanbanBoard.qml:2393/2483/2551, EventCapture.qml:204; b-24 APP-314 undocumented (lens keys tooltip, keycaps); b-25 link underline colour/offset.
Harness: scrollbar always, schedule popup at (0,0), no tracker connect, etc.
Captures: b\out\{bold,quiet}-*.png, crops b\sheets\.

## Decisions (owner review 2026-10-15)

- **a-07 headings:** no new settings. Weight 600 (SemiBold is bundled; the 700 the highlighter asks for is not, so Qt fakes it), sizes as the sheet's ratio to body text (H1 2x, H2 1.2x) set as pixel sizes instead of coarse FontSizeAdjustment steps. Size follows Appearance → Scale, the face Appearance → Font.
- **a-23 sidebar:** not deliberate (unchanged since APP-258) — match the sheet: row pitch 32, first row at the sheet's y.
- **b-09 recap bars:** blue in bold is the bold style's colour (present in the grey theme too) — a bug, fix. Separately (proposal): a "Синяя" theme card in Appearance → Themes with the sheets' palette (#0c0e11 ground; the pre-APP-314 Tabless values from git history).
- **b-21 log source column:** width = widest of the five source labels in the current language (TextMetrics), no elide.

- **b-01, b-23 binding loops:** not a sheet gap, but on the fix list (owner, 2026-10-15): WeekView.qml:1565/1587/1616 (day view: width from its own implicitWidth on an elided Text → TextMetrics), KanbanBoard.qml:2393/2483/2551 (Dialog implicitWidth), EventCapture.qml:204 (SpanHighlighter spans).

## Not gaps (owner review 2026-10-15)

- **a-08** (осознанно): Rich-режим (#317) показывает строку под курсором исходником, поэтому «# » у заголовка при открытии — его поведение, а не сбой.
- **a-12** (осознанно): «Мои виды» скрыты на первом запуске и текст подсказки переписан в правке EYES-3 («одно слово на одну вещь»).
- **a-13** (осознанно): Тексты первого запуска переписаны в аудитах A5-13 и PERSONA-2; список трекеров длиннее, чем на листе.
- **a-14** (осознанно): Строка «Вне поля (Esc — выйти из него)» добавлена правкой A5-13: клавиши страницы говорят, где они работают.
- **a-15** (осознанно): Тусклая рамка в покое — правка PERSONA-2; размер заголовка — следствие масштаба APP-314.
- **a-16** (осознанно): Тот же переписанный первый запуск (A5-13, PERSONA-2), в тихом стиле.
- **a-21** (осознанно): Клавиши в рамке шрифтом текста — APP-314 (KeyHint.qml).
- **a-22** (осознанно): Время шрифтом интерфейса с табличными цифрами — A2-27 и APP-314.
- **a-25** (не проявится): Компактный вид не появляется только при масштабе 100 %. По умолчанию масштаб 110–125 % (APP-314), и окно 1280 px уходит в компактный вид.
- **a-32** (осознанно): «Ctrl M встреча · Tab документ» в подвале быстрого ввода — правка A2-14: Tab открывал документ без предупреждения.
- **a-37** (осознанно): Пустые колонки сворачиваются в полосу (APP-314), плитка «+ колонка» вернулась в APP-289.
- **b-05** (осознанно): «Новая колонка…» в меню колонки — APP-289 (колонка с доски).
- **b-06** (осознанно): «Автоархив…» в меню колонки — APP-295.
- **b-13** (осознанно): Подпись раздела «Внешний вид» сменили, когда «Твики» убрали; старая отсылает к тому, чего нет.
- **b-16** (осознанно): Перенос в профиль из меню задачи и панели выбора — A5-14 («Shift M везде»).
- **b-19** (осознанно): Сумма оценок рядом со счётчиком колонки — APP-246.
- **b-24** (осознанно): Клавиши вкладок перенесены в подсказку, рамки у клавиш — APP-314.
- **a-20** (не расхождение): Дубля нет: «+ Добавить человека» — единственный путь мышью в список «Кому написать» (окно «Люди» добавляет в справочник, это другое). Владелец: ок.
- **a-26** (осознанно): Значок «Сегодня» — день календаря вместо солнца, APP-296. Владелец: не менять.
- **a-36** (не расхождение): «+6 ещё · 3 встречи» в месяце — владелец: ок.
- **b-10** (не расхождение): Кнопка «← Неделя раньше» в сводке — владелец: оставить.
- **b-12** (не расхождение): Строка «„Сегодня“ показывает» в календаре — владелец: оставить.
- **b-14** (не расхождение): Часовой пояс, ссылка и напоминание в панели встречи — владелец: оставить.
