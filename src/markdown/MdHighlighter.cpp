#include "markdown/MdHighlighter.h"

#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>

namespace heap::md {

namespace {

QColor colorAt(const QVariantMap& palette, const char* key, const QColor& fallback) {
  const QVariant value = palette.value(QLatin1String(key));
  if(!value.isValid()) {
    return fallback;
  }
  const auto color = value.value<QColor>();
  return color.isValid() ? color : fallback;
}

// A fence opens with three or more backticks or tildes; the info string after
// it names the language.
const QRegularExpression& fenceRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s{0,3}(`{3,}|~{3,})\\s*([A-Za-z0-9_+#.-]*)"));
  return rx;
}

const QRegularExpression& atxHeadingRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s{0,3}(#{1,6})(\\s+.*)?$"));
  return rx;
}

// A line of only = or - under text makes that text a heading. A per-line
// highlighter cannot see the text above it, but it can at least mark the
// underline, which is what the old one missed entirely.
const QRegularExpression& setextRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s{0,3}(={2,}|-{2,})\\s*$"));
  return rx;
}

const QRegularExpression& thematicBreakRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s{0,3}(?:-\\s*){3,}$|^\\s{0,3}(?:\\*\\s*){3,}$|^\\s{0,3}(?:_\\s*){3,}$"));
  return rx;
}

const QRegularExpression& listMarkerRx() {
  static const QRegularExpression rx(QStringLiteral("^(\\s*)([-*+]|\\d+[.)])(\\s+)(\\[([ xX])\\]\\s+)?"));
  return rx;
}

const QRegularExpression& quoteRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s*(?:>\\s?)+"));
  return rx;
}

const QRegularExpression& tableRowRx() {
  static const QRegularExpression rx(QStringLiteral("^\\s*\\|.*\\|\\s*$"));
  return rx;
}

}  // namespace

MdHighlighter::MdHighlighter(QObject* parent) : QSyntaxHighlighter(parent) {
  rebuildFormats();
}

void MdHighlighter::setTarget(QQuickTextDocument* target) {
  if(m_target == target) {
    return;
  }
  m_target = target;
  setDocument(target != nullptr ? target->textDocument() : nullptr);
  emit targetChanged();
}

void MdHighlighter::setPalette(const QVariantMap& palette) {
  if(m_palette == palette) {
    return;
  }
  m_palette = palette;
  rebuildFormats();
  rehighlight();
  emit paletteChanged();
}

void MdHighlighter::setHeadingScale(qreal scale) {
  if(qFuzzyCompare(m_headingScale, scale)) {
    return;
  }
  m_headingScale = scale;
  rebuildFormats();
  rehighlight();
  emit headingScaleChanged();
}

void MdHighlighter::setLivePreview(bool on) {
  if(m_livePreview == on) {
    return;
  }
  m_livePreview = on;
  rehighlight();
  emit livePreviewChanged();
}

void MdHighlighter::setRevealStart(int position) {
  if(m_revealStart == position) {
    return;
  }
  const int oldStart = m_revealStart;
  m_revealStart = position;
  revealMoved(oldStart, m_revealEnd);
  emit revealChanged();
}

void MdHighlighter::setRevealEnd(int position) {
  if(m_revealEnd == position) {
    return;
  }
  const int oldEnd = m_revealEnd;
  m_revealEnd = position;
  revealMoved(m_revealStart, oldEnd);
  emit revealChanged();
}

QPair<QTextBlock, QTextBlock> MdHighlighter::revealUnit(const QTextBlock& block) const {
  // A table is drawn as one grid and a fence as one box: the caret on any of
  // their lines shows all of their source.
  const auto isTable = [](const QTextBlock& b) {
    return b.isValid() && tableRowRx().match(b.text()).hasMatch();
  };
  if(isTable(block)) {
    QTextBlock first = block;
    QTextBlock last = block;
    while(isTable(first.previous())) {
      first = first.previous();
    }
    while(isTable(last.next())) {
      last = last.next();
    }
    return {first, last};
  }
  const auto inFence = [](const QTextBlock& b) {
    return b.isValid() && b.userState() == InFence;
  };
  const bool opens = fenceRx().match(block.text()).hasMatch() && !inFence(block.previous());
  if(inFence(block) || inFence(block.previous()) || opens) {
    QTextBlock first = block;
    while(inFence(first.previous())) {
      first = first.previous();
    }
    QTextBlock last = block;
    while(inFence(last) && last.next().isValid()) {
      last = last.next();
    }
    return {first, last};
  }
  return {block, block};
}

bool MdHighlighter::revealed(const QTextBlock& block) const {
  if(m_revealStart < 0) {
    return false;
  }
  const int from = std::min(m_revealStart, std::max(m_revealEnd, m_revealStart));
  const int to = std::max(m_revealStart, m_revealEnd);
  const auto [first, last] = revealUnit(block);
  const int start = first.position();
  const int end = last.position() + last.length() - 1;  // the line break belongs to it
  return from <= end && to >= start;
}

bool MdHighlighter::hides() const {
  return m_livePreview && !revealed(currentBlock());
}

void MdHighlighter::ghost(int start, int length) {
  if(length <= 0) {
    return;
  }
  QTextCharFormat f = format(start);
  f.merge(m_ghost);
  setFormat(start, length, f);
}

void MdHighlighter::revealMoved(int oldStart, int oldEnd) {
  if(!m_livePreview || document() == nullptr) {
    return;
  }
  // Only the lines entering or leaving the range change: a full pass on
  // every caret move would be felt in a long note.
  const auto range = [](int a, int b) {
    return std::make_pair(std::min(a, std::max(a, b)), std::max(a, b));
  };
  const auto [oa, ob] = range(oldStart, oldEnd < 0 ? oldStart : oldEnd);
  const auto [na, nb] = range(m_revealStart, m_revealEnd < 0 ? m_revealStart : m_revealEnd);
  QTextDocument* doc = document();
  const auto refresh = [&](int a, int b) {
    if(a < 0) {
      return;
    }
    QTextBlock block = revealUnit(doc->findBlock(a)).first;
    const QTextBlock last = revealUnit(doc->findBlock(b)).second;
    while(block.isValid()) {
      rehighlightBlock(block);
      if(block == last) {
        break;
      }
      block = block.next();
    }
  };
  refresh(oa, ob);
  refresh(na, nb);
}

QList<QPair<int, int>> MdHighlighter::hiddenMarkup(const QString& line) {
  QList<QPair<int, int>> out;
  const auto add = [&out](qsizetype start, qsizetype length) {
    if(length > 0) {
      out.append({static_cast<int>(start), static_cast<int>(length)});
    }
  };
  // "## " of a heading.
  static const QRegularExpression headingRx(QStringLiteral("^(\\s{0,3}#{1,6}\\s+)"));
  if(const auto m = headingRx.match(line); m.hasMatch()) {
    add(m.capturedStart(1), m.capturedLength(1));
  }
  // Inline code first: what is inside backticks is not markup.
  static const QRegularExpression codeRx(QStringLiteral("(`)([^`\\n]+)(`)"));
  QList<QPair<int, int>> code;
  for(auto it = codeRx.globalMatch(line); it.hasNext();) {
    const auto m = it.next();
    add(m.capturedStart(1), 1);
    add(m.capturedStart(3), 1);
    code.append({static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedEnd())});
  }
  const auto inCode = [&code](qsizetype at) {
    for(const auto& [a, b] : code) {
      if(at >= a && at < b) {
        return true;
      }
    }
    return false;
  };

  struct Pair {
    QRegularExpression rx;
    int open;
    int close;
  };

  // The delimiters of a span: the opening and closing groups are hidden, the
  // text between keeps its style.
  static const Pair pairs[] = {
      {QRegularExpression(QStringLiteral("(\\*\\*|__)(?=\\S)(.+?)(?<=\\S)\\1")), 1, 1},
      {QRegularExpression(QStringLiteral("(?<![*\\w])(\\*)(?!\\s)([^*\\n]+?)(\\*)(?!\\w)")), 1, 3},
      {QRegularExpression(QStringLiteral("(?<![_\\w])(_)(?!\\s)([^_\\n]+?)(_)(?!\\w)")), 1, 3},
      {QRegularExpression(QStringLiteral("(~~)([^~\\n]+)(~~)")), 1, 3},
      {QRegularExpression(QStringLiteral("(==)([^=\\n]+)(==)")), 1, 3},
  };
  for(const Pair& p : pairs) {
    for(auto it = p.rx.globalMatch(line); it.hasNext();) {
      const auto m = it.next();
      if(inCode(m.capturedStart())) {
        continue;
      }
      add(m.capturedStart(p.open), m.capturedLength(p.open));
      // Group 1 doubles as the closing delimiter for ** and __.
      add(m.capturedEnd() - m.capturedLength(p.close), m.capturedLength(p.close));
    }
  }
  // [text](url): the brackets and the address go, the text stays as a link.
  static const QRegularExpression linkRx(QStringLiteral("(?<!!)(\\[)([^\\]\\n]+)(\\]\\([^)\\n]*\\))"));
  for(auto it = linkRx.globalMatch(line); it.hasNext();) {
    const auto m = it.next();
    if(inCode(m.capturedStart())) {
      continue;
    }
    add(m.capturedStart(1), 1);
    add(m.capturedStart(3), m.capturedLength(3));
  }
  // [[target]] and [[target|alias]]: what is read is the target, or the alias.
  static const QRegularExpression wikiRx(QStringLiteral("(\\[\\[)([^\\]|\\n]+\\|)?([^\\]\\n]+)(\\]\\])"));
  for(auto it = wikiRx.globalMatch(line); it.hasNext();) {
    const auto m = it.next();
    if(inCode(m.capturedStart())) {
      continue;
    }
    add(m.capturedStart(1), 2);
    if(m.capturedLength(2) > 0) {
      add(m.capturedStart(2), m.capturedLength(2));
    }
    add(m.capturedStart(4), 2);
  }
  return out;
}

QVariantList MdHighlighter::decorations(const QString& text) {
  QVariantList out;
  const QStringList lines = text.split(QLatin1Char('\n'));
  QVector<int> starts(lines.size());
  int at = 0;
  for(int i = 0; i < lines.size(); ++i) {
    starts[i] = at;
    at += static_cast<int>(lines.at(i).size()) + 1;
  }
  const auto lineEnd = [&](int i) {
    return starts.at(i) + static_cast<int>(lines.at(i).size());
  };
  static const QRegularExpression separatorRx(QStringLiteral("^\\s*\\|?\\s*:?-+:?\\s*(\\|\\s*:?-+:?\\s*)*\\|?\\s*$"));
  const auto cellsOf = [](const QString& line) {
    QString l = line.trimmed();
    if(l.startsWith(QLatin1Char('|'))) {
      l.remove(0, 1);
    }
    if(l.endsWith(QLatin1Char('|'))) {
      l.chop(1);
    }
    QStringList cells;
    QString cur;
    for(qsizetype k = 0; k < l.size(); ++k) {
      if(l.at(k) == QLatin1Char('\\') && k + 1 < l.size() && l.at(k + 1) == QLatin1Char('|')) {
        cur += QLatin1Char('|');
        ++k;
      } else if(l.at(k) == QLatin1Char('|')) {
        cells << cur.trimmed();
        cur.clear();
      } else {
        cur += l.at(k);
      }
    }
    cells << cur.trimmed();
    return cells;
  };

  int i = 0;
  while(i < lines.size()) {
    const QString& line = lines.at(i);
    // Fenced code: one box from the opening fence to the closing one.
    if(const auto f = fenceRx().match(line); f.hasMatch()) {
      int j = i + 1;
      while(j < lines.size() && !(fenceRx().match(lines.at(j)).hasMatch() && fenceRx().match(lines.at(j)).captured(2).isEmpty())) {
        ++j;
      }
      const int last = std::min(j, static_cast<int>(lines.size()) - 1);
      out.append(QVariantMap{{"kind", "code"}, {"start", starts.at(i)}, {"end", lineEnd(last)}, {"language", f.captured(2)}});
      i = last + 1;
      continue;
    }
    // A table: a header row, the |---| line, then rows.
    if(tableRowRx().match(line).hasMatch() && i + 1 < lines.size() && separatorRx.match(lines.at(i + 1)).hasMatch()) {
      int j = i;
      while(j + 1 < lines.size() && tableRowRx().match(lines.at(j + 1)).hasMatch()) {
        ++j;
      }
      QVariantList rows;
      QVariantList align;
      for(const QString& cell : cellsOf(lines.at(i + 1))) {
        const bool l = cell.startsWith(QLatin1Char(':'));
        const bool r = cell.endsWith(QLatin1Char(':'));
        align << QString(l && r ? "c" : r ? "r" : l ? "l" : "");
      }
      for(int k = i; k <= j; ++k) {
        if(k == i + 1) {
          rows.append(QVariantMap{{"start", starts.at(k)}, {"separator", true}, {"cells", QStringList()}});
          continue;
        }
        rows.append(QVariantMap{{"start", starts.at(k)}, {"header", k == i}, {"separator", false}, {"cells", cellsOf(lines.at(k))}});
      }
      out.append(QVariantMap{{"kind", "table"}, {"start", starts.at(i)}, {"end", lineEnd(j)}, {"rows", rows}, {"align", align}});
      i = j + 1;
      continue;
    }
    // "---" right under a plain line of text underlines it as a heading; under
    // a blank line, a quote, a list item or a heading it is a rule.
    const auto underText = [&]() {
      if(i == 0 || !line.trimmed().startsWith(QLatin1Char('-'))) {
        return false;
      }
      const QString& above = lines.at(i - 1);
      return !above.trimmed().isEmpty() && !quoteRx().match(above).hasMatch() && !listMarkerRx().match(above).hasMatch() &&
             !atxHeadingRx().match(above).hasMatch();
    };
    if(thematicBreakRx().match(line).hasMatch() && !underText()) {
      out.append(QVariantMap{{"kind", "rule"}, {"start", starts.at(i)}, {"end", lineEnd(i)}});
      ++i;
      continue;
    }
    if(const auto q = quoteRx().match(line); q.hasMatch() && q.capturedLength() > 0) {
      out.append(QVariantMap{{"kind", "quote"},
                             {"start", starts.at(i)},
                             {"end", lineEnd(i)},
                             {"depth", static_cast<int>(q.captured(0).count(QLatin1Char('>')))}});
    } else if(const auto m = listMarkerRx().match(line); m.hasMatch() && !m.captured(2).at(0).isDigit()) {
      if(!m.captured(4).isEmpty()) {
        out.append(QVariantMap{{"kind", "task"},
                               {"start", starts.at(i)},
                               {"end", lineEnd(i)},
                               {"at", starts.at(i) + static_cast<int>(m.capturedStart(2))},
                               {"box", starts.at(i) + static_cast<int>(m.capturedStart(4))},
                               {"checked", m.captured(5).toLower() == QStringLiteral("x")}});
      } else {
        out.append(QVariantMap{{"kind", "bullet"},
                               {"start", starts.at(i)},
                               {"end", lineEnd(i)},
                               {"at", starts.at(i) + static_cast<int>(m.capturedStart(2))},
                               {"depth", static_cast<int>(m.captured(1).size()) / 2}});
      }
    }
    ++i;
  }
  return out;
}

void MdHighlighter::hideMarkup(const QString& text) {
  for(const auto& [start, length] : hiddenMarkup(text)) {
    // Kept in the text, drawn as nothing: a hairline glyph in no colour.
    setFormat(start, length, m_hidden);
  }
}

void MdHighlighter::rebuildFormats() {
  const QColor text = colorAt(m_palette, "text", QColor(0xE6, 0xE6, 0xE6));
  const QColor dim = colorAt(m_palette, "dim", QColor(0x8A, 0x8E, 0x98));
  const QColor accent = colorAt(m_palette, "accent", QColor(0x5A, 0xC8, 0xD8));
  const QColor codeColor = colorAt(m_palette, "code", text);
  const QColor codeBg = colorAt(m_palette, "codeBg", QColor());

  // Headings: the level shows in the weight here, and in the size applied per
  // line in highlightBlock. Sizing relative to the document's own font keeps
  // the editor's font setting as the baseline instead of hard-coding points.
  for(int level = 0; level < 6; ++level) {
    m_heading[level].setFontWeight(level <= 2 ? QFont::Bold : QFont::DemiBold);
    m_heading[level].setForeground(text);
  }

  m_code.setForeground(codeColor);
  m_code.setFontFixedPitch(true);
  if(codeBg.isValid()) {
    m_code.setBackground(codeBg);
  }
  m_codeFence = m_code;
  m_codeFence.setForeground(dim);

  m_quote.setForeground(dim);
  m_quote.setFontItalic(true);
  m_marker.setForeground(accent);
  m_marker.setFontWeight(QFont::DemiBold);
  m_link.setForeground(accent);
  m_link.setFontUnderline(true);
  m_emphasis.setFontItalic(true);
  m_strong.setFontWeight(QFont::Bold);
  m_strike.setFontStrikeOut(true);
  m_strike.setForeground(dim);
  m_taskDone.setFontStrikeOut(true);
  m_taskDone.setForeground(dim);
  m_mention.setForeground(colorAt(m_palette, "mention", accent));
  m_ticket.setForeground(colorAt(m_palette, "ticket", accent));
  m_ticket.setFontWeight(QFont::DemiBold);
  m_tag.setForeground(colorAt(m_palette, "tag", accent));
  m_wiki.setForeground(accent);
  m_math.setForeground(colorAt(m_palette, "math", accent));
  m_math.setFontFixedPitch(true);
  m_highlight.setBackground(colorAt(m_palette, "highlightBg", QColor(0, 0, 0, 0)));
  m_frontmatter.setForeground(dim);
  m_frontmatter.setFontFixedPitch(true);
  m_rule.setForeground(dim);
  m_table.setForeground(dim);

  // Fenced code uses the same colours the docs viewer already speaks, so a
  // snippet looks the same wherever it is read.
  m_codeKeyword.setForeground(colorAt(m_palette, "keyword", accent));
  m_codeKeyword.setFontWeight(QFont::DemiBold);
  m_codeString.setForeground(colorAt(m_palette, "string", QColor(0x9C, 0xC4, 0x8B)));
  m_codeComment.setForeground(dim);
  m_codeComment.setFontItalic(true);
  m_codeNumber.setForeground(colorAt(m_palette, "number", QColor(0xD8, 0xC2, 0x77)));

  // Collapsed: a one-pixel, 1%-wide glyph in no colour. A fresh format, not
  // a merge: a heading's size adjustment kept "## " a heading's width.
  m_hidden = QTextCharFormat();
  m_hidden.setForeground(QColor(0, 0, 0, 0));
  m_hidden.setProperty(QTextFormat::FontPixelSize, 1);
  m_hidden.setFontStretch(1);
  m_ghost = QTextCharFormat();
  m_ghost.setForeground(QColor(0, 0, 0, 0));
}

namespace {

// Carries the open fence's language down every line inside it. It cannot be a
// member of the highlighter: highlightBlock() runs for one changed line at a
// time, so a member holds whichever fence was opened last during the most
// recent full pass, not the one this line is actually inside.
class FenceLanguage : public QTextBlockUserData {
 public:
  explicit FenceLanguage(QString lang) : language(std::move(lang)) {
  }

  QString language;
};

QString fenceLanguageOf(const QTextBlock& block) {
  if(const auto* data = dynamic_cast<const FenceLanguage*>(block.userData())) {
    return data->language;
  }
  return {};
}

// Which line-comment marker a fence's language actually uses. An unknown or
// absent language keeps the permissive union of all three, which is what every
// fence used to get.
const QRegularExpression& commentRxFor(const QString& language) {
  static const QRegularExpression anyRx(QStringLiteral("(//|#|--)[^\\n]*$"));
  static const QRegularExpression hashRx(QStringLiteral("#[^\\n]*$"));
  static const QRegularExpression slashRx(QStringLiteral("//[^\\n]*$"));
  static const QRegularExpression slashHashRx(QStringLiteral("(//|#)[^\\n]*$"));
  static const QRegularExpression dashRx(QStringLiteral("--[^\\n]*$"));
  static const QRegularExpression neverRx(QStringLiteral("(?!)"));

  static const QHash<QString, const QRegularExpression*> kByLanguage = {
      {QStringLiteral("python"), &hashRx},
      {QStringLiteral("py"), &hashRx},
      {QStringLiteral("ruby"), &hashRx},
      {QStringLiteral("rb"), &hashRx},
      {QStringLiteral("sh"), &hashRx},
      {QStringLiteral("bash"), &hashRx},
      {QStringLiteral("zsh"), &hashRx},
      {QStringLiteral("shell"), &hashRx},
      {QStringLiteral("yaml"), &hashRx},
      {QStringLiteral("yml"), &hashRx},
      {QStringLiteral("toml"), &hashRx},
      {QStringLiteral("ini"), &hashRx},
      {QStringLiteral("conf"), &hashRx},
      {QStringLiteral("cmake"), &hashRx},
      {QStringLiteral("make"), &hashRx},
      {QStringLiteral("makefile"), &hashRx},
      {QStringLiteral("dockerfile"), &hashRx},
      {QStringLiteral("perl"), &hashRx},
      {QStringLiteral("r"), &hashRx},
      {QStringLiteral("nix"), &hashRx},

      {QStringLiteral("c"), &slashRx},
      {QStringLiteral("cpp"), &slashRx},
      {QStringLiteral("c++"), &slashRx},
      {QStringLiteral("cxx"), &slashRx},
      {QStringLiteral("h"), &slashRx},
      {QStringLiteral("hpp"), &slashRx},
      {QStringLiteral("java"), &slashRx},
      {QStringLiteral("js"), &slashRx},
      {QStringLiteral("javascript"), &slashRx},
      {QStringLiteral("ts"), &slashRx},
      {QStringLiteral("typescript"), &slashRx},
      {QStringLiteral("jsx"), &slashRx},
      {QStringLiteral("tsx"), &slashRx},
      {QStringLiteral("go"), &slashRx},
      {QStringLiteral("rust"), &slashRx},
      {QStringLiteral("rs"), &slashRx},
      {QStringLiteral("kotlin"), &slashRx},
      {QStringLiteral("kt"), &slashRx},
      {QStringLiteral("swift"), &slashRx},
      {QStringLiteral("cs"), &slashRx},
      {QStringLiteral("csharp"), &slashRx},
      {QStringLiteral("scala"), &slashRx},
      {QStringLiteral("dart"), &slashRx},
      {QStringLiteral("qml"), &slashRx},
      {QStringLiteral("glsl"), &slashRx},
      {QStringLiteral("groovy"), &slashRx},

      {QStringLiteral("php"), &slashHashRx},

      {QStringLiteral("sql"), &dashRx},
      {QStringLiteral("lua"), &dashRx},
      {QStringLiteral("haskell"), &dashRx},
      {QStringLiteral("hs"), &dashRx},
      {QStringLiteral("ada"), &dashRx},

      // No line comment at all — greying out a `#` in JSON is pure noise.
      {QStringLiteral("json"), &neverRx},
      {QStringLiteral("csv"), &neverRx},
      {QStringLiteral("html"), &neverRx},
      {QStringLiteral("xml"), &neverRx},
      {QStringLiteral("css"), &neverRx},
      {QStringLiteral("text"), &neverRx},
      {QStringLiteral("plain"), &neverRx},
      {QStringLiteral("txt"), &neverRx},
      {QStringLiteral("diff"), &neverRx},
  };

  const auto it = kByLanguage.constFind(language.trimmed().toLower());
  return it == kByLanguage.constEnd() ? anyRx : **it;
}

}  // namespace

void MdHighlighter::highlightCodeLine(const QString& text, const QString& language) {
  setFormat(0, static_cast<int>(text.size()), m_code);
  if(text.isEmpty()) {
    return;
  }

  // Deliberately coarse: strings, comments and numbers, plus a handful of
  // words every language in a note is likely to use. A note is not an IDE,
  // and a per-keystroke highlighter cannot afford a real lexer per language.
  static const QRegularExpression stringRx(QStringLiteral("\"(?:[^\"\\\\\\n]|\\\\.)*\"|'(?:[^'\\\\\\n]|\\\\.)*'|`(?:[^`\\\\\\n]|\\\\.)*`"));
  static const QRegularExpression numberRx(QStringLiteral("\\b\\d+(?:\\.\\d+)?\\b"));
  // Comment syntax is the one thing the union gets visibly wrong: with a
  // single (//|#|--) rule, a `#include` line greys out in a ```cpp fence and
  // the `//` of a URL does the same in ```python. The fence's language narrows
  // it; an unknown or absent language keeps the permissive union.
  const QRegularExpression& commentRx = commentRxFor(language);
  static const QRegularExpression keywordRx(
      QStringLiteral("\\b(?:if|else|for|while|return|class|struct|def|function|fn|let|const|var|import|from|"
                     "include|namespace|public|private|protected|static|void|int|bool|true|false|null|nullptr|"
                     "None|True|False|async|await|try|catch|except|finally|throw|raise|new|delete|this|self)\\b"));

  auto apply = [&](const QRegularExpression& rx, const QTextCharFormat& format) {
    auto it = rx.globalMatch(text);
    while(it.hasNext()) {
      const auto match = it.next();
      setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), format);
    }
  };

  apply(keywordRx, m_codeKeyword);
  apply(numberRx, m_codeNumber);
  // Strings and comments come last so a keyword inside one is not recoloured,
  // and between the two, whichever starts first wins: the # in
  // `echo "#1"` is inside a string, not the start of a comment.
  qsizetype pos = 0;
  while(pos < text.size()) {
    const auto s = stringRx.match(text, pos);
    const auto c = commentRx.match(text, pos);
    const bool haveS = s.hasMatch() && s.capturedLength() > 0;
    const bool haveC = c.hasMatch() && c.capturedLength() > 0;
    if(!haveS && !haveC) {
      break;
    }
    if(haveC && (!haveS || c.capturedStart() < s.capturedStart())) {
      setFormat(static_cast<int>(c.capturedStart()), static_cast<int>(c.capturedLength()), m_codeComment);
      break;  // a line comment runs to the end
    }
    setFormat(static_cast<int>(s.capturedStart()), static_cast<int>(s.capturedLength()), m_codeString);
    pos = s.capturedEnd();
  }
}

void MdHighlighter::highlightInline(const QString& text) {
  struct Rule {
    const QRegularExpression* pattern;
    const QTextCharFormat* format;
    int group;
  };

  static const QRegularExpression codeRx(QStringLiteral("`[^`\\n]+`"));
  static const QRegularExpression strongRx(QStringLiteral("\\*\\*[^*\\n]+\\*\\*|__[^_\\n]+__"));
  static const QRegularExpression emphasisRx(QStringLiteral("(?<![*\\w])\\*(?!\\s)[^*\\n]+\\*(?!\\w)|(?<![_\\w])_(?!\\s)[^_\\n]+_(?!\\w)"));
  static const QRegularExpression strikeRx(QStringLiteral("~~[^~\\n]+~~"));
  static const QRegularExpression highlightRx(QStringLiteral("==[^=\\n]+=="));
  static const QRegularExpression linkRx(QStringLiteral("!?\\[[^\\]\\n]*\\]\\([^)\\n]*\\)"));
  static const QRegularExpression wikiRx(QStringLiteral("\\[\\[[^\\]\\n]+\\]\\]"));
  static const QRegularExpression urlRx(QStringLiteral("\\bhttps?://[^\\s)\\]]+"));
  static const QRegularExpression mathRx(QStringLiteral("(?<![\\\\$])\\$(?!\\s)[^$\\n]+\\$(?!\\$)"));
  // Names and tags in any script: "@Олег_Т" is as much a mention as "@oleg".
  static const QRegularExpression mentionRx(QStringLiteral("(?<![\\p{L}\\p{N}_])@[\\p{L}\\p{N}_.-]+"));
  static const QRegularExpression ticketRx(QStringLiteral("(?<![A-Za-z0-9_])#[A-Z][A-Z0-9]*-\\d+"));
  static const QRegularExpression tagRx(QStringLiteral("(?<![\\p{L}\\p{N}_])#[\\p{L}][\\p{L}\\p{N}_/-]*"));
  static const QRegularExpression footnoteRx(QStringLiteral("\\[\\^[^\\]\\s]+\\]"));

  const Rule rules[] = {
      {&strongRx, &m_strong, 0},
      {&emphasisRx, &m_emphasis, 0},
      {&strikeRx, &m_strike, 0},
      {&highlightRx, &m_highlight, 0},
      {&wikiRx, &m_wiki, 0},
      {&linkRx, &m_link, 0},
      {&urlRx, &m_link, 0},
      {&mathRx, &m_math, 0},
      {&ticketRx, &m_ticket, 0},
      {&tagRx, &m_tag, 0},
      {&mentionRx, &m_mention, 0},
      {&footnoteRx, &m_wiki, 0},
  };

  for(const Rule& rule : rules) {
    auto it = rule.pattern->globalMatch(text);
    while(it.hasNext()) {
      const auto match = it.next();
      setFormat(static_cast<int>(match.capturedStart(rule.group)), static_cast<int>(match.capturedLength(rule.group)), *rule.format);
    }
  }

  // Inline code last: what is inside backticks is not markdown, so nothing
  // else should have coloured it. "#tag" in a snippet is a comment.
  auto it = codeRx.globalMatch(text);
  while(it.hasNext()) {
    const auto match = it.next();
    setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), m_code);
  }
}

void MdHighlighter::highlightBlock(const QString& text) {
  const int previous = previousBlockState();
  const int length = static_cast<int>(text.size());

  // ── Multi-line state ────────────────────────────────────────────
  // A line on its own cannot tell code from prose, which is what the old
  // per-line highlighter got wrong.
  if(previous == InFrontmatter) {
    setFormat(0, length, m_frontmatter);
    setCurrentBlockState(text.trimmed() == QStringLiteral("---") || text.trimmed() == QStringLiteral("...") ? Normal : InFrontmatter);
    return;
  }
  if(currentBlock().blockNumber() == 0 && text.trimmed() == QStringLiteral("---")) {
    setFormat(0, length, m_frontmatter);
    setCurrentBlockState(InFrontmatter);
    return;
  }

  if(previous == InFence) {
    // The language was captured on the opening fence and relayed line by line;
    // reading it from the previous block is what lets a re-highlight of a
    // single line in the middle of a fence still know what it is inside.
    const QString language = fenceLanguageOf(currentBlock().previous());
    const auto fence = fenceRx().match(text);
    if(fence.hasMatch() && fence.captured(2).isEmpty()) {
      setFormat(0, length, m_codeFence);
      setCurrentBlockUserData(nullptr);
      setCurrentBlockState(Normal);
      if(hides()) {
        ghost(0, length);
      }
      return;
    }
    setCurrentBlockUserData(new FenceLanguage(language));
    highlightCodeLine(text, language);
    setCurrentBlockState(InFence);
    return;
  }

  if(previous == InMathBlock) {
    setFormat(0, length, m_math);
    setCurrentBlockState(text.trimmed() == QStringLiteral("$$") ? Normal : InMathBlock);
    return;
  }

  const auto fence = fenceRx().match(text);
  if(fence.hasMatch()) {
    setFormat(0, length, m_codeFence);
    // The opening fence carries the language for every line that follows.
    setCurrentBlockUserData(new FenceLanguage(fence.captured(2)));
    setCurrentBlockState(InFence);
    if(hides()) {
      ghost(0, length);
    }
    return;
  }
  if(text.trimmed() == QStringLiteral("$$")) {
    setFormat(0, length, m_math);
    setCurrentBlockUserData(nullptr);
    setCurrentBlockState(InMathBlock);
    return;
  }

  // A line that is no longer inside a fence must not keep the language it had
  // when it was, or deleting a fence would leave the stale value behind for
  // whatever re-highlight reads this block next.
  setCurrentBlockUserData(nullptr);
  setCurrentBlockState(Normal);

  // ── Line-level structure ────────────────────────────────────────
  const auto heading = atxHeadingRx().match(text);
  if(heading.hasMatch()) {
    const int level = static_cast<int>(heading.captured(1).size());
    QTextCharFormat format = m_heading[std::clamp(level, 1, 6) - 1];
    // Qt stores a heading's relative size as a multiplier on the document's
    // default font; setting it here keeps the editor's own font setting as the
    // baseline instead of hard-coding point sizes.
    format.setProperty(QTextFormat::FontSizeAdjustment, static_cast<int>(std::round((m_headingScale - 1.0) * (7 - level))));
    setFormat(0, length, format);
    highlightInline(text);
    if(m_livePreview && !revealed(currentBlock())) {
      hideMarkup(text);
    }
    return;
  }

  const QString above = currentBlock().blockNumber() > 0 ? currentBlock().previous().text() : QString();
  if(setextRx().match(text).hasMatch() && !above.trimmed().isEmpty() && !quoteRx().match(above).hasMatch() &&
     !listMarkerRx().match(above).hasMatch() && !atxHeadingRx().match(above).hasMatch()) {
    // The underline of a setext heading. The old highlighter saw nothing here
    // at all, so such a heading looked like ordinary text with a rule under it.
    setFormat(0, length, m_heading[0]);
    return;
  }

  if(thematicBreakRx().match(text).hasMatch()) {
    setFormat(0, length, m_rule);
    if(hides()) {
      ghost(0, length);
    }
    return;
  }

  if(tableRowRx().match(text).hasMatch()) {
    setFormat(0, length, m_table);
    highlightInline(text);
    if(hides()) {
      ghost(0, length);  // the grid is drawn over it
    }
    return;
  }

  const auto quote = quoteRx().match(text);
  if(quote.hasMatch() && quote.capturedLength() > 0) {
    setFormat(0, length, m_quote);
    setFormat(0, static_cast<int>(quote.capturedLength()), m_marker);
    highlightInline(text);
    if(hides()) {
      hideMarkup(text);
      ghost(0, static_cast<int>(quote.capturedLength()));  // the bar is drawn there
    }
    return;
  }

  const auto list = listMarkerRx().match(text);
  if(list.hasMatch()) {
    setFormat(static_cast<int>(list.capturedStart(2)), static_cast<int>(list.capturedLength(2)), m_marker);
    if(!list.captured(4).isEmpty()) {
      setFormat(static_cast<int>(list.capturedStart(4)), static_cast<int>(list.capturedLength(4)), m_marker);
      // A finished task reads as finished, the way it does in the rendered
      // view.
      if(list.captured(5).toLower() == QStringLiteral("x")) {
        setFormat(static_cast<int>(list.capturedEnd(4)), length - static_cast<int>(list.capturedEnd(4)), m_taskDone);
      }
    }
  }

  highlightInline(text);
  if(hides()) {
    hideMarkup(text);
    // The bullet or the box is drawn over its marker, which keeps its width.
    if(list.hasMatch() && !list.captured(2).at(0).isDigit()) {
      const int to = list.captured(4).isEmpty() ? static_cast<int>(list.capturedEnd(2)) : static_cast<int>(list.capturedEnd(4)) - 1;
      ghost(static_cast<int>(list.capturedStart(2)), to - static_cast<int>(list.capturedStart(2)));
    }
  }
}

}  // namespace heap::md
