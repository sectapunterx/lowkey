// The first run (APP-271): the input line on Today makes a task with its
// date read from the line, and the example is a profile of its own that
// comes and goes without touching the person's tasks.
import QtQuick
import QtTest
import TodoCpp

TestCase {
    id: tc
    name: "FirstRun"
    when: windowShown
    visible: true
    width: 1000
    height: 700

    Item { id: host; anchors.fill: parent }

    function test_the_line_makes_a_task_with_its_date() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        const input = findChild(hero, "first-run-input");
        verify(input !== null);
        const before = AppController.tasks.rowCount();
        input.text = "first run demo tomorrow 12:00 p1";
        const id = hero.submit();
        verify(id.length > 0, "no task made");
        compare(AppController.tasks.rowCount(), before + 1);
        const t = AppController.taskById(id);
        compare(t.title, "first run demo");
        compare(t.priority, "P1");
        verify(AppController.welcomeSeen, "the first task did not end the first run");
        compare(input.text, "");
        AppController.deleteTask(id);
    }

    function test_three_keys_and_the_ways_in() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        verify(findChild(hero, "first-run-connect") !== null);
        verify(findChild(hero, "first-run-import") !== null);
        verify(findChild(hero, "first-run-example") !== null);
    }

    // Copy deck §7: the first line names the capture key the keymap really
    // has, not a fixed "Ctrl+Shift+Space".
    function test_first_line_names_the_capture_key() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        const sub = findChild(hero, "first-run-sub");
        verify(sub !== null);
        verify(sub.visible);
        const key = AppController.shortcutText("quick-capture");
        verify(key.length > 0, "quick capture has no key");
        verify(sub.text.indexOf(key) >= 0, "the line does not name " + key);
    }

    // DG-110 / DG-111: bold has three equal key cards, the third "?"; quiet
    // has no cards, only hint lines and text links.
    function test_bold_cards_and_quiet_links() {
        const saved = AppController.appSettingsJson;
        try {
            Style.apply("bold");
            const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
            const cards = [];
            (function walk(it) {
                if (!it) return;
                if (it.objectName === "first-run-key") cards.push(it);
                for (const k of it.children || []) walk(k);
            })(hero);
            compare(cards.length, 3);
            compare(cards[2].modelData.keys, "?");
            wait(50);
            compare(cards[0].height, cards[2].height, "the key cards are not one height");
            verify(cards[0].visible);
            Style.apply("quiet");
            verify(hero.quiet);
            verify(!cards[0].visible, "quiet still shows the key cards");
            // a-17 (Q-First): "? — все клавиши", the page key, not Ctrl /.
            const qkeys = findChild(hero, "first-run-q-keys").text;
            verify(qkeys.indexOf(">?</font>") >= 0, qkeys);
            verify(qkeys.indexOf("Ctrl /") < 0, qkeys);
            verify(!findChild(hero, "first-run-connect").visible);
        } finally {
            AppController.appSettingsJson = saved;
        }
    }

    function test_example_is_its_own_profile() {
        const own = AppController.activeProfileId;
        const ownTasks = AppController.tasks.rowCount();
        AppController.openExample();
        compare(AppController.activeProfileId, "tabless-example");
        verify(AppController.tasks.rowCount() > 0);
        const profiles = AppController.profiles.length;
        AppController.activeProfileId = own;
        AppController.openExample();
        compare(AppController.profiles.length, profiles, "a second example was made");
        AppController.removeExample();
        verify(!AppController.hasExample());
        compare(AppController.activeProfileId === "tabless-example", false);
        AppController.activeProfileId = own;
        compare(AppController.tasks.rowCount(), ownTasks, "the person's tasks changed");
    }

    // A5-9: the placeholder typed out as shown, lead word and all, makes the
    // task the example names, not "например: …".
    function test_the_placeholder_typed_literally_drops_its_lead_word() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        const input = findChild(hero, "first-run-input");
        const ph = I18n.t("first.placeholder");
        input.text = ph;
        const keepDay = AppController.selectedDate;
        const id = hero.submit();
        verify(id.length > 0);
        const t = AppController.taskById(id);
        const lead = ph.slice(0, ph.indexOf(":") + 1);
        verify(t.title.indexOf(lead) < 0, "the lead word stayed: " + t.title);
        verify(t.title.length > 0);
        AppController.deleteTask(id);
        AppController.selectedDate = keepDay;
    }

    // A5-10: the first task, made for another day, is shown on its day.
    function test_a_first_task_for_another_day_is_shown_on_its_day() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        const input = findChild(hero, "first-run-input");
        const keepDay = AppController.selectedDate;
        AppController.selectedDate = AppController.today;
        input.text = "first run later in 3 days";
        const id = hero.submit();
        verify(id.length > 0);
        const t = AppController.taskById(id);
        compare(Qt.formatDate(AppController.selectedDate, "yyyy-MM-dd"), Qt.formatDate(t.scheduledAt, "yyyy-MM-dd"));
        AppController.deleteTask(id);
        AppController.selectedDate = keepDay;
    }

    // A5-13: "d" and "?" are page keys; the line over the cards says so and
    // Esc takes the caret out of the field, so they no longer type "d?".
    function test_esc_leaves_the_field_for_the_page_keys() {
        const hero = createTemporaryQmlObject('import TodoCpp; FirstRunHero { width: 800 }', host);
        const input = findChild(hero, "first-run-input");
        hero.focusInput();
        verify(input.activeFocus);
        keyClick(Qt.Key_Escape);
        verify(!input.activeFocus, "Esc left the caret in the field");
        keyClick(Qt.Key_D);
        compare(input.text, "", "d went into the field");
        const where = findChild(hero, "first-run-keys-where");
        verify(where !== null);
        compare(where.text, I18n.t("first.keys.outside"));
    }
}
