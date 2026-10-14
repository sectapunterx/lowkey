#pragma once

#include <qqmlregistration.h>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QVariantList>
#include <QVariantMap>

namespace heap::md {

// Syntax highlighting for the notes editor.
//
// Replaces NotesHighlighter, which ran about twenty regular expressions
// against each line on its own. A line is not enough context for markdown:
// that highlighter could not tell a heading from a shell comment inside a
// fence, could not see a setext heading at all, and gave every heading level
// the same size, so a document's structure did not read at a glance.
//
// This one keeps a small state machine across lines — fences, frontmatter,
// display maths — and sizes headings by level. Inside a fenced block it hands
// the line to the same language rules the docs viewer uses, so a code block in
// a note looks like code rather than like tinted prose.
//
// It is deliberately lexical rather than AST-driven. A highlighter runs per
// line on every keystroke; a parse per keystroke of a large note would be felt
// as lag, and the AST is already available to everything that needs real
// structure.
class MdHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QQuickTextDocument* target READ target WRITE setTarget NOTIFY targetChanged)
  // Theme colours, by the same names NotesHighlighter used so the QML that
  // supplies them does not have to change.
  Q_PROPERTY(QVariantMap palette READ palette WRITE setPalette NOTIFY paletteChanged)
  // Relative size of an H1, with the levels below it scaled down toward 1.0.
  // 1.0 turns the effect off for anyone who would rather have an even page.
  Q_PROPERTY(qreal headingScale READ headingScale WRITE setHeadingScale NOTIFY headingScaleChanged)
  // Live preview (the notes editor, as Obsidian has it): the markup of a line
  // the caret is not on is hidden - "**", "# ", "[", "](url)", "[[" - so the
  // note reads as drawn while staying one plain text to select, copy and edit
  // across lines. The lines between revealStart and revealEnd (the caret, or
  // the selection) show their source.
  Q_PROPERTY(bool livePreview READ livePreview WRITE setLivePreview NOTIFY livePreviewChanged)
  Q_PROPERTY(int revealStart READ revealStart WRITE setRevealStart NOTIFY revealChanged)
  Q_PROPERTY(int revealEnd READ revealEnd WRITE setRevealEnd NOTIFY revealChanged)

 public:
  explicit MdHighlighter(QObject* parent = nullptr);

  QQuickTextDocument* target() const {
    return m_target;
  }

  void setTarget(QQuickTextDocument* target);

  QVariantMap palette() const {
    return m_palette;
  }

  void setPalette(const QVariantMap& palette);

  qreal headingScale() const {
    return m_headingScale;
  }

  void setHeadingScale(qreal scale);

  bool livePreview() const {
    return m_livePreview;
  }

  void setLivePreview(bool on);

  int revealStart() const {
    return m_revealStart;
  }

  void setRevealStart(int position);

  int revealEnd() const {
    return m_revealEnd;
  }

  void setRevealEnd(int position);

  // The markup ranges of one line that live preview hides, as (start, length)
  // pairs. Public for the tests; the highlighter applies them itself.
  static QList<QPair<int, int>> hiddenMarkup(const QString& line);

  // What live preview draws over the hidden source, one map per element:
  // {kind, start, end} with start/end the document positions of the lines
  // it covers (a reveal range touching them shows the source instead), and
  //   bullet: at (the marker)            task: at (the "["), checked
  //   quote: depth                       rule: -
  //   code: language                     table: rows [{start, cells, header}],
  //                                             align ["l"|"c"|"r"|""]
  // Fenced code and tables are revealed as a whole, the rest line by line.
  Q_INVOKABLE static QVariantList decorations(const QString& text);

 signals:
  void targetChanged();
  void paletteChanged();
  void headingScaleChanged();
  void livePreviewChanged();
  void revealChanged();

 protected:
  void highlightBlock(const QString& text) override;

 private:
  // What a line can be in the middle of. previousBlockState carries it.
  enum BlockState {
    Normal = 0,
    InFence,
    InFrontmatter,
    InMathBlock,
  };

  void rebuildFormats();
  void highlightInline(const QString& text);
  void highlightCodeLine(const QString& text, const QString& language);
  // Whether the line holds the caret or part of the selection.
  bool revealed(const QTextBlock& block) const;
  // Re-highlights the lines whose revealed state changes when the reveal
  // range moves from [oldStart, oldEnd] to the current one.
  void revealMoved(int oldStart, int oldEnd);
  void hideMarkup(const QString& text);
  void ghost(int start, int length);
  // The first and last line of the table or fence a line belongs to; the
  // line itself when it is in neither.
  QPair<QTextBlock, QTextBlock> revealUnit(const QTextBlock& block) const;
  bool hides() const;

  QQuickTextDocument* m_target = nullptr;
  QVariantMap m_palette;
  qreal m_headingScale = 1.45;
  bool m_livePreview = false;
  int m_revealStart = -1;
  int m_revealEnd = -1;
  QTextCharFormat m_hidden;  // collapsed: "## ", "**", "[", "](url)"
  QTextCharFormat m_ghost;   // invisible, keeping its width: what a drawn element sits on

  QTextCharFormat m_heading[6];
  QTextCharFormat m_code;
  QTextCharFormat m_codeFence;
  QTextCharFormat m_quote;
  QTextCharFormat m_marker;
  QTextCharFormat m_link;
  QTextCharFormat m_emphasis;
  QTextCharFormat m_strong;
  QTextCharFormat m_strike;
  QTextCharFormat m_taskDone;
  QTextCharFormat m_mention;
  QTextCharFormat m_ticket;
  QTextCharFormat m_tag;
  QTextCharFormat m_wiki;
  QTextCharFormat m_math;
  QTextCharFormat m_highlight;
  QTextCharFormat m_frontmatter;
  QTextCharFormat m_rule;
  QTextCharFormat m_table;

  // Language rules for whatever fence is currently open. The language itself
  // is not a member: it travels per block (see kCommentStyleFor in the .cpp),
  // because highlightBlock is called for one changed line at a time and a
  // member would hold whichever fence was opened last, not this line's.
  QTextCharFormat m_codeKeyword;
  QTextCharFormat m_codeString;
  QTextCharFormat m_codeComment;
  QTextCharFormat m_codeNumber;
};

}  // namespace heap::md
