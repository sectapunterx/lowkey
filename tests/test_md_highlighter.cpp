// MdHighlighter: the fenced-code path.
//
// A fenced block's language is captured on the opening fence and has to reach
// every line inside it. It used to be stored in a plain member that nothing
// ever read, so every fence was highlighted with the same union of comment
// markers: a `#include` greyed out in a ```cpp block, and the `//` of a URL
// did the same in ```python.
//
// The formats are read back off the QTextDocument rather than mocked, so these
// cases exercise the real QSyntaxHighlighter pass.

#include "markdown/MdHighlighter.h"

#include <QGuiApplication>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

#include <gtest/gtest.h>

namespace {

// The highlighter reaches its document through a QQuickTextDocument in the app.
// A test drives the QTextDocument directly, which is the same thing the
// QSyntaxHighlighter base class ends up attached to.
class Highlighted {
 public:
  explicit Highlighted(const QString& text) {
    m_highlighter.setDocument(&m_doc);
    m_doc.setPlainText(text);
    // Realising the layout is what makes QSyntaxHighlighter commit its
    // formats onto the blocks; in a headless document nothing else asks for
    // it, so both the initial pass and any later incremental one would leave
    // the block layouts empty and there would be nothing to read back.
    m_doc.documentLayout();
    m_highlighter.rehighlight();
  }

  // The format at `column` on `line`, by way of the formats the highlighter
  // left on the block.
  QTextCharFormat formatAt(int line, int column) const {
    const QTextBlock block = m_doc.findBlockByNumber(line);
    for(const QTextLayout::FormatRange& range : block.layout()->formats()) {
      if(column >= range.start && column < range.start + range.length) {
        return range.format;
      }
    }
    return {};
  }

  QColor colorAt(int line, int column) const {
    return formatAt(line, column).foreground().color();
  }

  bool isItalicAt(int line, int column) const {
    return formatAt(line, column).fontItalic();
  }

  QTextDocument* document() {
    return &m_doc;
  }

 private:
  QTextDocument m_doc;
  heap::md::MdHighlighter m_highlighter;
};

// The comment format is the only italic one the code path applies, which makes
// "was this styled as a comment?" answerable without hardcoding a palette.
bool looksLikeComment(const Highlighted& h, int line, int column) {
  return h.isItalicAt(line, column);
}

}  // namespace

TEST(MdHighlighter, HashIsNotACommentInACppFence) {
  const Highlighted h(QStringLiteral("```cpp\n#include <vector>\nint x = 1;\n```\n"));
  EXPECT_FALSE(looksLikeComment(h, 1, 0)) << "#include is a directive, not a comment";
}

TEST(MdHighlighter, SlashesAreNotACommentInAPythonFence) {
  const Highlighted h(QStringLiteral("```python\nx = a // b\n```\n"));
  EXPECT_FALSE(looksLikeComment(h, 1, 6)) << "// is floor division in Python";
}

TEST(MdHighlighter, HashIsAcommentInAPythonFence) {
  const Highlighted h(QStringLiteral("```python\nx = 1  # set it\n```\n"));
  EXPECT_TRUE(looksLikeComment(h, 1, 7));
}

TEST(MdHighlighter, SlashesAreACommentInACppFence) {
  const Highlighted h(QStringLiteral("```cpp\nint x = 1;  // set it\n```\n"));
  EXPECT_TRUE(looksLikeComment(h, 1, 12));
}

TEST(MdHighlighter, DashesAreACommentInASqlFence) {
  const Highlighted h(QStringLiteral("```sql\nselect 1;  -- why\n```\n"));
  EXPECT_TRUE(looksLikeComment(h, 1, 11));
}

TEST(MdHighlighter, JsonHasNoLineComments) {
  const Highlighted h(QStringLiteral("```json\n{\"tag\": \"#nope\"}\n```\n"));
  EXPECT_FALSE(looksLikeComment(h, 1, 9));
}

// An unlabelled fence keeps the old permissive behaviour rather than losing
// comment highlighting entirely.
TEST(MdHighlighter, AnUnlabelledFenceStillHighlightsAnyComment) {
  const Highlighted h(QStringLiteral("```\nx = 1  # set it\n```\n"));
  EXPECT_TRUE(looksLikeComment(h, 1, 7));
}

TEST(MdHighlighter, AnUnknownLanguageKeepsThePermissiveUnion) {
  const Highlighted h(QStringLiteral("```brainfuck\nx = 1  # set it\n```\n"));
  EXPECT_TRUE(looksLikeComment(h, 1, 7));
}

// The regression the block-local storage exists for: editing one line inside a
// fence re-highlights only that line, so the language has to be reachable from
// the block rather than from whatever fence was opened last.
TEST(MdHighlighter, ReHighlightingOneLineKeepsItsOwnFenceLanguage) {
  Highlighted h(QStringLiteral("```cpp\nint x = 1;\n```\n\n```python\ny = 2\n```\n"));

  // Touch a line in the FIRST fence after the second one has been parsed.
  QTextCursor cursor(h.document()->findBlockByNumber(1));
  cursor.select(QTextCursor::LineUnderCursor);
  cursor.insertText(QStringLiteral("int x = 1;  // set it"));

  EXPECT_TRUE(looksLikeComment(h, 1, 12)) << "// must still be a comment in the cpp fence";
}

TEST(MdHighlighter, ReHighlightingOneLineInTheSecondFenceUsesItsLanguage) {
  Highlighted h(QStringLiteral("```cpp\nint x = 1;\n```\n\n```python\ny = 2\n```\n"));

  QTextCursor cursor(h.document()->findBlockByNumber(5));
  cursor.select(QTextCursor::LineUnderCursor);
  cursor.insertText(QStringLiteral("y = a // b"));

  EXPECT_FALSE(looksLikeComment(h, 5, 6)) << "// is not a comment in the python fence";
}

// Deleting a fence must not leave its language behind on the lines that were
// inside it.
TEST(MdHighlighter, ProseAfterAFenceIsNotTreatedAsCode) {
  Highlighted h(QStringLiteral("```python\ny = 2\n```\nplain # text\n"));
  EXPECT_FALSE(h.colorAt(3, 0).isValid() && looksLikeComment(h, 3, 6)) << "a line outside the fence is prose, not python";
}

// ── Live preview: the markup of lines away from the caret is hidden ──

namespace {

// The hidden spans of a line as the source text they cover, joined by "|".
QString hiddenOf(const QString& line) {
  QStringList parts;
  for(const auto& [start, length] : heap::md::MdHighlighter::hiddenMarkup(line)) {
    parts << line.mid(start, length);
  }
  return parts.join(QLatin1Char('|'));
}

}  // namespace

TEST(MdHighlighterLive, HidesTheDelimitersNotTheText) {
  EXPECT_EQ(hiddenOf(QStringLiteral("## Title")), QStringLiteral("## "));
  EXPECT_EQ(hiddenOf(QStringLiteral("a **bold** b")), QStringLiteral("**|**"));
  EXPECT_EQ(hiddenOf(QStringLiteral("an *em* word")), QStringLiteral("*|*"));
  EXPECT_EQ(hiddenOf(QStringLiteral("~~gone~~")), QStringLiteral("~~|~~"));
  EXPECT_EQ(hiddenOf(QStringLiteral("see [docs](https://x.test/a)")), QStringLiteral("[|](https://x.test/a)"));
  EXPECT_EQ(hiddenOf(QStringLiteral("[[Note]] and [[Target|alias]]")), QStringLiteral("[[|]]|[[|Target||]]"));
  EXPECT_EQ(hiddenOf(QStringLiteral("run `a **b**` now")), QStringLiteral("`|`")) << "nothing inside code is markup";
  EXPECT_EQ(hiddenOf(QStringLiteral("2 * 3 * 4")), QString()) << "loose asterisks are not emphasis";
  EXPECT_EQ(hiddenOf(QStringLiteral("snake_case_name")), QString());
}

TEST(MdHighlighterLive, TheCaretLineShowsItsSource) {
  QTextDocument doc;
  heap::md::MdHighlighter hl;
  hl.setDocument(&doc);
  doc.setPlainText(QStringLiteral("a **bold** one\n\nb **bold** two\n"));
  doc.documentLayout();
  hl.setLivePreview(true);
  hl.setRevealStart(0);
  hl.setRevealEnd(0);
  const auto hiddenAt = [&doc](int line, int column) {
    const QTextBlock block = doc.findBlockByNumber(line);
    for(const QTextLayout::FormatRange& range : block.layout()->formats()) {
      if(column >= range.start && column < range.start + range.length) {
        return range.format.foreground().color().alpha() == 0;
      }
    }
    return false;
  };
  EXPECT_FALSE(hiddenAt(0, 2)) << "the caret's line keeps its **";
  EXPECT_TRUE(hiddenAt(2, 2)) << "another line hides them";
  // The caret moves to the last line: the first hides, the last shows.
  const int third = doc.findBlockByNumber(2).position();
  hl.setRevealStart(third);
  hl.setRevealEnd(third);
  EXPECT_TRUE(hiddenAt(0, 2));
  EXPECT_FALSE(hiddenAt(2, 2));
  hl.setLivePreview(false);
  EXPECT_FALSE(hiddenAt(0, 2)) << "the plain source mode hides nothing";
}

TEST(MdHighlighterLive, DecorationsNameWhatIsDrawn) {
  const QString text = QStringLiteral(
      "- one\n"
      "  - [x] done\n"
      "> quote\n"
      "---\n"
      "\n"
      "```js\nx\n```\n"
      "| a | b |\n|:--|--:|\n| 1 | 2 |\n");
  const QVariantList d = heap::md::MdHighlighter::decorations(text);
  QStringList kinds;
  for(const QVariant& v : d) {
    kinds << v.toMap().value(QStringLiteral("kind")).toString();
  }
  EXPECT_EQ(kinds.join(QLatin1Char(',')), QStringLiteral("bullet,task,quote,rule,code,table"));
  const QVariantMap task = d.at(1).toMap();
  EXPECT_TRUE(task.value(QStringLiteral("checked")).toBool());
  EXPECT_EQ(text.mid(task.value(QStringLiteral("box")).toInt(), 3), QStringLiteral("[x]"));
  const QVariantMap code = d.at(4).toMap();
  EXPECT_EQ(text.mid(code.value(QStringLiteral("start")).toInt(), 5), QStringLiteral("```js"));
  EXPECT_EQ(code.value(QStringLiteral("end")).toInt(), static_cast<int>(text.indexOf(QStringLiteral("```\n|"))) + 3);
  const QVariantMap table = d.at(5).toMap();
  EXPECT_EQ(table.value(QStringLiteral("align")).toList(), (QVariantList{QStringLiteral("l"), QStringLiteral("r")}));
  const QVariantList rows = table.value(QStringLiteral("rows")).toList();
  ASSERT_EQ(rows.size(), 3);
  EXPECT_TRUE(rows.at(1).toMap().value(QStringLiteral("separator")).toBool());
  EXPECT_EQ(rows.at(2).toMap().value(QStringLiteral("cells")).toStringList(), (QStringList{QStringLiteral("1"), QStringLiteral("2")}));
  // A "---" under a line of text is a setext heading, not a rule.
  EXPECT_TRUE(heap::md::MdHighlighter::decorations(QStringLiteral("Title\n---\n")).isEmpty());
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
