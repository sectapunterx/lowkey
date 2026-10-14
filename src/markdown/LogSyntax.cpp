#include "markdown/LogSyntax.h"

#include <QRegularExpression>

#include <algorithm>

namespace heap::md {

bool isLogLanguage(const QString& language) {
  const QString l = language.trimmed().toLower();
  return l == QLatin1String("log") || l == QLatin1String("logs") || l == QLatin1String("stderr") || l == QLatin1String("stdout") ||
         l == QLatin1String("trace");
}

QList<LogSpan> logSpans(const QString& line) {
  struct Rule {
    QRegularExpression rx;
    LogKind kind;
  };

  // Earlier rules win where two would cover the same characters: a quote
  // and a file name ("sinr-error-data.cc") before the levels.
  static const QList<Rule> rules = {
      {QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\"")), LogKind::Quoted},
      {QRegularExpression(
           QStringLiteral("(?:[A-Za-z]:\\\\|/|\\.{1,2}/)?(?:[\\w.-]+[/\\\\])+[\\w.-]+(?::\\d+)?|"
                          "\\b[\\w-]+\\.(?:c|cc|cpp|cxx|h|hpp|py|js|ts|go|rs|java|kt|qml|log|txt|json|yaml|yml|xml|sh)(?::\\d+)?\\b")),
       LogKind::Path},
      {QRegularExpression(QStringLiteral("\\b(?:ERROR|ERR|FATAL|CRITICAL|CRIT|PANIC|FAIL(?:ED|URE)?|SEVERE|ALERT|EMERG)\\b|"
                                         "^\\s*E/|\\bTraceback\\b|\\bException\\b|\\bterminate called\\b"),
                          QRegularExpression::CaseInsensitiveOption),
       LogKind::Error},
      {QRegularExpression(QStringLiteral("\\b(?:WARN(?:ING)?|WRN)\\b|^\\s*W/"), QRegularExpression::CaseInsensitiveOption), LogKind::Warn},
      {QRegularExpression(QStringLiteral("\\b(?:INFO|INF|DEBUG|DBG|TRACE|TRC|VERBOSE|NOTICE)\\b|^\\s*[IDV]/")), LogKind::Quiet},
      {QRegularExpression(QStringLiteral("\\b\\d{4}-\\d{2}-\\d{2}(?:[T ]\\d{2}:\\d{2}(?::\\d{2}(?:[.,]\\d+)?)?(?:Z|[+-]\\d{2}:?\\d{2})?)?|"
                                         "\\b\\d{1,2}:\\d{2}:\\d{2}(?:[.,]\\d+)?\\b|[+]\\d+(?:\\.\\d+)?s\\b|\\[\\s*\\d+\\.\\d+\\]")),
       LogKind::Time},
  };
  QList<LogSpan> out;
  QList<bool> taken(line.size(), false);
  for(const Rule& r : rules) {
    for(auto it = r.rx.globalMatch(line); it.hasNext();) {
      const auto m = it.next();
      const qsizetype s = m.capturedStart();
      const qsizetype e = m.capturedEnd();
      if(e <= s || std::any_of(taken.begin() + s, taken.begin() + e, [](bool b) {
           return b;
         })) {
        continue;
      }
      std::fill(taken.begin() + s, taken.begin() + e, true);
      out.append({static_cast<int>(s), static_cast<int>(e - s), r.kind});
    }
  }
  std::sort(out.begin(), out.end(), [](const LogSpan& a, const LogSpan& b) {
    return a.start < b.start;
  });
  return out;
}

}  // namespace heap::md
