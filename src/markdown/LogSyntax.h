#pragma once

#include <QList>
#include <QString>

namespace heap::md {

// What a line of a log is made of (APP-315), for ```log blocks and .log
// files: the level, the time, a file path; one rule shared by the note's
// editor (MdHighlighter) and the drawn note (CodeHighlighter).

enum class LogKind {
  Error,   // ERROR, FATAL, CRITICAL, PANIC, FAIL(ED), E/ (android), Traceback
  Warn,    // WARN, WARNING, W/
  Quiet,   // INFO, DEBUG, TRACE, VERBOSE, NOTICE
  Time,    // 2026-10-14 21:05:03.123, 21:05:03, +0.102928570s, [12.345]
  Path,    // src/a/b.cc, C:\x\y.log, file.cc:754
  Quoted,  // "…"
};

struct LogSpan {
  int start = 0;
  int length = 0;
  LogKind kind = LogKind::Quoted;
};

bool isLogLanguage(const QString& language);

// The spans of one line, left to right, not overlapping.
QList<LogSpan> logSpans(const QString& line);

}  // namespace heap::md
