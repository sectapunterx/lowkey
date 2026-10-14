.pragma library

// A task description is markdown. Where it is shown as a one- or two-line
// preview — a board card, a timeline row — it should read as prose, not as
// "**Steps:** - [ ] build - [x] test" (audit TASKS-33 / TASKS-30).
//
// `max` (optional) cuts the result at that many characters with an ellipsis.
function plain(md, max) {
    let s = String(md || "")
        .replace(/```[\s\S]*?```/g, " ")
        .replace(/`([^`]*)`/g, "$1")
        .replace(/!\[[^\]]*\]\([^)]*\)/g, "")
        .replace(/\[([^\]]+)\]\([^)]*\)/g, "$1")
        .replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (m, a, b) => b || a)
        .replace(/^\s{0,3}#{1,6}\s+/gm, "")
        .replace(/^\s*>\s?/gm, "")
        .replace(/^\s*(?:[-*+]|\d+[.)])\s+\[[ xX]\]\s*/gm, "")
        .replace(/^\s*(?:[-*+]|\d+[.)])\s+/gm, "")
        .replace(/(\*\*|__)(.*?)\1/g, "$2")
        .replace(/(^|[^*])\*([^*\n]+)\*/g, "$1$2")
        .replace(/~~(.*?)~~/g, "$1")
        .replace(/\s+/g, " ")
        .trim();
    if (max && s.length > max) s = s.substring(0, max).trim() + "…";
    return s;
}

function _esc(s) {
    return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

// One line of a description as StyledText (APP-313): bold, italic, strike,
// `code` and links keep their look; the line's own marker (list, box,
// quote, heading) and images go. At most `max` visible characters, with an
// ellipsis. `colors`: { code, link, mono } (Theme tokens from the caller).
function inlineHtml(md, max, colors) {
    const c = colors || {};
    let s = String(md || "")
        .replace(/!\[[^\]]*\]\([^)]*\)/g, "")
        .replace(/^\s{0,3}#{1,6}\s+/, "")
        .replace(/^\s*>\s?/, "")
        .replace(/^\s*(?:[-*+]|\d+[.)])\s+\[[ xX]\]\s*/, "")
        .replace(/^\s*(?:[-*+]|\d+[.)])\s+/, "")
        .replace(/\s+/g, " ")
        .trim();
    let budget = max > 0 ? max : 1e9;
    let cut = false;
    // Plain words, cut to what is left of the budget.
    function text(t) {
        if (budget <= 0) { if (t.length > 0) cut = true; return ""; }
        if (t.length > budget) { t = t.substring(0, budget).replace(/\s+$/, ""); cut = true; }
        budget -= t.length;
        return _esc(t);
    }
    // The spans, outermost first; a span's inside is read again for the
    // ones nested in it (bold around a link).
    const rx = /`([^`]+)`|\[\[([^\]|]+)(?:\|([^\]]+))?\]\]|\[([^\]]+)\]\(([^)]*)\)|(\*\*|__)(\S(?:.*?\S)?)\6|(^|[^*\w])\*([^\s*](?:[^*]*?[^\s*])?)\*(?![*\w])|(^|[^_\w])_([^\s_](?:[^_]*?[^\s_])?)_(?![_\w])|~~(\S(?:.*?\S)?)~~/;
    function walk(t, depth) {
        let out = "";
        while (t.length > 0 && budget > 0) {
            const m = depth < 4 ? rx.exec(t) : null;
            if (!m) { out += text(t); break; }
            // The character an emphasis took from before its "*" / "_".
            const lead = m[8] !== undefined ? m[8] : (m[10] !== undefined ? m[10] : "");
            out += text(t.substring(0, m.index) + lead);
            if (budget <= 0) break;
            if (m[1] !== undefined)
                out += "<font color=\"" + (c.code || "") + "\"" + (c.mono ? " face=\"" + _esc(c.mono) + "\"" : "") + ">" + text(m[1]) + "</font>";
            else if (m[2] !== undefined)
                out += "<font color=\"" + (c.link || "") + "\">" + text(m[3] || m[2]) + "</font>";
            else if (m[4] !== undefined)
                out += "<font color=\"" + (c.link || "") + "\">" + walk(m[4], depth + 1) + "</font>";
            else if (m[7] !== undefined)
                out += "<b>" + walk(m[7], depth + 1) + "</b>";
            else if (m[9] !== undefined)
                out += "<i>" + walk(m[9], depth + 1) + "</i>";
            else if (m[11] !== undefined)
                out += "<i>" + walk(m[11], depth + 1) + "</i>";
            else if (m[12] !== undefined)
                out += "<s>" + walk(m[12], depth + 1) + "</s>";
            t = t.substring(m.index + m[0].length);
        }
        if (t.length > 0 && budget <= 0 && depth === 0) cut = true;
        return out;
    }
    const html = walk(s, 0);
    return cut ? html + "…" : html;
}
