#pragma once

#include <QString>

// LaTeX maths typeset as Qt rich text (APP-312): what a developer's notes hold
// - Greek letters, operators, relations and arrows, super- and subscripts,
// \frac, \sqrt, \mathbb/\mathbf/\mathrm/\text, accents, the usual functions -
// set with Unicode and <sup>/<sub>, so it draws in a TextEdit, in the rich
// editor's overlay and in the drawn note, offline and with no web engine.
// What it does not know (an environment, an unknown command) is not guessed
// at: the formula is shown as its source, marked.
namespace heap::md {

struct MathRender {
  QString html;    // rich text (a fragment, no <html>)
  QString plain;   // the same without markup, for measuring and copying
  bool ok = true;  // false: `html` is the marked source
};

// `tex` is what is between the dollars; `display` is $$ … $$ (centred, a
// step larger).
MathRender renderMath(const QString& tex, bool display = false);

}  // namespace heap::md
