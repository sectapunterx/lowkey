.pragma library

// A meeting's repeat rule in words, and the words of a one-line input made
// into a rule and a length (X-Dlg-Event, DG-120). Pure: the strings come in
// through `i18n` (the I18n singleton), so this file is testable alone.

var DAYS = ["MO", "TU", "WE", "TH", "FR", "SA", "SU"];

function parts(rule) {
    var out = {};
    var chunks = String(rule || "").toUpperCase().replace(/^RRULE:/, "").split(";");
    for (var i = 0; i < chunks.length; i++) {
        var eq = chunks[i].indexOf("=");
        if (eq > 0) out[chunks[i].slice(0, eq)] = chunks[i].slice(eq + 1);
    }
    return out;
}

// "каждую неделю по пятницам", "раз в 2 недели", "по будням", "не повторяется".
// `startDay` (a Date) names the weekday of a weekly rule without BYDAY.
function describe(rule, startDay, i18n) {
    if (!rule || String(rule).trim().length === 0) return i18n.t("event.repeat.none");
    var p = parts(rule);
    var n = p.INTERVAL ? parseInt(p.INTERVAL) : 1;
    var days = p.BYDAY ? p.BYDAY.split(",") : [];
    var out = "";
    if (p.FREQ === "DAILY") out = n > 1 ? i18n.count(n, "event.repeat.everyNDays") : i18n.t("event.repeat.daily");
    else if (p.FREQ === "WEEKLY") {
        if (days.length === 5 && days.join(",") === "MO,TU,WE,TH,FR" && n === 1) out = i18n.t("event.repeat.weekdays");
        else {
            if (days.length === 0 && startDay && startDay.getDay) days = [DAYS[(startDay.getDay() + 6) % 7]];
            var names = days.map(function (d) { return i18n.t("event.repeat.on." + d); }).join(", ");
            // "раз в 2 недели · пн, ср": the days of an interval said too,
            // once there is more than one (B7).
            var short = days.map(function (d) { return i18n.dayName((DAYS.indexOf(d) + 1) % 7).toLowerCase(); }).join(", ");
            out = n > 1 ? i18n.count(n, "event.repeat.everyNWeeks") + (p.BYDAY && days.length > 1 ? " · " + short : "")
                        : i18n.t("event.repeat.weeklyOn").arg(names);
        }
    } else if (p.FREQ === "MONTHLY") {
        out = n > 1 ? i18n.count(n, "event.repeat.everyNMonths") : i18n.t("event.repeat.monthly");
        // "every 2 months · on the 15", as the task's repeat chip says it
        // (F1-new-15): the day was dropped from the meeting's row.
        if (p.BYMONTHDAY && /^\d+$/.test(p.BYMONTHDAY)) out += " · " + i18n.t("repeat.monthDay").arg(p.BYMONTHDAY);
    } else if (p.FREQ === "YEARLY") out = i18n.t("event.repeat.yearly");
    else return String(rule);
    if (p.COUNT) out += " · " + i18n.count(parseInt(p.COUNT), "event.repeat.times");
    if (p.UNTIL) out += " · " + i18n.t("event.repeat.until").arg(p.UNTIL.slice(6, 8) + "." + p.UNTIL.slice(4, 6) + "." + p.UNTIL.slice(0, 4));
    return out;
}

// The rule a chrono recurrence ("every:week", "every:fri", "every:month:15")
// stands for.
function fromChrono(rec) {
    var r = String(rec || "");
    if (r.length === 0) return "";
    if (r === "every:day") return "FREQ=DAILY";
    if (r === "every:weekday") return "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR";
    if (r === "every:week") return "FREQ=WEEKLY";
    var m = r.match(/^every:month:(\d+)$/);
    if (m) return "FREQ=MONTHLY;BYMONTHDAY=" + m[1];
    var names = ["mon", "tue", "wed", "thu", "fri", "sat", "sun"];
    // "every:mon" and, since B7, "every:mon,wed": BYDAY lists the days.
    var d = r.match(/^every:((?:mon|tue|wed|thu|fri|sat|sun)(?:,(?:mon|tue|wed|thu|fri|sat|sun))*)$/);
    if (d) return "FREQ=WEEKLY;BYDAY=" + d[1].split(",").map(function (x) { return DAYS[names.indexOf(x)]; }).join(",");
    // "every:2:day", "every:3:week", "every:2:month[:15]": INTERVAL.
    var n = r.match(/^every:(\d+):(day|week|month)(?::(\d+))?$/);
    if (n) {
        var freq = n[2] === "day" ? "DAILY" : n[2] === "week" ? "WEEKLY" : "MONTHLY";
        var every = parseInt(n[1]);
        return "FREQ=" + freq + (every > 1 ? ";INTERVAL=" + every : "") + (n[3] ? ";BYMONTHDAY=" + n[3] : "");
    }
    return "";
}

// What the chrono parser does not read, taken out of the line first:
// "каждые 2 недели" / "every 2 weeks" (a rule) and "на 1 ч" / "for 30 min"
// (a length). { text, rule, minutes, ruleText, lengthText }.
function extract(raw) {
    var text = String(raw || "");
    var out = { text: text, rule: "", minutes: 0, ruleText: "", lengthText: "" };
    var every = /(?:^|\s)(каждые|каждых|every)\s+(\d+)\s+(недел[а-яё]*|weeks?|дн[а-яё]*|days?|месяц[а-яё]*|months?)(?=\s|$)/i;
    var m = text.match(every);
    if (m) {
        var n = parseInt(m[2]);
        var unit = m[3].toLowerCase();
        var freq = /^(недел|week)/.test(unit) ? "WEEKLY" : /^(дн|day)/.test(unit) ? "DAILY" : "MONTHLY";
        out.rule = "FREQ=" + freq + (n > 1 ? ";INTERVAL=" + n : "");
        out.ruleText = m[0].trim();
        text = text.replace(m[0], " ");
    }
    if (!out.rule) {
        var weekly = weeklyRule(text);
        if (weekly) {
            out.rule = weekly.rule;
            out.ruleText = weekly.text;
            text = text.replace(weekly.text, " ");
        }
    }
    // "на 30 мин", "for 1h", and a bare "30m" / "1,5ч" too (EYE-24: "30м"
    // was tinted as read and the meeting stayed an hour). "через 30 мин" /
    // "in 30m" is when, not how long: chrono's.
    var len = /(^|\s)(?:(на|for)\s+)?(\d+(?:[.,]\d+)?)\s*(ч|час[а-яё]*|h|hrs?|hours?|мин[а-яё]*|м|m|min|mins|minutes?)(?=\s|$)/i;
    var l = null;
    var rest = text;
    var offset = 0;
    while ((l = rest.match(len)) !== null) {
        var before = text.slice(0, offset + l.index).trim().split(/\s+/).pop().toLowerCase();
        if (l[2] || (before !== "через" && before !== "in" && before !== "after" && before !== "спустя")) break;
        offset += l.index + l[0].length;
        rest = text.slice(offset);
    }
    if (l) {
        l = { 0: l[0], 2: l[3], 3: l[4] };
        var v = parseFloat(l[2].replace(",", "."));
        var hours = /^(ч|час|h)/i.test(l[3]);
        out.minutes = Math.round(hours ? v * 60 : v);
        out.lengthText = l[0].trim();
        text = text.replace(l[0], " ");
    }
    out.text = text.replace(/\s+/g, " ").trim();
    return out;
}

// A weekly repeat chrono does not read (EYE-24): "every wednesday",
// "каждую среду", "по средам", "every mon and thu", "weekly", "еженедельно".
// { rule, text } or null; `text` is the words as typed.
var DAY_WORDS = [
    ["MO", "monday|mon|понедельник(?:ам|и)?|пн"],
    ["TU", "tuesday|tues|tue|вторник(?:ам|и)?|вт"],
    ["WE", "wednesday|wed|сред(?:у|ам|а|ы)|ср"],
    ["TH", "thursday|thurs|thur|thu|четверг(?:ам|и)?|чт"],
    ["FR", "friday|fri|пятниц(?:у|ам|а|ы)|пт"],
    ["SA", "saturday|sat|суббот(?:у|ам|а|ы)|сб"],
    ["SU", "sunday|sun|воскресень(?:е|ям|я)|вс"]
];

function dayOf(word) {
    var w = String(word || "").toLowerCase().replace(/s$/, "");
    for (var i = 0; i < DAY_WORDS.length; i++) {
        if (new RegExp("^(?:" + DAY_WORDS[i][1] + ")$", "i").test(w) ||
            new RegExp("^(?:" + DAY_WORDS[i][1] + ")$", "i").test(w + "s")) return DAY_WORDS[i][0];
    }
    return "";
}

function weeklyRule(raw) {
    var text = String(raw || "");
    var plain = /(^|\s)(weekly|еженедельно|every\s+week|each\s+week|каждую\s+неделю)(?=\s|$|[,.])/i.exec(text);
    if (plain) return { rule: "FREQ=WEEKLY", text: plain[2] };
    var day = "(?:" + DAY_WORDS.map(function (d) { return d[1]; }).join("|") + ")s?";
    var list = day + "(?:\\s*(?:,|\\sи|\\sand)\\s*" + day + ")*";
    var re = new RegExp("(^|\\s)((?:every|each|каждый|каждую|каждое|каждые|по)\\s+(" + list + "))(?=\\s|$|[.])", "i");
    var m = re.exec(text);
    if (!m) return null;
    var words = m[3].split(/\s*(?:,|\sи\s|\sand\s)\s*/i);
    var days = [];
    for (var i = 0; i < words.length; i++) {
        var d = dayOf(words[i].trim());
        if (!d) return null;
        if (days.indexOf(d) < 0) days.push(d);
    }
    // "по вт" is not how anyone says it: "по" takes the plural, full word.
    if (/^по\s/i.test(m[2]) && !/(ам|ям)$/i.test(m[3])) return null;
    days.sort(function (a, b) { return DAYS.indexOf(a) - DAYS.indexOf(b); });
    return { rule: "FREQ=WEEKLY;BYDAY=" + days.join(","), text: m[2] };
}

// The first day on or after `from` that a weekly BYDAY rule meets; `from`
// itself when the rule names no day.
function firstOn(rule, from) {
    var p = parts(rule);
    if (p.FREQ !== "WEEKLY" || !p.BYDAY || !from || !from.getDay) return from;
    var days = p.BYDAY.split(",");
    for (var i = 0; i < 7; i++) {
        var d = new Date(from.getFullYear(), from.getMonth(), from.getDate() + i);
        if (days.indexOf(DAYS[(d.getDay() + 6) % 7]) >= 0) return d;
    }
    return from;
}
